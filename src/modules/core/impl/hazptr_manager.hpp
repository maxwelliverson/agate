//
// Created by maxwe on 2024-04-15.
//

#ifndef AGATE_INTERNAL_CORE_IMPL_HAZPTR_MANAGER_HPP
#define AGATE_INTERNAL_CORE_IMPL_HAZPTR_MANAGER_HPP

#include "config.hpp"
#include "agate/atomic.hpp"

namespace agt {

  struct hazard_obj;

  namespace impl {
    inline constexpr static uint32_t HazptrManagerConcurrency = 8;

    struct atomic_hazptr_domain_list {
      uintptr_t head;
      agt_ctx_t lockOwnerCtx;
      int       reenterCount;
    };

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

      hazard_obj * retiredLists[impl::HazptrManagerConcurrency];
      impl::atomic_hazptr_domain_list domainList;
    };

    struct AGT_cache_aligned hazptr_ctx_cache {
      agt_hazptr_t hazptrs[7];
      uint32_t     count;
    };

    AGT_forceinline static void init_hazptr_manager(hazptr_manager& manager) noexcept {
      manager.hazptrs = nullptr;
      manager.availList = 0;
      manager.syncTime = 0;
      manager.hazptrCount = 0;
      manager.bulkReclaimCount = 0;
      manager.isShuttingDown = false;
      manager.count = 0;
      manager.dueTime = {};
      // TODO: Add attributes that allow for modification of these values via agt_config_t
      manager.minReclaimThreshold = 1000;
      manager.thresholdMultiplier = 2; // maybe make customizable
      manager.dueTimeSyncPeriod = 2000000000;

      for (auto&& list : manager.retiredLists)
        list = {};
      manager.domainList = {};
    }

    AGT_forceinline static void init_hazptr_ctx_cache(hazptr_ctx_cache& cache) noexcept {
      for (auto& entry : cache.hazptrs)
        entry = nullptr;
      cache.count = 0;
    }

    void destroy_hazptr_manager(agt_instance_t instance) noexcept;

    void destroy_hazptr_ctx_cache(agt_ctx_t ctx) noexcept;
  }
}

#endif//AGATE_INTERNAL_CORE_IMPL_HAZPTR_MANAGER_HPP
