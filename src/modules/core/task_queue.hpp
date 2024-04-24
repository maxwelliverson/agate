//
// Created by maxwe on 2024-04-08.
//

#ifndef AGATE_INTERNAL_CORE_TASK_QUEUE_HPP
#define AGATE_INTERNAL_CORE_TASK_QUEUE_HPP

#include "config.hpp"

#include "core/impl/spinlock.hpp"

namespace agt {


  struct task_queue;
  struct locked_task_queue;


  inline static bool yield(agt_ctx_t ctx) noexcept;

  inline static void suspend(agt_ctx_t ctx) noexcept;

  inline static bool suspend_for(agt_ctx_t ctx, agt_timeout_t timeout) noexcept;



  struct blocked_task {
    blocked_task*  next;
    void        (* pfnResumeTask)(agt_ctx_t ctx, agt_ctxexec_t prevCtxExec, agt_utask_t task);
    agt_ctxexec_t  execCtxData;
    agt_utask_t    task;
  };



  /**
   * =============== [ Task Queues ] ===============
   *
   * Data structure that maintains a queue of tasks,
   *
   * Ops:
   *  - insert:  push a blocked task to the queue with a wake callback
   *  - remove:  remove a *specific* task from the queue (thus allowing for cancelable operations, like timed waits)
   *  - wakeOne: pops a task from the queue and invokes the wake callback it was inserted with.
   *  - wakeAll: clears the queue, and invokes the wake callback for each removed task.
   *
   *  The 'signal' task_queues have slightly different semantics regarding the
   *  ordering of wait and wake operations. With a normal task_queue, a queued
   *  task is only woken by a subsequent wait operation. With a signal task_queue,
   *  a wake count is maintained, such that if a 'wake' is invoked on an empty
   *  queue, the wake count is incremented. If a task is to be inserted into a
   *  queue with a non-zero wake count, the wake count is instead decremented,
   *  and the task is not blocked.
   *
   *  The following queue types are made available, and their semantics are
   *  described as such
   *
   * task_queue:
   *  - thread-safe
   *  - lock-free
   *  - wait-free
   *
   * unsafe_task_queue:
   *  - not thread-safe
   *
   * locked_task_queue:
   *  - thread-safe
   *  - uses a spinlock
   *
   * signal_task_queue:
   *  - thread-safe
   *  - lock-free
   *  - wait-free
   *  - signal semantics
   *
   * unsafe_signal_task_queue:
   *  - not thread-safe
   *  - signal semantics
   *
   * locked_signal_task_queue
   *  - thread-safe
   *  - uses a spinlock
   *  - signal semantics
   * */

  // If returns true, task has been unblocked
  // otherwise, task remains blocked.
  using task_queue_predicate_t = bool(*)(agt_ctx_t ctx, blocked_task* task, void* userData);



  struct unsafe_task_queue {
    impl::spinlock_t lock;
    int32_t          size;
    uintptr_t        value;
  };

  struct AGT_alignas(16) task_queue {
    agt_u16_t     activeRefCount; // needed to be able to
    agt_u16_t     epoch;
    agt_i32_t     size;
    blocked_task* task;
  };

  struct AGT_alignas(16) locked_task_queue : unsafe_task_queue { };

  struct unsafe_signal_task_queue : unsafe_task_queue { };

  struct signal_task_queue {
    agt_u32_t      index;
    agt_i32_t      size;
    blocked_task*  head;
    blocked_task** tail;
  };

  struct AGT_alignas(16) locked_signal_task_queue : unsafe_signal_task_queue { };


  struct signal_waiter {
    void              (* resume)(agt_ctx_t ctx, agt_ctxexec_t prevCtxExec, agt_utask_t task);
    agt_ctxexec_t        ctxexec;
    agt_utask_t          task;
  };

  struct spsc_signal_queue {
    uintptr_t value;
    agt_i32_t size;
  };

  struct spsc_signal_waiter_vec;


  static_assert(sizeof(unsafe_task_queue) == 16);
  static_assert(sizeof(task_queue) == 16);
  static_assert(sizeof(locked_task_queue) == 16);
  static_assert(sizeof(unsafe_signal_task_queue) == 16);
  static_assert(sizeof(signal_task_queue) == 24);
  static_assert(sizeof(locked_signal_task_queue) == 16);


  namespace impl {
    void task_queue_insert(agt_ctx_t ctx, unsafe_task_queue& queue, blocked_task& task) noexcept;

    bool task_queue_contains(const unsafe_task_queue& queue, const blocked_task& task) noexcept;
  }


  AGT_forceinline static void task_queue_init(unsafe_task_queue& queue) noexcept {
    queue.lock.value = impl::SpinlockUnlockedValue;
    queue.size  = 0;
    queue.value = 0;
  }

  AGT_forceinline static void task_queue_init(task_queue& queue) noexcept {
    queue.epoch = 0;
    queue.activeRefCount = 0;
    queue.size = 0;
    queue.task = nullptr;
  }

  AGT_forceinline static void task_queue_init(spsc_signal_queue& queue) noexcept {
    queue.size    = 0;
    queue.value   = 0;
  }

  void blocked_task_init(agt_ctx_t ctx, blocked_task& task) noexcept;



  inline static void insert(agt_ctx_t ctx, unsafe_task_queue& queue, blocked_task& task) noexcept {
    impl::task_queue_insert(ctx, queue, task);
    ++queue.size;
  }

  bool remove(unsafe_task_queue& queue, blocked_task& task) noexcept;

  void wake_one(agt_ctx_t ctx, unsafe_task_queue& queue) noexcept;

  void wake_all(agt_ctx_t ctx, unsafe_task_queue& queue) noexcept;

  bool wake_one(agt_ctx_t ctx, unsafe_task_queue& queue, task_queue_predicate_t predicate, void* userData) noexcept;

  bool wake_all(agt_ctx_t ctx, unsafe_task_queue& queue, task_queue_predicate_t predicate, void* userData) noexcept;



  void insert(agt_ctx_t ctx, task_queue& queue, blocked_task& task) noexcept;

  bool remove(task_queue& queue, blocked_task& task) noexcept;

  void wake_one(agt_ctx_t ctx, task_queue& queue) noexcept;

  void wake_all(agt_ctx_t ctx, task_queue& queue) noexcept;

  bool wake_one(agt_ctx_t ctx, task_queue& queue, task_queue_predicate_t predicate, void* userData) noexcept;

  bool wake_all(agt_ctx_t ctx, task_queue& queue, task_queue_predicate_t predicate, void* userData) noexcept;



  inline static void insert(agt_ctx_t ctx, locked_task_queue& queue, blocked_task& task) noexcept {
    lock(ctx, queue.lock);
    insert(ctx, static_cast<unsafe_task_queue&>(queue), task);
    unlock(queue.lock);
  }

  inline static bool remove(agt_ctx_t ctx, locked_task_queue& queue, blocked_task& task) noexcept {
    lock(ctx, queue.lock);
    bool result = remove(static_cast<unsafe_task_queue&>(queue), task);
    unlock(queue.lock);
    return result;
  }

  inline static bool wake_one(agt_ctx_t ctx, locked_task_queue& queue) noexcept {
    lock(ctx, queue.lock);
    wake_one(ctx, static_cast<unsafe_task_queue&>(queue));
    bool result = queue.size == 0;
    unlock(queue.lock);
    return result;
  }

  inline static void wake_all(agt_ctx_t ctx, locked_task_queue& queue) noexcept {
    lock(ctx, queue.lock);
    wake_all(ctx, static_cast<unsafe_task_queue&>(queue));
    unlock(queue.lock);
  }

  inline static bool wake_one(agt_ctx_t ctx, locked_task_queue& queue, task_queue_predicate_t predicate, void* userData) noexcept {
    lock(ctx, queue.lock);
    bool result = wake_one(ctx, static_cast<unsafe_task_queue&>(queue), predicate, userData);
    unlock(queue.lock);
    return result;
  }

  inline static bool wake_all(agt_ctx_t ctx, locked_task_queue& queue, task_queue_predicate_t predicate, void* userData) noexcept {
    lock(ctx, queue.lock);
    bool result = wake_all(ctx, static_cast<unsafe_task_queue&>(queue), predicate, userData);
    unlock(queue.lock);
    return result;
  }


  // If returns true, task was successfully blocked.
  // If returns false, then task was immediately woken by a postponed wake
  inline static bool insert(agt_ctx_t ctx, unsafe_signal_task_queue& queue, blocked_task& task) noexcept {
    bool result = false;

    if (queue.size >= 0) {
      impl::task_queue_insert(ctx, queue, task);
      result = true;
    }
    ++queue.size;
    return result;
  }

  // the signalling mechanism has no impact on task removal, so calling remove on an unsafe_signal_task_queue will simply dispatch by supertype to the right function.
  // bool remove(unsafe_signal_task_queue& queue, blocked_task& task) noexcept;

  void wake(agt_ctx_t ctx, unsafe_signal_task_queue& queue, uint32_t n) noexcept;


  // If returns true, task was successfully blocked.
  // If returns false, then task was immediately woken by a postponed wake
  bool insert(signal_task_queue& queue, blocked_task& task) noexcept;

  bool remove(signal_task_queue& queue, blocked_task& task) noexcept;

  void wake(signal_task_queue& queue, int32_t n) noexcept;



  // If returns true, task was successfully blocked.
  // If returns false, then task was immediately woken by a postponed wake
  inline static bool insert(agt_ctx_t ctx, locked_signal_task_queue& queue, blocked_task& task) noexcept {
    lock(ctx, queue.lock);
    bool result = insert(ctx, static_cast<unsafe_signal_task_queue&>(queue), task);
    unlock(queue.lock);
    return result;
  }

  inline static bool remove(agt_ctx_t ctx, locked_signal_task_queue& queue, blocked_task& task) noexcept {
    lock(ctx, queue.lock);
    bool result = remove(static_cast<unsafe_signal_task_queue&>(queue), task);
    unlock(queue.lock);
    return result;
  }

  inline static void wake(agt_ctx_t ctx, locked_signal_task_queue& queue, int32_t n) noexcept {
    lock(ctx, queue.lock);

    wake(ctx, static_cast<unsafe_signal_task_queue&>(queue), n);
    unlock(queue.lock);
  }



  inline static bool wait_for(agt_ctx_t ctx, unsafe_signal_task_queue& queue, agt_timeout_t timeout) noexcept {

    if (timeout == AGT_DO_NOT_WAIT) {
      if (queue.size < 0) {
        ++queue.size;
        return true;
      }
      return false;
    }

    ++queue.size;
    if (queue.size > 0) {

      blocked_task task;
      blocked_task_init(ctx, task);
      impl::task_queue_insert(ctx, queue, task);

      if (timeout != AGT_WAIT) {
        if (!suspend_for(ctx, timeout) && remove(queue, task))
          return false;
      }
      else
        suspend(ctx);

      AGT_assert( !impl::task_queue_contains(queue, task) );
    }

    return true;
  }



  void signal(agt_ctx_t ctx, spsc_signal_queue& queue, int32_t n) noexcept;

  bool wait(agt_ctx_t ctx, spsc_signal_queue& queue, agt_timeout_t timeout) noexcept;


}

#endif//AGATE_INTERNAL_CORE_TASK_QUEUE_HPP
