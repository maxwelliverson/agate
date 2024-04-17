//
// Created by maxwe on 2024-04-01.
//

#ifndef AGATE_BLOCKED_QUEUE_HPP
#define AGATE_BLOCKED_QUEUE_HPP

#include "config.hpp"

#include "spinlock.hpp"

namespace agt::impl {
  struct blocked_queue {
    spinlock_t spinlock;
    uint32_t   size;
    uintptr_t  value;
  };

  enum class block_kind {
    eThread,
    eFiber,
    eParallelFiber
  };

  struct local_event_executor;
  struct local_event_eagent;

  // This should always be allocated on the stack in some capacity.
  struct blocked_exec {
    blocked_queue*  queue;
    agt_executor_t  exec;
    agt_self_t      self;
  };

  AGT_forceinline void blocked_queue_init(blocked_queue& queue) noexcept {
    queue.value = 0;
    queue.spinlock.value = SpinlockUnlockedValue;
    queue.size = 0;
  }




  /*
   * Ignore the following, it's no longer relevant, but I'm
   * keeping it around to use as a template for describing
   * other synchronization problems.
   *
   * Used by monitor_list implementation, decrements monitorRefCount
   * once the queue lock has been acquired. Otherwise the exact same
   * as blocked_queue_insert(ctx, queue, blockedExec).
   *
   * This resolves an important problem, as described here.
   * Given two independent executors, consider the following
   * two possible execution sequences:
   *
   * ===================== [ CASE 1 ] =====================
   *
   *         Exec #1:                      Exec #2:
   * ------------------------------------------------------
   *  ++monitorRefCount (= 1)
   *  insert(queue, exec)
   *                                ++monitorRefCount (= 2)
   *                                resolve(queue) (= true)
   *                                --monitorRefCount (= 1)
   *  --monitorRefCount (= 0)
   * ======================================================
   *
   * ===================== [ CASE 2 ] =====================
   *
   *         Exec #1:                      Exec #2:
   * ------------------------------------------------------
   *  ++monitorRefCount (= 1)
   *  insert(queue, exec)
   *  --monitorRefCount (= 0)
   *                                ++monitorRefCount (= 1)
   *                                resolve(queue) (= true)
   *                                --monitorRefCount (= 0)
   * ======================================================
   *
   * ===================== [ CASE 3 ] =====================
   *
   *         Exec #1:                      Exec #2:
   * ------------------------------------------------------
   *  ++monitorRefCount (= 1)
   *  insert(queue, exec)
   *  --monitorRefCount (= 0)
   *                                ++monitorRefCount (= 1)
   *                                resolve(queue) (= true)
   *  ++monitorRefCount (= 2)
   *  insert(queue, exec2)
   *  --monitorRefCount (= 1)
   *                                --monitorRefCount (= 0)
   * ======================================================
   *
   *
   * The conditions where the monitor would be released are when
   * monitorRefCount is dropped to 0 *while* the queue is empty.
   *
   *
   * In CASE 1, Exec #1 is responsible for releasing the monitor,
   * and in CASE 2, even though Exec #1 does drop the reference
   * count to 0, it does so while the queue is not empty, whereas
   * when Exec #2 drops it to 0, the queue *is* empty. Therefore,
   * Exec #2 is responsible for releasing the monitor.
   *
   * An ambiguity arises from the fact that from the perspective
   * of Exec #1, CASE 1 is indistinguishable from CASE 2.
   *
   * In addition, from a semantic standpoint, we'd ideally like for
   * only the parties responsible for removing entries from the queue
   * to be potentially responsible for releasing the monitor.
   *
   * This problem is solved by having monitorRefCount be decremented
   * as part of the insert operation, while the queue lock is held.
   * This makes CASE 1 impossible.
   * */

  void unsafe_blocked_queue_insert(agt_ctx_t ctx, blocked_queue& queue, blocked_exec* blockedExec) noexcept;

  // Returns true if successful,
  // newSize is assigned the value of the size of the blocked_queue following the operation.
  bool unsafe_unblock_local(blocked_exec* blockedExec) noexcept;

  // Unblock unconditionally. Queue is necessarily cleared following this call.
  void unsafe_blocked_queue_resolve(agt_ctx_t ctx, blocked_queue& queue) noexcept;

  // Unblock unconditionally.
  void unsafe_blocked_queue_resolve_one(agt_ctx_t ctx, blocked_queue& queue) noexcept;

  // Unblock only those execs for which the callback returns true
  bool unsafe_blocked_queue_resolve(agt_ctx_t ctx, blocked_queue& queue, bool(* unblockPredicate)(blocked_exec* exec, void* userData), void* userData) noexcept;

  // Unblock only those execs for which the callback returns true
  bool unsafe_blocked_queue_resolve_one(agt_ctx_t ctx, blocked_queue& queue, bool(* unblockPredicate)(blocked_exec* exec, void* userData), void* userData) noexcept;




  inline static void blocked_queue_insert(agt_ctx_t ctx, blocked_queue& queue, blocked_exec* blockedExec) noexcept {
    lock(queue.spinlock);
    unsafe_blocked_queue_insert(ctx, queue, blockedExec);
    unlock(queue.spinlock);
  }

  inline static bool unblock_local(blocked_exec* blockedExec, uint32_t& newSize) noexcept {
    auto& queue = *blockedExec->queue;
    lock(queue.spinlock);
    auto result = unsafe_unblock_local(blockedExec);
    newSize = queue.size; // take it while the lock is held
    unlock(queue.spinlock);
    return result;
  }

  inline static void blocked_queue_resolve(agt_ctx_t ctx, blocked_queue& queue) noexcept {
    lock(queue.spinlock);
    unsafe_blocked_queue_resolve(ctx, queue);
    unlock(queue.spinlock);
  }

  /// Returns true if the queue is now empty
  inline static bool blocked_queue_resolve_one(agt_ctx_t ctx, blocked_queue& queue) noexcept {
    lock(queue.spinlock);
    unsafe_blocked_queue_resolve_one(ctx, queue);
    bool isEmpty = queue.size == 0;
    unlock(queue.spinlock);
    return isEmpty;
  }

  /// Calls unblockCallback on every queue entry, which returns true if it successfully unblocks.
  /// If unblockCallback fails to unblock and returns false, then that entry is requeued.
  /// Returns true if queue was fully cleared.
  inline static bool blocked_queue_resolve(agt_ctx_t ctx, blocked_queue& queue, bool(* unblockCallback)(blocked_exec* exec, void* userData), void* userData) noexcept {
    lock(queue.spinlock);
    auto result = unsafe_blocked_queue_resolve(ctx, queue, unblockCallback, userData);
    unlock(queue.spinlock);
    return result;
  }

  /// @returns true if queue is now empty
  inline static bool blocked_queue_resolve_one(agt_ctx_t ctx, blocked_queue& queue, bool(* unblockCallback)(blocked_exec* exec, void* userData), void* userData) noexcept {
    lock(queue.spinlock);
    auto result = unsafe_blocked_queue_resolve_one(ctx, queue, unblockCallback, userData);
    unlock(queue.spinlock);
    return result;
  }
}

#endif//AGATE_BLOCKED_QUEUE_HPP
