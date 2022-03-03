//
// Created by maxwe on 2021-09-18.
//

#include "dictionary.hpp"

using namespace jem;

namespace {
  inline constexpr jem_u32_t default_bucket_count = 16;
  inline jem_u32_t get_min_bucket_to_reserve_for_entries(jem_u32_t entries) noexcept {
    if ( entries == 0 )
      return 0;
    return std::bit_ceil(entries * 4 / 3 + 1);
  }
  
  inline constexpr jem_u32_t hash_offset32_v = 5381;
  
  template <typename T> requires (std::has_unique_object_representations_v<T>)
  inline constexpr jem_u32_t hash_append_bytes32(const T* address, size_t length, jem_u32_t val = hash_offset32_v) noexcept {
    for (const T* const end = address + length; address != end; ++address) {
      if constexpr ( sizeof(T) != 1 ) {
        for (const jem_u8_t b : std::bit_cast<std::array<jem_u8_t, sizeof(T)>>(*address)) {
          val = (val << 5) + val + b;
        }
      }
      else {
        val = (val << 5) + val + std::bit_cast<jem_u8_t>(*address);
      }
    }
    return val;
  }

  inline constexpr jem_u32_t hash_string(std::string_view str) noexcept {
    return hash_append_bytes32(str.data(), str.size());
  }

  template <std::unsigned_integral UInt>
  inline constexpr bool is_pow2(UInt x) {
    return (x & (x - 1)) == 0;
  }

  impl::dictionary_entry_base** alloc_table(jem_u32_t count) noexcept {
    return static_cast<impl::dictionary_entry_base**>(_aligned_malloc((sizeof(void*) + sizeof(jem_u32_t)) * (count + 1), alignof(void*)));
  }
}

void impl::dictionary::init(unsigned Size) noexcept {
  assert(is_pow2(Size) && "Init Size must be a power of 2 or zero!");

  unsigned NewNumBuckets = Size ? Size : default_bucket_count;
  NumItems = 0;
  NumTombstones = 0;

  TheTable = alloc_table(NewNumBuckets);
  /*TheTable = static_cast<impl::dictionary_entry_base **>(calloc(
      NewNumBuckets + 1, sizeof(impl::dictionary_entry_base **) + sizeof(unsigned)));*/

  // Set the member only if TheTable was successfully allocated
  NumBuckets = NewNumBuckets;

  // Allocate one extra bucket, set it to look filled so the iterators stop at
  // end.
  TheTable[NumBuckets] = (impl::dictionary_entry_base *)2;
}

jem_u32_t impl::dictionary::rehash_table(unsigned int BucketNo) noexcept {
  unsigned NewSize;
  unsigned *HashTable = (unsigned *)(TheTable + NumBuckets + 1);

  // If the hash table is now more than 3/4 full, or if fewer than 1/8 of
  // the buckets are empty (meaning that many are filled with tombstones),
  // grow/rehash the table.
  if (NumItems * 4 > NumBuckets * 3) [[unlikely]] {
      NewSize = NumBuckets * 2;
    }
  else if (NumBuckets - (NumItems + NumTombstones) <= NumBuckets / 8) [[unlikely]] {
      NewSize = NumBuckets;
    }
  else {
    return BucketNo;
  }

  unsigned NewBucketNo = BucketNo;
  // Allocate one extra bucket which will always be non-empty.  This allows the
  // iterators to stop at end.
  auto NewTableArray = alloc_table(NewSize);

  auto* NewHashArray = (unsigned *)(NewTableArray + NewSize + 1);
  NewTableArray[NewSize] = (impl::dictionary_entry_base *)2;

  // Rehash all the items into their new buckets.  Luckily :) we already have
  // the hash values available, so we don't have to rehash any strings.
  for (unsigned I = 0, E = NumBuckets; I != E; ++I) {
    impl::dictionary_entry_base *Bucket = TheTable[I];
    if (Bucket && Bucket != get_tombstone_val()) {
      // Fast case, bucket available.
      unsigned FullHash = HashTable[I];
      unsigned NewBucket = FullHash & (NewSize - 1);
      if (!NewTableArray[NewBucket]) {
        NewTableArray[FullHash & (NewSize - 1)] = Bucket;
        NewHashArray[FullHash & (NewSize - 1)] = FullHash;
        if (I == BucketNo)
          NewBucketNo = NewBucket;
        continue;
      }

      // Otherwise probe for a spot.
      unsigned ProbeSize = 1;
      do {
        NewBucket = (NewBucket + ProbeSize++) & (NewSize - 1);
      } while (NewTableArray[NewBucket]);

      // Finally found a slot.  Fill it in.
      NewTableArray[NewBucket] = Bucket;
      NewHashArray[NewBucket] = FullHash;
      if (I == BucketNo)
        NewBucketNo = NewBucket;
    }
  }


  dealloc_table();

  TheTable = NewTableArray;
  NumBuckets = NewSize;
  NumTombstones = 0;
  return NewBucketNo;
}

jem_u32_t impl::dictionary::lookup_bucket_for(std::string_view Key) noexcept {
  jem_u32_t HTSize = NumBuckets;
  if (HTSize == 0) { // Hash table unallocated so far?
    init(default_bucket_count);
    HTSize = NumBuckets;
  }
  jem_u32_t FullHashValue = hash_string(Key);
  jem_u32_t BucketNo = FullHashValue & (HTSize - 1);
  auto* HashTable = (jem_u32_t*)(TheTable + NumBuckets + 1);

  unsigned ProbeAmt = 1;
  int FirstTombstone = -1;
  while (true) {
    impl::dictionary_entry_base *BucketItem = TheTable[BucketNo];
    // If we found an empty bucket, this key isn't in the table yet, return it.
    if (!BucketItem) [[likely]] {
        // If we found a tombstone, we want to reuse the tombstone instead of an
        // empty bucket.  This reduces probing.
        if (FirstTombstone != -1) {
          HashTable[FirstTombstone] = FullHashValue;
          return FirstTombstone;
        }

        HashTable[BucketNo] = FullHashValue;
        return BucketNo;
      }

    if (BucketItem == get_tombstone_val()) {
      // Skip over tombstones.  However, remember the first one we see.
      if (FirstTombstone == -1)
        FirstTombstone = BucketNo;
    } else if (HashTable[BucketNo] == FullHashValue) [[likely]] {
        // If the full hash value matches, check deeply for a match.  The common
        // case here is that we are only looking at the buckets (for item info
        // being non-null and for the full hash value) not at the items.  This
        // is important for cache locality.

        // Do the comparison like this because Name isn't necessarily
        // null-terminated!
        auto *ItemStr = (char*)BucketItem + ItemSize;
        if (Key == std::string_view(ItemStr, BucketItem->get_key_length())) {
          // We found a match!
          return BucketNo;
        }
      }

    // Okay, we didn't find the item.  Probe to the next bucket.
    BucketNo = (BucketNo + ProbeAmt) & (HTSize - 1);

    // Use quadratic probing, it has fewer clumping artifacts than linear
    // probing and has good cache behavior in the common case.
    ++ProbeAmt;
  }
}

void impl::dictionary::explicit_init(jem_u32_t initSize) noexcept {

  // If a size is specified, initialize the table with that many buckets.
  if (initSize) {
    // The table will grow when the number of entries reach 3/4 of the number of
    // buckets. To guarantee that "InitSize" number of entries can be inserted
    // in the table without growing, we allocate just what is needed here.
    init(get_min_bucket_to_reserve_for_entries(initSize));
    return;
  }

  // Otherwise, initialize it with zero buckets to avoid the allocation.
  TheTable = nullptr;
  NumBuckets = 0;
  NumItems = 0;
  NumTombstones = 0;
}

int impl::dictionary::find_key(std::string_view Key) const noexcept {
  unsigned HTSize = NumBuckets;
  if (HTSize == 0)
    return -1; // Really empty table?
  jem_u32_t FullHashValue = hash_string(Key);
  jem_u32_t BucketNo = FullHashValue & (HTSize - 1);
  auto* HashTable = (jem_u32_t*)(TheTable + NumBuckets + 1);

  unsigned ProbeAmt = 1;
  while (true) {
    impl::dictionary_entry_base *BucketItem = TheTable[BucketNo];
    // If we found an empty bucket, this key isn't in the table yet, return.
    if (!BucketItem) [[likely]]
      return -1;

    if (BucketItem == get_tombstone_val()) {
      // Ignore tombstones.
    } else if (HashTable[BucketNo] == FullHashValue) [[likely]] {
        // If the full hash value matches, check deeply for a match.  The common
        // case here is that we are only looking at the buckets (for item info
        // being non-null and for the full hash value) not at the items.  This
        // is important for cache locality.

        // Do the comparison like this because NameStart isn't necessarily
        // null-terminated!
        auto *ItemStr = (char *)BucketItem + ItemSize;
        if (Key == std::string_view(ItemStr, BucketItem->get_key_length())) {
          // We found a match!
          return BucketNo;
        }
      }

    // Okay, we didn't find the item.  Probe to the next bucket.
    BucketNo = (BucketNo + ProbeAmt) & (HTSize - 1);

    // Use quadratic probing, it has fewer clumping artifacts than linear
    // probing and has good cache behavior in the common case.
    ++ProbeAmt;
  }
}

impl::dictionary_entry_base * impl::dictionary::remove_key(std::string_view Key) noexcept {
  int Bucket = find_key(Key);
  if (Bucket == -1)
    return nullptr;

  impl::dictionary_entry_base *Result = TheTable[Bucket];
  TheTable[Bucket] = get_tombstone_val();
  --NumItems;
  ++NumTombstones;
  assert(NumItems + NumTombstones <= NumBuckets);

  return Result;
}

void impl::dictionary::remove_key(dictionary_entry_base *V) noexcept {
  auto VStr = (char *)V + ItemSize;
  impl::dictionary_entry_base *V2 = remove_key(std::string_view(VStr, V->get_key_length()));
  (void)V2;
  assert(V == V2 && "Didn't find key?");
}

void impl::dictionary::dealloc_table() noexcept {
  _aligned_free(TheTable);
}