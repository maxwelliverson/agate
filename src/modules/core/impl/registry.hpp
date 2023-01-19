//
// Created by maxwe on 2023-01-10.
//

#ifndef AGATE_CORE_IMPL_REGISTRY_HPP
#define AGATE_CORE_IMPL_REGISTRY_HPP

#include "config.hpp"

#include "agate/atomic.hpp"
#include "../object.hpp"

#include <span>

#include <immintrin.h>

namespace agt {
  namespace reg_impl {

    template <size_t N>
    AGT_forceinline static bool is_aligned(const void* ptr) noexcept {
      return (reinterpret_cast<uintptr_t>(ptr) & (N - 1)) == 0;
    }

/*

    enum {
      REGISTRY_ENTRY_IS_BOUND_FLAG = 0x1,
      REGISTRY_ENTRY_IS_IMPORTED_FLAG = 0x2
    };

    struct trie_node;

    union child_table_entry {
      agt_u32_t  next;
      trie_node* node;
    };

    enum node_flags {
      node_prefix_is_inline = 0x1,
      node_is_branch        = 0x2,
      node_is_valid_branch  = 0x4,
      node_is_root          = 0x8
    };

    struct small_trie_buffer {
      agt_u8_t  length;
      std::byte buffer[11];
    };

    struct trie_node : object {
      agt_u32_t        length;
      const std::byte* address;
      trie_node*       chainNext;
      trie_node*       child;
      object*          obj;
    };

    struct trie_node_allocator {

    };


    trie_node* new_leaf(trie_node_allocator* allocator, const void* suffix, size_t suffixLength) noexcept;

    trie_node* new_branch(trie_node_allocator* allocator, const void* prefix, size_t prefixLength) noexcept;

    void       delete_node(trie_node_allocator* allocator, trie_node* node) noexcept;

    void       drop_prefix(trie_node_allocator* allocator, trie_node* node, size_t bytes) noexcept;

    inline std::span<const std::byte> get_node_bytes(trie_node& node) noexcept {
      if (node.prefixIsInline)
        return { node.inlinePrefix.buffer, node.inlinePrefix.length };
      else
        return { node.externPrefix.address, node.externPrefix.length };
    }


    std::pair<trie_node*, bool> insert(trie_node_allocator* allocator, trie_node*& head, const std::byte* addrPos, const std::byte* addrEnd) noexcept;

    std::pair<trie_node*, bool> insert_nonnull(trie_node_allocator* allocator, trie_node*& head, const std::byte* addrPos, const std::byte* addrEnd) noexcept;

    void resize_children_table(trie_branch* branch, size_t newCapacity) noexcept {
      auto oldChildren = branch->children;
      auto newChildren = (trie_node**)std::realloc(oldChildren, newCapacity * sizeof(void*));
      branch->childrenCapacity = newCapacity;

      if (oldChildren != newChildren) {
        for (auto i = 0u; i < branch->childrenCount; ++i)
          newChildren[i]->parent = newChildren + i;
        branch->children = newChildren;
      }
    }

    void add_child(trie_branch*& branch, trie_node* child) noexcept {
      if (branch->childrenCount == branch->childrenCapacity) [[unlikely]]
        resize_children_table(branch, branch->childrenCapacity * 2);
      agt_u32_t index = branch->childrenCount++;
      branch->children[index] = child;
      child->parentPtr = branch->children + index;
    }

    bool has_shared_prefix(trie_node* child, const std::byte* addrPos) noexcept {
      std::byte firstByte;
      if (child->prefixIsInline)
        firstByte = child->inlinePrefix.buffer[0];
      else
        firstByte = *child->externPrefix.address;
      return firstByte == *addrPos;
    }

    void rebind_node(trie_node_allocator* allocator, trie_node* node, const std::byte* newPrefix, size_t length) noexcept;


    std::pair<trie_node*, bool> insert_at_leaf(trie_node_allocator* allocator, trie_node*& leaf, const std::byte* addrPos, const std::byte* addrEnd) noexcept {
      const auto nodeBytes = get_node_bytes(*leaf);
      auto pos = addrPos;
      auto nodePos = nodeBytes.data();
      const auto nodeEnd = std::to_address(nodeBytes.end());
      do {
        if (*pos != *nodePos) {

        }



        if (pos == addrEnd) {
          if (nodePos == nodeEnd)
            return { leaf, false };
          auto newBranch = new_branch(allocator, nodePos, nodeEnd - nodePos);
          newBranch->isValidBranch = 1;
          newBranch->parent = leaf->parent;

          leaf->parent = newBranch


              leaf = newBranch;
        }

        if (nodePos == nodeEnd) {
          auto newBranch = new_branch(allocator, nodeBytes.data(), nodeBytes.size());
          newBranch->isValidBranch = 1;
          rebind_node(allocator, leaf, addrPos, addrEnd - addrPos);
          add_child(newBranch, leaf);
          leaf = newBranch;
          return { leaf, true };
        }

      } while(true);


    }

    std::pair<trie_node*, bool> insert_at_branch(trie_node_allocator* allocator, trie_branch*& branch, const std::byte* addrPos, const std::byte* addrEnd) noexcept {

    }


    std::pair<trie_node*, bool> insert_nonnull(trie_node_allocator* allocator, trie_node*& head, const std::byte* addrPos, const std::byte* addrEnd) noexcept {
      if (head->isBranch)
        return insert_at_branch(allocator, (trie_branch*&)head, addrPos, addrEnd);
      else
        return insert_at_leaf(allocator, head, addrPos, addrEnd);
    }

    std::pair<trie_node*, bool> insert(trie_node_allocator* allocator, trie_node*& head, const std::byte* addrPos, const std::byte* addrEnd) noexcept {
      if (head)
        return insert_nonnull(allocator, head, addrPos, addrEnd);
      auto leaf = new_leaf(allocator, addrPos, addrEnd - addrPos);
      leaf->parent = head;
      head = leaf;
      return { leaf, true };
    }

*/




    /*class lockfree_trie {

    };*/

    struct registry_entry : object {
      agt_u32_t   flags;
      agt_u32_t   hash;
      agt_u32_t   nameLength;
      agt_u32_t   refCount;
      agt_u32_t   isValid;
      object*     pObject;
      agt_char_t  nameBuffer[];
    };


#define AGT_registry_tombstone reinterpret_cast<registry_entry*>(static_cast<uintptr_t>(-1))

    struct registry {
      registry_entry** table;
      agt_u32_t        tableSize;
      agt_u32_t        tombstoneCount;
      agt_u32_t        entryCount;
      agt_u32_t        indexMask;
      read_write_mutex mutex;
    };


#if defined(AGT_NAME_CACHE_EVICTION_POLICY)
#endif

    inline constexpr static size_t RegistryCacheBucketsPerSlot = 4;

    struct alignas(32) registry_cache_entry {
      registry_entry* buckets[RegistryCacheBucketsPerSlot];
    };

    struct registry_cache {
      registry*             instanceRegistry;
      registry_cache_entry* slots;
      agt_u32_t             slotCount;
      agt_u32_t             indexMask;
      agt_u32_t             tagMask;
    };



    [[nodiscard]] agt_u32_t _hash_string(const agt_char_t* string, size_t length) noexcept;

    bool _retain_entry(registry_entry* entry) noexcept;

    void _release_entry(registry_entry* entry) noexcept;



    // Lock must be held for reading
    [[nodiscard]] registry_entry* _lookup_instance_entry(registry& reg, const agt_char_t* string, size_t length, agt_u32_t hash) noexcept {
      const agt_u32_t indexMask = reg.indexMask;
      const agt_u32_t initialIndex = hash & indexMask;
      agt_u32_t probeDistance = 1;

      for (agt_u32_t index = initialIndex; ; index += (probeDistance++), index = index & indexMask) {
        auto entry = reg.table[index];

        if (entry == nullptr || entry == AGT_registry_tombstone)
          return nullptr;
        if (entry->hash == hash && entry->nameLength == length && (memcmp(entry->nameBuffer, string, length) == 0))
          return entry;
      }
    }


    [[nodiscard]] std::pair<registry_entry*, bool> _try_create_instance_entry(agt_ctx_t ctx, registry& reg, read_lock* readLock, const agt_char_t* string, size_t length, agt_u32_t hash) noexcept {
      write_lock lock;
      if (readLock)
        lock = write_lock(*readLock);
      else
        lock = write_lock(reg.mutex);



    }




    void _set_most_recently_used(registry_cache_entry& entry, agt_u32_t entryIndex) noexcept {
      if (entryIndex < entry.maxBucketIndex)
        std::swap(entry.buckets[entryIndex], entry.buckets[entry.maxBucketIndex]);
    }

    void _rotate_cache_entries(registry_cache_entry& cacheEntry) noexcept {
      __m256i cacheRegister = _mm256_load_si256((const __m256i*)&cacheEntry.buckets);
      _mm256_permute4x64_epi64();
    }

    bool _add_to_full_cache(registry_cache_entry& cacheEntry, registry_entry* entry) noexcept {
      if (_retain_entry(entry)) {
        _release_entry(cacheEntry.buckets[0]);
        cacheEntry.buckets[0] = entry;

        if (cacheEntry.maxBucketIndex != 0) {

        }
      }
    }

    // Uses LRU ejection policy. Maybe worth changing depending on profiling results
    bool _add_to_cache(registry_cache_entry& cacheEntry, registry_entry* entry, agt_i32_t cacheIndex) noexcept {
      if (_retain_entry(entry)) {
        cacheEntry.buckets[cacheIndex] = entry;
        _set_most_recently_used(cacheEntry, cacheIndex);
        return true;
      }
      return false;
    }

    [[nodiscard]] registry_entry* _lookup_entry(registry_cache& reg, const agt_char_t* string, size_t length) noexcept {
      const auto hash = _hash_string(string, length);

      const auto slotIndex = hash & reg.indexMask;
      auto& cacheEntry = reg.slots[slotIndex];

      const __int64* loadOffset = reinterpret_cast<const __int64*>(offsetof(registry_entry, hash));

      // This algorithm works by loading values from _mm256_i64gather_epi64 with the desired offset as the "base address", and then the actual addresses as the offset vector. This is because rather than loading from the same array at multiple offsets, this is loading from different arrays at the same offsets.

      const __m256i cacheRegister = _mm256_load_si256((const __m256i*)&cacheEntry.buckets);
      const __m256i zeroRegister  = _mm256_setzero_si256();
      const __m256i entryHashes   = _mm256_i64gather_epi64(loadOffset, cacheRegister, 1); // Genuinely no clue if this is valid, but in theory it should be???
      const __m256i entryMask     = _mm256_cmpeq_epi64(entryHashes, _mm256_set1_epi64x((length << 32) | static_cast<agt_u64_t>(hash)));

      if (_mm256_testnzc_si256(zeroRegister, entryMask)) {
        // A valid entry mask was found! try to find valid entry in slots.

        auto maskedCacheEntry = std::bit_cast<registry_cache_entry>(_mm256_and_epi64(cacheRegister, entryMask));

#define AGT_LOOKUP_REGISTER_CACHE_ENTRY_PERMUTE(n, imm) \
  if (auto entry = maskedCacheEntry.buckets[n]) { \
    if (memcmp(entry->nameBuffer, string, length) == 0) { \
      _mm256_store_si256((__m256i*)&cacheEntry.buckets, _mm256_permute4x64_epi64(cacheRegister, imm)); \
      return entry; \
    }\
  }

#define AGT_LOOKUP_REGISTER_CACHE_ENTRY(n) \
  if (auto entry = maskedCacheEntry.buckets[n]) { \
    if (memcmp(entry->nameBuffer, string, length) == 0) { \
      return entry; \
    }\
  }

        AGT_LOOKUP_REGISTER_CACHE_ENTRY(3)
        AGT_LOOKUP_REGISTER_CACHE_ENTRY_PERMUTE(2, 0xB4)
        AGT_LOOKUP_REGISTER_CACHE_ENTRY_PERMUTE(1, 0x9C)
        AGT_LOOKUP_REGISTER_CACHE_ENTRY_PERMUTE(0, 0x93)

#undef AGT_LOOKUP_REGISTER_CACHE_ENTRY_PERMUTE
#undef AGT_LOOKUP_REGISTER_CACHE_ENTRY

        // very very unlikely that this point in the code is reached, but it is possible for a 2 entries to have both the same hash and length but be different.

        // fallthrough to case where nothing was found
      }

      // No matching entry was found, so search in instance registry, and if a valid entry is found, add it to the cache.

      registry_entry* instanceEntry;

      {
        read_lock lock{reg.instanceRegistry->mutex};
        instanceEntry = _lookup_instance_entry(*reg.instanceRegistry, string, length, hash);
      }

      if (instanceEntry) {
        _retain_entry(instanceEntry);
        if (auto bootedEntry = cacheEntry.buckets[0])
          _release_entry(bootedEntry);
        _mm256_store_si256((__m256i*)&cacheEntry.buckets, _mm256_permute4x64_epi64(cacheRegister, 0x93));
        cacheEntry.buckets[3] = instanceEntry;
      }

      // No instance entry was found either :(

      return instanceEntry;
    }

    [[nodiscard]] registry_entry& _lookup_or_create_entry(agt_ctx_t ctx, registry_cache& reg, const agt_char_t* string, size_t length) noexcept {

    }




  }
}

#endif//AGATE_CORE_IMPL_REGISTRY_HPP
