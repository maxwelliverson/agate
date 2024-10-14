//
// Created by maxwe on 2024-10-13.
//

#ifndef AGATE_CORE_SCHEDULER_HPP
#define AGATE_CORE_SCHEDULER_HPP

#include "config.hpp"

#include "agate/atomic.hpp"
#include "agate/set.hpp"

#include "core/object.hpp"
#include "core/ref.hpp"

AGT_api_object(agt_sched_st, sched_entity, ref_counted) {
  agt::sched_instance* multiThreadedSched;
};

namespace agt {
  
  using task_state   = agt_u64_t;
  using suspend_id_t = agt_u64_t;
  
  //     READY  => 0
  //   RUNNING  => 1
  // SUSPENDING => 3????
  // SUSPENDED  => 2

  
  
  AGT_forceinline suspend_id_t get_suspend_id(const task_state& state) {
    auto value = atomic_load(state);
    AGT_invariant( (value & 0x3ULL) == 1 ); // state is RUNNING
    return value;
  }

  // Returns
  //   - AGT_TRUE:  The task was sucessfully suspended.
  //   - AGT_FALSE: The task has been woken up before suspension finished.
  AGT_forceinline agt_bool_t   try_suspend_task(task_state& state, suspend_id_t key) {
    // We assume state is RUNNING, but it's possible that it's already READY
    // This is a simple check, because if it's already READY, the epoch will have changed.
    AGT_invariant( (key & 0x3ULL) == 1 ); // key should always be set to RUNNING

    // +0 for epoch increment, +1 for running --> suspended
    return atomic_try_replace(state, key, key + 1);
  }
  
  // Returns 
  //   - AGT_SUCCESS:  Task had already been woken up, and now is set to running
  //   - AGT_WOKEN_UP: Task had not already been woken up, but this call woke it up and set it to running.
  //   - AGT_NOT_READY: Task had not yet finished suspending, and as such, is not yet ready to restart (This should almost never happen, but it is possible).
  //   - AGT_ERROR_ALREADY_COMPLETED: Task had already resumed from the wait in question; this call did nothing.

  // NOTE: It is *maybe* possible for this call to try to go from running --> running IF another thread picks up
  //       and tries to resume the task BEFORE suspension completes. For now, we don't check for this.
  // TODO: Resolve the above issue. Maybe by disallowing other threads from directly resuming from within a call to 'resume'??
  AGT_forceinline agt_status_t try_resume_task(task_state& state,  suspend_id_t key) {
    AGT_invariant( (key & 0x3ULL) == 1 ); // key is always RUNNING
    suspend_id_t suspendedKey = key + 1;  // +0 for epoch increment, +1 for running --> suspended
    suspend_id_t wokenKey     = key + 3;  // +4 for epoch increment, -1 for running --> ready
    suspend_id_t newKey       = key + 4;  // +4 for epoch increment, +0 for running --> running
    suspend_id_t oldKey       = wokenKey; // Expect most common case to be that task was already woken.
    
    agt_status_t status       = AGT_SUCCESS;  // most common case is success.
    while(!atomic_cas(state, oldKey, newKey)) {
      if (oldKey == suspendedKey)  // Second most common case; this still succeeds at resuming, but also was responsible for waking.
        status = AGT_WOKEN_UP;
      else if (oldKey == key)  // This case occurs when the first CAS failed spuriously, OR failed due to the task having not yet been woken, but in between the first and second CAS, it WAS woken up. In that case, we reset and retry.
        return AGT_NOT_READY;
      else if (oldKey > newKey)
        return AGT_ERROR_ALREADY_COMPLETED; // This can be a simple comparison, because a) key is monotonically increasing, b) if there's a rollover, both oldKey and newKey will have rolled over, so ordering remains consistent.
      else {
        AGT_assert( oldKey == wokenKey ); // This should be the only other possible case (and also the rarest)
        status = AGT_SUCCESS;
      }
    }

    return status;
  }

  // Returns:
  //   - AGT_TRUE:  Task was successfully woken by this call ( running/suspended --> ready )
  //   - AGT_FALSE: Task had already been woken from this particular suspension; this call did nothing.
  AGT_forceinline agt_bool_t   try_wake_task(task_state& state,    suspend_id_t key) {
    AGT_invariant( (key & 0x3ULL) == 1 ); // key is always RUNNING
    suspend_id_t newKey       = key + 3;  // +4 for epoch increment; -1 for running --> ready
    suspend_id_t suspendedKey = key + 1;  // +0 for epoch increment; +1 for running --> suspended
    suspend_id_t oldKey       = suspendedKey;

    while(!atomic_cas(state, oldKey, newKey)) {
      // key is no longer valid; ALAS
      // NOTE: Comparing the differences is a way of dealing with this
      //       that is safe from rollovers; if a number of suspensions
      //       have happened in the interim, and oldKey has rolled over,
      //       then comparing oldKey > newKey would return false, but
      //       (oldKey - key) should always return a negative number,
      //       which, because the IDs are stored as unsigned integers and
      //       negatives are stored in twos compliment, the "negative"
      //       numbers will be interpreted as numbers in the upper half
      //       of the integer type's domain, which will invariably be
      //       larger than 3.
      if ((oldKey - key) > (newKey - key))
        return AGT_FALSE;
      AGT_assert( oldKey == suspendedKey || oldKey == key );
    }

    return AGT_TRUE;
  }
  
  
  
  

  AGT_abstract_object(sched_instance, ref_counted) {
    ref<agt_sched_st> entity;
    agt_u64_t         schedEpoch; // This is used to get unique identifiers for suspend/wait operations.
  };

  AGT_object(single_thread_sched, extends(sched_instance)) {
    agt_ctx_t ctx;
  };

  AGT_object(multi_thread_sched, extends(sched_instance)) {
    set<agt_ctx_t> ctxSet;
  };
}

#endif//AGATE_CORE_SCHEDULER_HPP
