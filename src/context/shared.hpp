//
// Created by maxwe on 2022-03-14.
//

#ifndef AGATE_INTERNAL_CONTEXT_SHARED_HPP
#define AGATE_INTERNAL_CONTEXT_SHARED_HPP

#include "fwd.hpp"

namespace agt {

  struct shared_cb;
  struct shared_registry;
  struct shared_processes;
  struct shared_allocator;
  struct shared_segment_descriptor;
  struct shared_allocation_descriptor;
  struct process_descriptor;



  struct shared_allocation_lut {
    shared_allocation_descriptor** allocTable;
    size_t allocBucketCount;
    size_t allocEntryCount;
    size_t allocTombstoneCount;
    shared_segment_descriptor** segTable;
    size_t segBucketCount;
    size_t segEntryCount;
    size_t segTombstoneCount;
  };

  struct shared_ctx {
    shared_cb*            cb;
    process_descriptor*   self;
    shared_registry*      registry;
    shared_processes*     processes;
    shared_allocator*     allocator;
    shared_allocation_lut allocations;
  };



  agt_status_t sharedControlBlockInit(shared_ctx* pSharedCtx) noexcept;


  void         sharedControlBlockClose(shared_ctx* pSharedCtx) noexcept;
}

#endif //AGATE_INTERNAL_CONTEXT_SHARED_HPP
