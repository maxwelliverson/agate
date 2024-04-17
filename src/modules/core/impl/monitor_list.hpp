//
// Created by maxwe on 2024-04-01.
//

#ifndef AGATE_MONITOR_LIST_HPP
#define AGATE_MONITOR_LIST_HPP

#include "config.hpp"

#include "agate/atomic.hpp"

#include "core/task_queue.hpp"
#include "spinlock.hpp"
#include "object_defs.hpp"

namespace agt {

  // If returns true, then the address in question has an appropriate value.
  using address_predicate_t = bool(*)(agt_ctx_t ctx, const void* address, void* userData) noexcept;

  namespace impl {

    AGT_BITFLAG_ENUM(monitor_flags, uint32_t) {
        eIsShared = 0x1
    };

    AGT_BITFLAG_ENUM(monitor_waiter_flags, uint32_t) {
        eHasPredicateCallback = 0x1
    };

    inline constexpr static monitor_flags eMonitoredAddressIsShared    = monitor_flags::eIsShared;
    inline constexpr static monitor_waiter_flags eMonitorWaiterHasPredicateCallback = monitor_waiter_flags::eHasPredicateCallback;



    AGT_object(monitored_address, extends(object), aligned(16)) {
      monitor_flags       flags;
      uint32_t            activeRefCount;
      monitored_address*  next;
      monitored_address** prevNext;
      const void*         address;
      spinlock_t*         segmentLock;
      unsafe_task_queue   waiterQueue;
    };


    struct AGT_cache_aligned monitor_waiter : blocked_task {
      monitored_address*   address;
      monitor_waiter_flags flags;
      bool                 isEnqueued;
      union {
        struct {
          address_predicate_t predicate;
          void*               predicateUserData;
        };
        struct {
          size_t   valueSize;
          uint64_t cmpValue;
        };
      };
    };

    /*struct monitor_list_entry {
      monitor_list_entry* next;
      monitored_address
    };*/

    struct monitor_list {
      agt_u32_t           tableSize;
      agt_u32_t           segmentCount;
      agt_u64_t           segmentMask;
      spinlock_t*         segmentLocks;
      agt_u64_t           tableMask;
      monitored_address** table;
    };


    void init_monitor_list_local(agt_instance_t instance, monitor_list& list, size_t tableSize, uint32_t concurrencyFactor) noexcept;

    void destroy_monitor_list_local(agt_instance_t instance, monitor_list& list) noexcept;


    void init_monitor(agt_ctx_t ctx, monitor_waiter &waiter, address_predicate_t predicate, void* userData) noexcept;

    void init_monitor(agt_ctx_t ctx, monitor_waiter &waiter, size_t dataSize, agt_u64_t cmpValue) noexcept;


    void attach_monitor(agt_ctx_t ctx, monitor_list& list, const void* addr, monitor_waiter & exec) noexcept;

    // This should only be called by the caller than originally blocked exec with attach_monitor.
    // Intended to allow the caller to implement a timed out wait.
    // Returns true if *prior* to this call, exec was still blocked.
    // This allows the caller to ensure any subsequent actions are taken if and only if this call was responsible for unblocking exec. (if false is returned, we still know that exec is unblocked, but that some other caller was responsible, and thus the required actions should already have been taken).
    /// @post exec is unblocked
    bool remove_monitor(agt_ctx_t ctx, monitor_waiter & exec) noexcept;

    void wake_one_on_monitored_address(agt_ctx_t ctx, monitor_list& list, const void* addr) noexcept;

    void wake_all_on_monitored_address(agt_ctx_t ctx, monitor_list& list, const void* addr) noexcept;
  }



}

#endif//AGATE_MONITOR_LIST_HPP
