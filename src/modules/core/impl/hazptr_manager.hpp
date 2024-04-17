//
// Created by maxwe on 2024-04-15.
//

#ifndef AGATE_INTERNAL_CORE_IMPL_HAZPTR_MANAGER_HPP
#define AGATE_INTERNAL_CORE_IMPL_HAZPTR_MANAGER_HPP

#include "config.hpp"
#include "agate/atomic.hpp"

namespace agt {
  struct hazptr_manager {
    agt_hazptr_t hazptrs;
    uintptr_t    availList;
    uint64_t     syncTime;
    int          hazptrCount;
    agt_u16_t    bulkReclaimCount;
    bool         isShuttingDown;
    int          count;
    hwtime       dueTime;
    int          minReclaimThreshold;
    int          thresholdMultiplier;
    uint64_t     dueTimeSyncPeriod;   // 2s -> to tsc offset

    RetiredList untagged_[kNumShards];
    RetiredList tagged_[kNumShards];
    Atom<int> count_{0};
    Atom<uint64_t> due_time_{0};
    Atom<ExecFn> exec_fn_{nullptr};
    Atom<int> exec_backlog_{0};
  };

  struct AGT_cache_aligned hazptr_ctx_cache {
    agt_hazptr_t hazptrs[7];
    uint32_t     count;
  };


  agt_hazptr_t create_new_hazptr(agt_ctx_t ctx, hazptr_manager& manager) noexcept;

  agt_hazptr_t acquire_hazptrs(agt_ctx_t ctx, hazptr_manager& manager, uint32_t count) noexcept;

  void         release_hazptrs(agt_ctx_t ctx, hazptr_manager& manager, agt_hazptr_t head, agt_hazptr_t tail) noexcept;

  inline static void         release_hazptr(agt_ctx_t ctx, hazptr_manager& manager, agt_hazptr_t hazptr) noexcept {
    release_hazptrs(ctx, manager, hazptr, hazptr);
  }

  void         try_reclaim_hazptrs(agt_ctx_t ctx, hazptr_manager& manager) noexcept;
}

#endif//AGATE_INTERNAL_CORE_IMPL_HAZPTR_MANAGER_HPP
