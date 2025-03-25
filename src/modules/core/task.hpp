//
// Created by maxwe on 2024-10-14.
//

#ifndef AGATE_CORE_TASK_HPP
#define AGATE_CORE_TASK_HPP

#include "config.hpp"

#include "agate/cast.hpp"
#include "agate/flags.hpp"

#include "core/object.hpp"
#include "core/time.hpp"

/**
 *
 * Some of the backend mechanisms will be different depending on if
 *
 *  Facts:
 *      Users only need to explicitly call
 *
 *  Task Queue:
 *      entry: objects stored in the queue with info for waking
 *
 *  Boolean state:
 *      A: Whether or not the suspension has a deadline
 *      B: If the sched was created with AGT_SCHED_ORDERED_WAKING
 *
 *  Lifetime of waker (SUSPEND):
 *
 *    (%01=task)
 *        |
 *    [enqueue]
 *        |
 *        |--[suspend]-->|
 *                       |
 *                  (task=other)
 *                       |                            ...
 *                      ...                            |
 *                                                     |
 *                                                 [dequeue]
 *                                                     |
 *                         <=-=-=-=-=[resume]-=-=-=-=-=|
 *
 *                      ...
 *                       |
 *                       |
 *                       |
 *        |<-------------|
 *        |
 *   (task=%01)
 *        |
 *        V
 *
 *
 * ==> A=true  and B=false
 *
 * The queue entry is removed when the task is woken, but the "async_waker" object might last
 * indefinitely. As such, task suspension state is stored in a seperate object. The queue entry
 * contains a strong reference to this object, while the async_waker holds a weak reference.
 *
 * When you try to wake by removing the queue entry, you simply drop the reference
 * once you've removed it from the queue.
 *
 *
 *   queue entry's lifetime might NOT outlast any of its references, so we must
 *      create some proxies to store extra state. In particular, a ref_counted
 *      'scaffolding' to store task suspension state. The async waiter then stores
 *      a weak reference to this scaffolding, while the queue entry stores a strong
 *      reference.
 *
 *
 *  pseudocode for sched without ordered waking
 *
 *      agt_wake_task(async_waker, reason):
 *          waker := resolveWeakRef(async_waker)
 *          if waker is null:
 *              return false
 *          returnValue := false
 *          atomic if waker.state is SUSPENDED:
 *              waker.state := WAKING
 *              if dequeue(entry) is successful or deadline is strict:
 *                  resume(waker.suspendedTask, reason)
 *                  atomic waker.state := READY
 *                  returnValue := true
 *          release(waker)
 *          return returnValue
 *
 *
 *
 *
 *
 *
 * ==> A=true  and B=true
 *   queue entry has lifetime guaranteed to outlive any of its references.
 *   a user call to 'wake' must TRY to remove from queue, then if successful,
 *   call resume.
 * ==> A=false and B=false
 *   queue entry has lifetime guaranteed to outlive any of its references.
 *   dequeue guaranteed before suspend returns
 * ==> A=false and B=true
 *   queue entry has lifetime guaranteed to outlive any of its references.
 *   dequeue guaranteed before suspend returns
 *
 *
 *
 *
 *
 * */


namespace agt {

  enum suspended_task_flags {
    suspended_task_has_deadline           = 0x1,
    suspended_task_strict_deadline        = 0x2,
    suspended_task_wake_is_ordered        = 0x10,
    suspended_task_skip_resume_on_timeout = 0x20,
    suspended_task_precise_timing         = 0x200,
    suspended_task_allocated_waker        = 0x8000,
  };

  struct task_queue_entry;

  enum class task_state : agt_u32_t {
    suspended,
    waking,
    ready,
  };

  // Only used for ordered wakers/indefinite suspensions.
  AGT_object(task_waker, ref_counted) {
    task_state           state;
    agt_suspended_task_t suspendedTask;
    hwtime               deadline; // Stored so that if deadline is strict, we can check against it.
    void              (* pfnResumeTask)(agt_ctx_t ctx, const agt_suspended_task_t* pSuspendedTask, agt_resume_reason_t reason);
    task_queue_entry*    queueEntry;
    agt_bool_t        (* pfnQueueRemove)(agt_ctx_t ctx, task_queue_entry* entry, void* queue);
    void*                queue;
  };

  AGT_object(async_task_waker) {
    agt_epoch_t refEpoch;
    task_waker* weakRef;
  };

  //
  /**
   * MUST be allocated as a local variable on the stack.
   *
   * This class handles
   * */
  class stack_allocated_task_waker{
    union {
      task_waker  inlineWaker;
      task_waker* refCountedWaker;
    };
    agt_task_waker_t externalWaker;
  public:

    stack_allocated_task_waker() {
      refCountedWaker      = nullptr;
      inlineWaker.refCount = 0;
      inlineWaker.epoch    = 0;
      externalWaker        = nullptr;
    }

    /**
     * If refs > 1,
     * */
    AGT_nodiscard task_waker*      get_internal(agt_ctx_t ctx, bool hasDeadline, agt_u32_t refs = 1) noexcept;

    AGT_nodiscard AGT_forceinline agt_task_waker_t get_external() const noexcept {
      return externalWaker;
    }

    ~stack_allocated_task_waker();
  };

}


// Can either actually be agt::async_task_waker or agt::task_waker.
// Needs to be implemented horizontally rather than by actual inheritance,
// cause task_waker and async_task_waker need different base classes...
AGT_abstract_api_object(agt_task_waker_st, task_waker) {};

AGT_enable_casting(agt_task_waker_st, task_waker);
AGT_enable_casting(task_waker, agt_task_waker_st);
AGT_enable_casting(agt_task_waker_st, async_task_waker);
AGT_enable_casting(async_task_waker, agt_task_waker_st);


namespace agt {



  // using task_state   = agt_u32_t;
  // using suspend_id_t = agt_u32_t;

  //     READY  => 0
  //   RUNNING  => 1
  // SUSPENDED  => 2
  //    WAKING  => 3




  agt_bool_t wake_task(agt_ctx_t ctx, task_queue_entry* entry, agt_resume_reason_t reason) noexcept;


  agt_bool_t wake_timed_suspend(agt_ctx_t ctx, agt_task_waker_t waker, agt_resume_reason_t reason) noexcept;

  agt_bool_t wake_indefinite_suspend(agt_ctx_t ctx, agt_task_waker_t waker, agt_resume_reason_t reason) noexcept;

  agt_bool_t wake(agt_ctx_t ctx, agt_task_waker_t waker, agt_resume_reason_t reason) noexcept;

}




#endif//AGATE_CORE_TASK_HPP
