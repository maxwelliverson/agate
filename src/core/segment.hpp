//
// Created by maxwe on 2022-03-08.
//

#ifndef AGATE_INTERNAL_CONTEXT_PAGE_HPP
#define AGATE_INTERNAL_CONTEXT_PAGE_HPP

#include "config.hpp"

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


  void         seg_manager_init(seg_manager* sm, size_t tableBuckets, rng_seeder* seeder) noexcept;



  agt_status_t seg_manager_lookup(seg_manager* sm, agt_u32_t id, seg_descriptor** pDescriptor) noexcept;

  agt_status_t seg_manager_alloc(seg_manager* sm, seg_type type, size_t size, seg_descriptor** pDescriptor) noexcept;

  void         seg_manager_destroy(seg_manager* sm) noexcept;

}

#endif//AGATE_INTERNAL_CONTEXT_PAGE_HPP
