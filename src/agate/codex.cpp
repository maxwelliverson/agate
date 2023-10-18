//
// Created by maxwe on 2023-08-16.
//

#include "codex.hpp"



using namespace agt;

namespace {
  inline constexpr agt_u32_t default_bucket_count = 16;
  inline agt_u32_t get_min_bucket_to_reserve_for_entries(agt_u32_t entries) noexcept {
    if ( entries == 0 )
      return 0;
    return std::bit_ceil(entries * 4 / 3 + 1);
  }

  inline constexpr agt_u32_t hash_offset32_v = 5381;

  template <typename T> requires (std::has_unique_object_representations_v<T>)
  inline constexpr agt_u32_t hash_append_bytes32(const T* address, size_t length, agt_u32_t val = hash_offset32_v) noexcept {
    for (const T* const end = address + length; address != end; ++address) {
      if constexpr ( sizeof(T) != 1 ) {
        for (const agt_u8_t b : std::bit_cast<std::array<agt_u8_t, sizeof(T)>>(*address)) {
          val = (val << 5) + val + b;
        }
      }
      else {
        val = (val << 5) + val + std::bit_cast<agt_u8_t>(*address);
      }
    }
    return val;
  }

  inline agt_u32_t hash_string(const void* data, size_t bytes) noexcept {
    return hash_append_bytes32(static_cast<const std::byte*>(data), bytes);
  }

  inline size_t getTableMemorySize(uint32_t bucketCount) noexcept {
    return (sizeof(void*) + sizeof(agt_u32_t)) * (bucketCount + 1);
  }

  impl::codex_entry_base** _alloc_table(uint32_t count) noexcept {
    const size_t tableSize = getTableMemorySize(count);
    const auto mem = alloc_aligned(tableSize, alignof(void*));
    std::memset(mem, 0, tableSize);
    return static_cast<impl::codex_entry_base**>(mem);
  }
}

void impl::codex::_init(agt_u32_t size) noexcept {
  assert(std::has_single_bit(size) && "Init Size must be a power of 2 or zero!");

  uint32_t newBucketCount = size ? size : default_bucket_count;

  m_entryCount = 0;
  m_tombstoneCount = 0;

  m_table = _alloc_table(newBucketCount);

  m_bucketCount = newBucketCount;

  m_table[newBucketCount] = (impl::codex_entry_base*)2;
}

void impl::codex::_explicit_init(agt_u32_t size) noexcept {
  if (size) {
    _init(get_min_bucket_to_reserve_for_entries(size));
    return;
  }

  // Otherwise, initialize it with zero buckets to avoid the allocation.
  m_table = nullptr;
  m_bucketCount = 0;
  m_entryCount = 0;
  m_tombstoneCount = 0;
}

agt_u32_t impl::codex::_rehash_table(agt_u32_t bucketIndex) noexcept {
  uint32_t newSize;

  // If the hash table is now more than 3/4 full, or if fewer than 1/8 of
  // the buckets are empty (meaning that many are filled with tombstones),
  // grow/rehash the table.
  if (m_entryCount * 4 > m_bucketCount * 3) [[unlikely]]
    newSize = m_bucketCount * 2;
  else if (m_bucketCount - (m_entryCount + m_tombstoneCount) <= m_bucketCount / 8) [[unlikely]]
    newSize = m_bucketCount;
  else
    return bucketIndex;

  uint32_t newIndexMask = newSize - 1;

  const auto hashTable = (uint32_t*)(m_table + m_bucketCount + 1);
  uint32_t   newBucketIndex = bucketIndex;

  auto newTable = _alloc_table(newSize);

  auto* newHashTable = (uint32_t*)(newTable + newSize + 1);

  const auto tombstone = get_tombstone_val();
  const uint32_t end = m_bucketCount;
  for (uint32_t i = 0; i < end; ++i) {
    auto* bucket = m_table[i];
    if (bucket && bucket != tombstone) {
      uint32_t hash = hashTable[i];
      uint32_t newBucket = hash & newIndexMask;

      if (newTable[newBucket]) {
        uint32_t probeSize = 1;
        do {
          newBucket = (newBucket + probeSize++) & newIndexMask;
        } while(newTable[newBucket]);
      }


      newTable[newBucket] = bucket;
      newHashTable[newBucket] = hash;
      if (i == bucketIndex)
        newBucketIndex = newBucket;
    }
  }

  _dealloc_table();

  m_table = newTable;
  m_bucketCount = newSize;
  m_tombstoneCount = 0;
  return newBucketIndex;
}

bool impl::codex::_key_matches_entry(const void *keyData, size_t keyLength, agt::impl::codex_entry_base *entry) const noexcept {
  if (entry->get_key_length() != keyLength)
    return false;

  return std::memcmp(entry->_get_key_data(), keyData, keyLength * m_keyCharSize) == 0;
}

agt_u32_t impl::codex::_lookup_bucket_for(const void *keyData, size_t keyLength) noexcept {
  agt_u32_t hashTableSize = m_bucketCount;
  if (hashTableSize == 0) { // Hash table unallocated so far?
    _init(default_bucket_count);
    hashTableSize = m_bucketCount;
  }

  agt_u32_t indexMask   = hashTableSize - 1;
  agt_u32_t hashValue   = hash_string(keyData, keyLength);
  agt_u32_t bucketIndex = hashValue & indexMask;

  auto*     hashTable = (agt_u32_t*)(m_table + hashTableSize + 1);

  uint32_t probeStride = 1;
  int      firstTombstone = -1;

  const auto tombstone = get_tombstone_val();

  while (true) {
    auto maybeBucket = m_table[bucketIndex];

    if (!maybeBucket) [[likely]] {
      if (firstTombstone != -1) {
        hashTable[firstTombstone] = hashValue;
        return firstTombstone;
      }

      hashTable[bucketIndex] = hashValue;
      return bucketIndex;
    }

    if (maybeBucket == tombstone) {
      if (firstTombstone == -1)
        firstTombstone = (int)bucketIndex;
    }
    else if (hashTable[bucketIndex] == hashValue) [[likely]] {
      if (_key_matches_entry(keyData, keyLength, maybeBucket))
        return bucketIndex;
    }

    bucketIndex = (bucketIndex + probeStride) & indexMask;
    ++probeStride;
  }
}

agt_i32_t impl::codex::_find_key(const void *keyData, size_t keyLength) const noexcept {
  agt_u32_t hashTableSize = m_bucketCount;
  if (hashTableSize == 0)
    return -1;

  agt_u32_t indexMask   = hashTableSize - 1;
  agt_u32_t hashValue   = hash_string(keyData, keyLength);
  agt_u32_t bucketIndex = hashValue & indexMask;

  auto*     hashTable = (agt_u32_t*)(m_table + hashTableSize + 1);

  uint32_t probeStride = 1;

  const auto tombstone = get_tombstone_val();

  while (true) {
    auto maybeBucket = m_table[bucketIndex];

    if (!maybeBucket) [[likely]]
      return -1;

    if (maybeBucket == tombstone) {

    }
    else if (hashTable[bucketIndex] == hashValue) [[likely]] {
      if (_key_matches_entry(keyData, keyLength, maybeBucket))
        return (int)bucketIndex;
    }

    bucketIndex = (bucketIndex + probeStride) & indexMask;
    ++probeStride;
  }
}


impl::codex_entry_base* impl::codex::_remove_key(const void *keyData, size_t keyLength) noexcept {
  int bucket = _find_key(keyData, keyLength);
  if (bucket == -1)
    return nullptr;

  auto result = m_table[bucket];
  m_table[bucket] = get_tombstone_val();
  --m_entryCount;
  ++m_tombstoneCount;
  assert( m_entryCount + m_tombstoneCount <= m_bucketCount );

  return result;
}

void impl::codex::_remove_key(agt::impl::codex_entry_base* entry) noexcept {
  auto result = _remove_key(entry->_get_key_data(), entry->get_key_length());
  (void)result;
  assert( entry == result && "Tried to remove entry not in codex." );
}

void impl::codex::_dealloc_table() noexcept {
  free_aligned(m_table, getTableMemorySize(m_bucketCount), alignof(void*));
}
