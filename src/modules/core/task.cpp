//
// Created by maxwe on 2024-10-14.
//

#include "task.hpp"

#include "core/ctx.hpp"
#include "core/scheduler.hpp"

namespace {
  agt_bool_t do_wake_timed(agt_ctx_t ctx, agt::task_waker* waker, agt_resume_reason_t reason) noexcept {
    const bool isTimedout = reason == AGT_RESUME_REASON_TIMED_OUT;

    AGT_assert(waker->flags & suspended_task_has_deadline);

    // deadline is strict and wait has timed out implies deadline has passed.
    AGT_assert( !(isTimedout && (waker->flags & suspended_task_strict_deadline)) || (waker->deadline < hwnow()) );

    // Only one party may successfully put it into 'waking' state.
    if (atomic_try_replace(waker->state, task_state::suspended, task_state::waking)) {
      auto entry = waker->queueEntry;

      // Relaxed because it's okay if we get false positives; it's just an optimization
      // that's much faster than trying to remove from the queue when it already has been.
      // This should be guaranteed not to give any false negatives.
      if (atomic_relaxed_load(entry->enqueued))
        waker->pfnQueueRemove(ctx, entry, waker->queue);
      // Don't care whether or not the queue removal fails honestly.

      // Skip calling resume if the skip_resume_on_timeout flag is specified and its a timeout.
      // Hopefully this should be pretty obvious.
      if (!((waker->flags & suspended_task_skip_resume_on_timeout) && isTimedout)) {
        waker->pfnResumeTask(ctx, &waker->suspendedTask, reason);
      }

      return AGT_TRUE;
    }

    return AGT_FALSE;
  }

  agt_bool_t do_wake_indefinite(agt_ctx_t ctx, agt::task_waker* waker, agt_resume_reason_t reason) noexcept {
    if (waker->flags & suspended_task_allocated_waker) { // If not allocated, then there was only a single ref to begin with.
      // This is the only possible case where there might be any contention,
      if (!atomic_try_replace(waker->state, task_state::suspended, task_state::waking)) {
        // Uh oh, something already woke :(
        return AGT_FALSE;
      }
    }

    waker->pfnResumeTask(ctx, &waker->suspendedTask, reason);
    AGT_assert( !waker->queueEntry ); // I cannot think of any reason why this might have been in a queue. If we're doing an internal wake on an indefinite wait, then
    return AGT_TRUE;
  }

  agt_bool_t try_resolve_waker(agt_ctx_t ctx, agt_task_waker_t externalWaker, agt::task_waker*& waker, agt::async_task_waker*& asyncWaker) noexcept {
    asyncWaker = nullptr;
    waker      = nonnull_object_cast<task_waker>(externalWaker);

    if (!waker) {
      // If this path is taken, we KNOW
      //   a) there's a deadline
      //   b) the wake is unconditional, and therefore not guaranteed to be sequenced prior to task resuming...
      AGT_assert( nonnull_object_isa<async_task_waker>(externalWaker) );
      asyncWaker = object_cast<async_task_waker>(externalWaker);
      waker = actualize_weak_ref(asyncWaker->weakRef, asyncWaker->refEpoch);
      if (!waker) {
        // This means that the weak reference was no longer valid, so we drop it and release the holder.
        drop_weak_ref(asyncWaker->weakRef);
        release(asyncWaker); // Maybe recycle this in a better way?
        return AGT_FALSE;
      }
    }

    return AGT_TRUE;
  }
}

task_waker* agt::stack_allocated_task_waker::get_internal(agt_ctx_t ctx, bool hasDeadline, agt_u32_t refs) noexcept {

  bool requiresAllocation = false;

  ctx->sched->

  if (refs) {

  }
}

agt_task_waker_t agt::stack_allocated_task_waker::get_external(agt_ctx_t ctx) noexcept {
  agt_task_waker_t result;

  if (inlineWaker.refCount) {

    if (!refCountedWaker) {
      // If refCountedWaker is still 0, then we haven't allocated any internal
    }
    else {
      auto async = alloc<async_task_waker>(ctx);
      async->weakRef = refCountedWaker;
      async->refEpoch = take_weak_ref(refCountedWaker);
    }


  }
}

agt::stack_allocated_task_waker::~stack_allocated_task_waker() {
  if (inlineWaker.refCount)
    release(refCountedWaker);
}

/**
   * Call this only on an entry that has already been successfully removed from
   * its queue.
   * */
agt_bool_t agt::wake_task(agt_ctx_t ctx, task_queue_entry* entry, agt_resume_reason_t reason) noexcept {
  auto waker = entry->waker;
  agt_bool_t result = AGT_TRUE;
  const bool hasDeadline = entry->flags & suspended_task_has_deadline;

  AGT_assert( atomic_relaxed_load(entry->enqueued) == AGT_FALSE );

  if (hasDeadline &&
      (((entry->flags & suspended_task_strict_deadline) && waker->deadline < hwnow()) ||
       !atomic_try_replace(waker->state, task_state::suspended, task_state::waking))) {
    // atomic_store(entry->enqueued, AGT_FALSE); // We've already removed from the queue.

    // If there's a deadline, then there's the possibility of ordering issues.
    // Even though we know we're the only one who could have removed this from the queue,
    // we still need to guarantee that the user-called agt_wake_task does not impede.

    // deadline is strict, so if we've passed the deadline, fail.
    // Or, if deadline is not strict OR the deadline hasn't passed,
    // we try to set waker state to waking. If THAT fails, then we also
    // fail. If it succeeds however, we good.

    result = AGT_FALSE;
  }
  else {
    // If there's no deadline, then there's no ordering issues. This has already been
    // adequately ordered by virtue of having been removed from the queue.
    // As such, no other synchronization needs to happen.
    waker->pfnResumeTask(ctx, &waker->suspendedTask, reason);
  }

  if (entry->flags & suspended_task_allocated_waker)
    release(waker);

  // Just to be safe c:
  entry->waker = nullptr;

  return result;
}


agt_bool_t agt::wake_timed_suspend(agt_ctx_t ctx, agt_task_waker_t externalWaker, agt_resume_reason_t reason) noexcept {
  task_waker* waker;
  async_task_waker* asyncWaker;
  if (!try_resolve_waker(ctx, externalWaker, waker, asyncWaker))
    return AGT_FALSE;

  agt_bool_t result = do_wake_timed(ctx, waker, reason);

  if (asyncWaker) {
    release(waker);
    release(asyncWaker);
  }
  else if (waker->flags & suspended_task_allocated_waker) {
    release(waker);
  }

  return result;
}

agt_bool_t agt::wake_indefinite_suspend(agt_ctx_t ctx, agt_task_waker_t waker, agt_resume_reason_t reason) noexcept {
  auto pWaker = unsafe_object_cast<task_waker>(waker);
  auto result = do_wake_indefinite(ctx, pWaker, reason);

  // Generally unlikely that an indefinite wait might have an allocated waker, but it's possible,
  // if there were multiple possible internal wake sources.
  // We account for the possibility here. Question: Should we consider async internal wakers for indefinite waits?? NO. >:)
  if (atomic_relaxed_load(pWaker->flags) & suspended_task_allocated_waker)
    release(pWaker);
  return result;
}

agt_bool_t agt::wake(agt_ctx_t ctx, agt_task_waker_t externalWaker, agt_resume_reason_t reason) noexcept {
  task_waker* waker;
  async_task_waker* asyncWaker;
  if (!try_resolve_waker(ctx, externalWaker, waker, asyncWaker))
    return AGT_FALSE;

  agt_bool_t result;

  if (waker->flags & suspended_task_has_deadline)
    result = do_wake_timed(ctx, waker, reason);
  else
    result = do_wake_indefinite(ctx, waker, reason);

  if (asyncWaker) {
    release(waker);
    release(asyncWaker);
  }
  else if (waker->flags & suspended_task_allocated_waker) {
    release(waker);
  }

  return result;
}


namespace agt::api {

  agt_status_t AGT_stdcall new_sched_local(agt_ctx_t ctx, agt_sched_t* pSched, const agt_sched_desc_t* pSchedDesc) noexcept;
  void         AGT_stdcall close_sched_local(agt_ctx_t ctx, agt_sched_t sched) noexcept;
  agt_status_t AGT_stdcall bind_sched_local(agt_ctx_t ctx, agt_sched_t sched) noexcept;


  agt_bool_t   AGT_stdcall wake_task_local(agt_ctx_t ctx, agt_task_waker_t userWaker, agt_resume_reason_t reason) noexcept {
    AGT_assert( userWaker != nullptr );
    AGT_resolve_ctx(ctx);

    // WAKER HAS A DEADLINE.
    // THIS SHOULD BE GUARANTEED, AS agt_task_waker_t IS NOT MADE AVAILABLE TO THE USER OTHERWISE.

    task_waker* waker;
    async_task_waker* asyncWaker;
    if (!try_resolve_waker(ctx, userWaker, waker, asyncWaker))
      return AGT_FALSE;

    agt_bool_t result = do_wake_timed(ctx, waker, reason);

    if (asyncWaker) {
      release(waker);
      release(asyncWaker);
    }
    // else if (waker->flags & suspended_task_allocated_waker) {
    //   release(waker);
    // }

    return result;
  }


  agt_bool_t   AGT_stdcall ctx_has_same_sched_local(agt_ctx_t ctx, agt_ctx_t otherCtx) noexcept {
    AGT_unreachable;
    // AGT_ERROR_NOT_YET_IMPLEMENTED;
  }

}
