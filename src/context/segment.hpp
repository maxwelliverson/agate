//
// Created by maxwe on 2022-03-08.
//

#ifndef AGATE_INTERNAL_CONTEXT_PAGE_HPP
#define AGATE_INTERNAL_CONTEXT_PAGE_HPP

#include "fwd.hpp"

#include "random.hpp"

namespace agt {

  inline constexpr static size_t SegmentIdBits = 20;



  enum class seg_type {
    localObjectInfoPool,
  };

  struct seg_table_entry;
  struct seg_descriptor;
  struct seg_descriptor_pool;


  struct seg_descriptor_header {
    seg_descriptor_header* next;
    seg_descriptor_header* prev;
  };

  struct seg_list : seg_descriptor_header {
    size_t elCount;
  };

  struct seg_table {
    seg_table_entry* table;
    size_t           bucketCount;
    size_t           entryCount;
    size_t           abandonnedCount;
  };

  struct seg_manager {
    seg_table             table;
    rng32                 idGenerator;
    seg_list              segList;
    seg_list              emptyList;
    seg_descriptor_pool** poolStackBase;
    seg_descriptor_pool** poolStackHead;
    seg_descriptor_pool** poolStackEnd;
    seg_descriptor_pool*  allocPool;
    seg_descriptor_pool*  freePool;
    seg_descriptor_pool*  emptyPool;
  };


  void         segManagerInit(seg_manager* sm, size_t tableBuckets, rng_seeder* seeder) noexcept;

  agt_status_t segManagerLookup(seg_manager* sm, agt_u32_t id, seg_descriptor** pDescriptor) noexcept;

  agt_status_t segManagerAlloc(seg_manager* sm, seg_type type, size_t size, seg_descriptor** pDescriptor) noexcept;



  void         segManagerDestroy(seg_manager* sm) noexcept;









  agt_u32_t  rehash_table(agt_u32_t BucketNo) noexcept;

  /// lookup_bucket_for - Look up the bucket that the specified string should end
  /// up in.  If it already exists as a key in the map, the Item pointer for the
  /// specified bucket will be non-null.  Otherwise, it will be null.  In either
  /// case, the FullHashValue field of the bucket will be set to the hash value
  /// of the string.
  agt_u32_t lookup_bucket_for(std::string_view Key) noexcept;

  /// find_key - Look up the bucket that contains the specified key. If it exists
  /// in the map, return the bucket number of the key.  Otherwise return -1.
  /// This does not modify the map.
  AGT_nodiscard agt_i32_t find_key(std::string_view Key) const noexcept;

  /// remove_key - Remove the specified dictionary_entry from the table, but do not
  /// delete it.  This aborts if the value isn't in the table.
  void remove_key(dictionary_entry_base *V) noexcept;

  /// remove_key - Remove the dictionary_entry for the specified key from the
  /// table, returning it.  If the key is not in the table, this returns null.
  dictionary_entry_base *remove_key(std::string_view Key) noexcept;


  void pageTableDestroy(page_table* pt) noexcept;

}

#endif//AGATE_INTERNAL_CONTEXT_PAGE_HPP
