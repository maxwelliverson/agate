//
// Created by maxwe on 2024-04-09.
//

#include "uexec.hpp"

#include "agate/cast.hpp"
#include "agate/time.hpp"


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


enum busy_state {
  UEXEC_STATE_RUNNING,
  UEXEC_STATE_WAITING,
  UEXEC_STATE_TIMED_OUT,
  UEXEC_STATE_WOKEN_UP,
};

AGT_object(default_uexec_task) {
  agt_ctx_t  ctx;
  busy_state state; // these are only ever updated by this thread, so they need not have atomic semantics (they do need to be signal safe though).
  agt_u32_t  stateEpoch;
  HANDLE     threadHandle;
  HANDLE     timer;
};

AGT_object(default_uexec_timeout_data) {
  agt_u32_t           epoch;
  default_uexec_task* task;
};

namespace {
  AGT_forceinline void enter_wait_state(default_uexec_task& task) noexcept {
    ++task.stateEpoch;
    task.state = UEXEC_STATE_WAITING;
    std::atomic_signal_fence(std::memory_order_release);
  }

  void AGT_stdcall wake_up_after_timeout(LPVOID arg, DWORD dwTimerLowValue, DWORD dwTimerHighValue) noexcept {
    (void)dwTimerLowValue;
    (void)dwTimerHighValue;
    auto data = static_cast<default_uexec_timeout_data*>(arg);
    const auto task = data->task;
    if (task->stateEpoch == data->epoch && task->state == UEXEC_STATE_WAITING)
      task->state = UEXEC_STATE_TIMED_OUT;
    release(data);
  }

  void AGT_stdcall wake_up_suspended(ULONG_PTR arg) noexcept {
    AGT_invariant( arg != 0 );

    auto& task = *reinterpret_cast<default_uexec_task*>(arg);
    std::atomic_signal_fence(std::memory_order_acquire);
    if (task.state == UEXEC_STATE_WAITING)
      task.state = UEXEC_STATE_WOKEN_UP;
  }


  inline constexpr static agt_timeout_t MininumLongTimeout = 20000; // 20 ms


  void yield_uexec(agt_ctx_t ctx, agt_ctxexec_t userCtxExec) noexcept {
    (void)ctx;
    (void)userCtxExec;
    SwitchToThread();
  }

  void suspend_uexec(agt_ctx_t ctx, agt_ctxexec_t userCtxExec) noexcept {
    (void)ctx;
    const auto pTask = static_cast<agt_utask_t*>(userCtxExec);
    auto& task = *unsafe_nonnull_object_cast<default_uexec_task>(*pTask);

    enter_wait_state(task);

    do {
      DWORD result = SleepEx(INFINITE, TRUE);
      AGT_invariant( result == WAIT_IO_COMPLETION );
      std::atomic_signal_fence(std::memory_order_acquire);
    } while(task.state == UEXEC_STATE_WAITING); // this is in case any *other* APC callbacks occur.

    AGT_invariant( task.state == UEXEC_STATE_WOKEN_UP );

    task.state = UEXEC_STATE_RUNNING;
  }

  /*agt_status_t fallback_suspend_for_uexec(agt_ctx_t ctx, agt_ctxexec_t userCtxExec, agt_timeout_t timeout) noexcept {
    (void)ctx;
    const auto pTask = static_cast<agt_utask_t*>(userCtxExec);
    auto& task = *unsafe_nonnull_object_cast<default_uexec_task>(*pTask);

    enter_wait_state(task);

    if (timeout < MininumLongTimeout) {
      auto timeoutData = alloc<default_uexec_timeout_data>(ctx);
      timeoutData->epoch = task.stateEpoch;
      timeoutData->task  = &task;
      LARGE_INTEGER liTimeout;
      liTimeout.QuadPart = -static_cast<LONGLONG>(timeout * 10);
      // If we use a callback with SetWaitableTimer, the callback will always go off, even if

      // Because this is a manual reset timer, setting the timer here will cancel any previously signals, so we don't have to worry about previous timed waits that we were woken from timing out and waking us early in this wait.
      BOOL success = SetWaitableTimerEx(task.timer, &liTimeout, 0, nullptr, nullptr, nullptr, 0);
      // BOOL success = SetWaitableTimer(self.timer, &liTimeout, 0, wake_up_after_timeout, timeoutData, FALSE);
      assert( success );

      do {
        DWORD result = SleepEx(INFINITE, TRUE);
        AGT_invariant( result == WAIT_IO_COMPLETION );
        std::atomic_signal_fence(std::memory_order_acquire);
      } while(self.state == AGT_BUSY_STATE_WAITING);
}
}
*/
  agt_status_t high_precision_suspend_for_uexec(agt_ctx_t ctx, agt_ctxexec_t userCtxExec, agt_timeout_t timeout) noexcept {
    (void)ctx;
    const auto pTask = static_cast<agt_utask_t*>(userCtxExec);
    auto& task = *unsafe_nonnull_object_cast<default_uexec_task>(*pTask);

    constexpr static agt_timeout_t LongTimeoutThreshold = 1000;

    enter_wait_state(task);

    if (timeout >= MininumLongTimeout) {
      LARGE_INTEGER liTimeout;
      liTimeout.QuadPart = -static_cast<LONGLONG>(timeout * 10);
      // If we use a callback with SetWaitableTimer, the callback will always go off, even if

      // Because this is a manual reset timer, setting the timer here will cancel any previously signals, so we don't have to worry about previous timed waits that we were woken from timing out and waking us early in this wait.
      BOOL success = SetWaitableTimerEx(task.timer, &liTimeout, 0, nullptr, nullptr, nullptr, 0);
      assert( success );

      do {
        auto waitResult = WaitForSingleObjectEx(task.timer, INFINITE, TRUE);
        if (waitResult == WAIT_IO_COMPLETION) {
          if (task.state == UEXEC_STATE_WAITING) [[unlikely]]
            continue;
        }
        else if (waitResult == WAIT_OBJECT_0)
          task.state = UEXEC_STATE_TIMED_OUT;
        break;
      } while(true);
    }
    else {


      uint32_t backoff = 0;

      const auto deadline = agt::now() + (timeout * 10); // worry about this maybe?

      while (task.state == UEXEC_STATE_WAITING) {
        const auto currentTime = agt::get_stable_timestamp() + (timeout * 10);
        if (deadline <= currentTime) {
          task.state = UEXEC_STATE_TIMED_OUT;
          break;
        }
        DUFFS_MACHINE_EX(backoff,
                         if (SwitchToThread())
                             backoff = 0;
                         );
        std::atomic_signal_fence(std::memory_order_acquire);
      }
    }


    bool timedOut = task.state == UEXEC_STATE_TIMED_OUT;
    task.state = UEXEC_STATE_RUNNING;
    return timedOut ? AGT_TIMED_OUT : AGT_SUCCESS;
  }

  void fallback_wake_uexec(agt_ctx_t ctx, agt_ctxexec_t userCtxExec, agt_utask_t _task) noexcept {
    (void)ctx;
    (void)userCtxExec;
    auto& task = *unsafe_nonnull_object_cast<default_uexec_task>(_task);

    std::atomic_signal_fence(std::memory_order_acquire);

    if (task.state == UEXEC_STATE_WAITING) [[likely]] {
      auto result = QueueUserAPC(wake_up_suspended, task.threadHandle, reinterpret_cast<ULONG_PTR>(_task));
      if (!result) {
        // :(
        // ???
      }
    }
  }

  void high_precision_wake_uexec(agt_ctx_t ctx, agt_ctxexec_t userCtxExec, agt_utask_t _task) noexcept {
    (void)ctx;
    (void)userCtxExec;
    auto& task = *unsafe_nonnull_object_cast<default_uexec_task>(_task);

    std::atomic_signal_fence(std::memory_order_acquire);

    if (task.state == UEXEC_STATE_WAITING) [[likely]] {
      auto result = QueueUserAPC2(wake_up_suspended, task.threadHandle, reinterpret_cast<ULONG_PTR>(_task), QUEUE_USER_APC_FLAGS_SPECIAL_USER_APC);
      if (!result) {
        // :(
        // ???
      }
    }
  }
}

inline constexpr static agt_uexec_vtable_t DefaultUExecVTable = {

    .yield       = yield_uexec,

    .suspend     = suspend_uexec,

    .suspend_for = high_precision_suspend_for_uexec,

    .resume      = high_precision_wake_uexec
};

inline constexpr static agt_uexec_vtable_t HPAPCDefaultUExecVTable = { };

inline constexpr static agt_uexec_vtable_t HPTimerDefaultUExecVTable = { };

inline constexpr static agt_uexec_vtable_t HPTimerAndAPCDefaultUExecVTable = { };



void agt::bind_default_uexec(agt_ctx_t ctx) noexcept {
  auto task = alloc<default_uexec_task>(ctx);
  ctx->uexecVPtr = &DefaultUExecVTable;
  ctx->uexec = nullptr;
  ctx->ctxexec = &ctx->task;
  ctx->task = task;
  ctx->flags &= ~CTX_HAS_USER_UEXEC;
  task->ctx = ctx;
  task->state = UEXEC_STATE_RUNNING;
  task->stateEpoch = 0;
  task->threadHandle = OpenThread(SYNCHRONIZE, FALSE, GetCurrentThreadId());
  task->timer = CreateWaitableTimerExA(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION | CREATE_WAITABLE_TIMER_MANUAL_RESET, TIMER_ALL_ACCESS);
}

void agt::unbind_default_uexec(agt_ctx_t ctx) noexcept {
  auto task = unsafe_nonnull_object_cast<default_uexec_task>(ctx->task);
  CloseHandle(task->threadHandle);
  CloseHandle(task->timer);
  release(task);
  ctx->task = nullptr;
}

namespace agt {

  agt_status_t AGT_stdcall new_user_uexec(agt_ctx_t ctx, agt_uexec_t* pExec, const agt_uexec_desc_t* pExecDesc) noexcept {
    if (!pExec || !pExecDesc || !pExecDesc->vtable)
      return AGT_ERROR_INVALID_ARGUMENT;

    auto vptr = pExecDesc->vtable;

    if (!vptr->yield || !vptr->suspend || !vptr->suspend_for || !vptr->resume)
      return AGT_ERROR_INVALID_ARGUMENT;

    AGT_try_resolve_ctx(ctx);

    auto&& desc = *pExecDesc;

    auto uexec = alloc<agt_uexec_st>(ctx);
    if (!uexec)
      return AGT_ERROR_BAD_ALLOC;

    uexec->refCount = 1;
    uexec->flags    = desc.flags;
    uexec->vtable   = desc.vtable;

    *pExec = uexec;

    return AGT_SUCCESS;
  }

  void         AGT_stdcall destroy_user_uexec(agt_ctx_t ctx, agt_uexec_t exec) noexcept {
    // AGT_resolve_ctx(ctx);
    (void)ctx;

    // down the line, perhaps we invoke 'destroy' callback. For now though, we simply
    release(exec);
    /*if (ctx->uexec == exec) {

    }*/
  }

  agt_status_t AGT_stdcall bind_uexec_to_ctx(agt_ctx_t ctx, agt_uexec_t exec, void* execCtxData) noexcept {
    AGT_resolve_ctx(ctx);

    const auto boundUExec = ctx->uexec;

    if (boundUExec == exec)
      return AGT_ERROR_ALREADY_BOUND; // This isn't strictly an error, but it is worth communicating that the passed ctxexec was not bound.

    if (!exec && execCtxData != nullptr) // If exec is null, then the default uexec is being bound, in which case execCtxData must be null.
      return AGT_ERROR_INVALID_ARGUMENT;


    if (boundUExec) {
      // TODO: Unbind uexec before binding a new one.
      // we must unbind the existing uexec
      // for now, this consists only of releasing the ref held by the uexec object.
      release(boundUExec);
    }
    else {
      unbind_default_uexec(ctx);
    }

    if (exec) {
      retain(exec);
      ctx->uexec     = exec;
      ctx->ctxexec   = execCtxData;
      ctx->uexecVPtr = exec->vtable;
      ctx->task      = nullptr;
      ctx->flags    |= CTX_HAS_USER_UEXEC;
    }
    else {
      bind_default_uexec(ctx);
    }


    return AGT_SUCCESS;
  }

}