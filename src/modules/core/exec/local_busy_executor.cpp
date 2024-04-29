//
// Created by Maxwell on 2024-03-10.
//


#include "agate/cast.hpp"

#include "core/exec.hpp"


#include <mutex>

#define NOMINMAX
#include <Windows.h>
#include <VersionHelpers.h>

enum busy_state {
  AGT_BUSY_STATE_RUNNING,
  AGT_BUSY_STATE_WAITING,
  AGT_BUSY_STATE_TIMED_WAIT,
  AGT_BUSY_STATE_TIMED_OUT,
  AGT_BUSY_STATE_WOKEN_UP,
};

using pfn_queue_user_apc_t = BOOL(*)(PAPCFUNC, HANDLE, ULONG_PTR, QUEUE_USER_APC_FLAGS); // This is so that, if available, we can use QueueUserAPC2

struct local_busy_executor;

AGT_object(local_busy_self, extends(agt_self_st)) { };

AGT_object(local_busy_executor_cbdata, extends(object), aligned(8)) {
  agt_u32_t epoch;
  local_busy_executor* exec;
};

struct state_t {
  agt_u32_t  epoch;
  busy_state state;
};

AGT_object(local_busy_executor, extends(agt_executor_st)) {
  agt_u32_t  stateEpoch;
  busy_state state; // these are only ever updated by this thread, so they need not have atomic semantics (they do need to be signal safe though).

  pfn_queue_user_apc_t queueUserAPC2;
  HANDLE wakeEvent;
  HANDLE     threadHandle;
  HANDLE     timer;
  local_busy_self self; // given there can only be one self at any given point, simply store it inline.
  agt_ctxexec_t ctxexec;
  agt_bool_t shouldWake;
  local_busy_executor_cbdata* pTimeoutData;
  local_busy_executor_cbdata* pWakeData;
  local_busy_executor_cbdata  timeoutData;
  local_busy_executor_cbdata  wakeData;
};


using pfn_create_executor_t = agt_status_t(*)(agt_ctx_t, local_busy_executor*& exec, const agt_busy_executor_desc_t& desc);



namespace {

  AGT_forceinline state_t& get_compound_state(local_busy_executor& exec) noexcept {
    return *reinterpret_cast<state_t*>(&exec.stateEpoch);
  }

  void AGT_stdcall timeout_busy_executor(LPVOID arg, DWORD dwTimerLowValue, DWORD dwTimerHighValue) noexcept {
    (void)dwTimerLowValue;
    (void)dwTimerHighValue;
    auto data = static_cast<local_busy_executor_cbdata*>(arg);
    auto exec = data->exec;

    std::atomic_signal_fence(std::memory_order_acquire);
    if (exec->stateEpoch == data->epoch && exec->state == AGT_BUSY_STATE_WAITING) {
      ++exec->stateEpoch;
      exec->state = AGT_BUSY_STATE_TIMED_OUT;
    }

    if (data == &exec->timeoutData)
      exec->pTimeoutData = data;
    else
      release(data);
    std::atomic_signal_fence(std::memory_order_release);
  }

  void AGT_stdcall wake_up_busy_executor(ULONG_PTR arg) noexcept {
    auto data = reinterpret_cast<local_busy_executor_cbdata*>(arg);
    auto exec = data->exec;

    std::atomic_signal_fence(std::memory_order_acquire);
    if (data->epoch == exec->stateEpoch /*&& exec->state == AGT_BUSY_STATE_WAITING*/) {
      ++exec->stateEpoch;
      exec->state = AGT_BUSY_STATE_WOKEN_UP;
      std::atomic_signal_fence(std::memory_order_release);
    }

    if (!atomic_try_replace(exec->pWakeData, nullptr, data)) [[unlikely]]
      release(data);
  }

  void AGT_stdcall wake_up_busy_executor_with_special_apc(ULONG_PTR arg) noexcept {
    auto data = reinterpret_cast<local_busy_executor_cbdata*>(arg);
    auto exec = data->exec;

    // If the epoch is still the same, then it hasn't yet timed out either, so we don't need to check that.
    if (data->epoch == exec->stateEpoch /*&& exec->state != AGT_BUSY_STATE_TIMED_OUT*/) {
      if (exec->state == AGT_BUSY_STATE_WAITING)
        SetEvent(exec->wakeEvent);
      ++exec->stateEpoch;
      exec->state = AGT_BUSY_STATE_WOKEN_UP;
      std::atomic_signal_fence(std::memory_order_release);
    }

    if (!atomic_try_replace(exec->pWakeData, nullptr, data)) [[unlikely]]
      release(data);
  }


  inline constexpr static agt_timeout_t HighPrecisionTimerResolution = 500000; // 500 us

  std::mutex g_busyExecutorMutex;

  struct {
    agt_uexec_vtable_t uexecVTable = {
        .yield       = nullptr,
        .suspend     = nullptr,
        .suspend_for = nullptr,
        .resume      = nullptr
    };

    pfn_queue_user_apc_t queueUserAPC2 = nullptr;
    pfn_create_executor_t createExecutor = nullptr;
  } g_busyExecutorVTables;


  agt_bool_t yield_busy_executor(agt_ctx_t ctx, agt_ctxexec_t ctxexec) noexcept {
    // auto& exec = *unsafe_nonnull_object_cast<local_busy_executor>(ctxexec);
    return SwitchToThread();
  }

  void suspend_busy_executor_with_special_apc(agt_ctx_t ctx, agt_ctxexec_t ctxexec) noexcept {
    auto& exec = *member_to_struct_cast<&local_busy_executor::stateEpoch>(static_cast<agt_u32_t*>(ctxexec));

    // Do this as an atomic operation, even though no other thread should modify this,
    // so that there can be no interuption between checking and setting
    if (atomic_try_replace(exec.state, AGT_BUSY_STATE_RUNNING, AGT_BUSY_STATE_WAITING)) [[likely]] {
      do {
        auto result = WaitForSingleObjectEx(exec.wakeEvent, INFINITE, TRUE);
        // DWORD result = SleepEx(INFINITE, TRUE);
        // AGT_invariant( result == WAIT_IO_COMPLETION );
        std::atomic_signal_fence(std::memory_order_acquire);
      } while(exec.state == AGT_BUSY_STATE_WAITING);
    }

    // ++exec.stateEpoch;
    AGT_assert(exec.state == AGT_BUSY_STATE_WOKEN_UP);
    exec.state = AGT_BUSY_STATE_RUNNING;
    std::atomic_signal_fence(std::memory_order_release);
  }

  void suspend_busy_executor_default(agt_ctx_t ctx, agt_ctxexec_t ctxexec) noexcept {
    auto& exec = *unsafe_nonnull_object_cast<local_busy_executor>(ctxexec);

    exec.state = AGT_BUSY_STATE_WAITING;
    std::atomic_signal_fence(std::memory_order_release);

    do {
      DWORD result = SleepEx(INFINITE, TRUE);
      AGT_invariant( result == WAIT_IO_COMPLETION );
      std::atomic_signal_fence(std::memory_order_acquire);
    } while(exec.state == AGT_BUSY_STATE_WAITING);

    AGT_invariant( exec.state == AGT_BUSY_STATE_WOKEN_UP );

    // ++exec.stateEpoch;
    exec.state = AGT_BUSY_STATE_RUNNING;
  }


  agt_bool_t suspend_busy_executor_with_special_apc_until(agt_ctx_t ctx, agt_ctxexec_t ctxexec, agt_timeout_t timeout) noexcept {
    AGT_assert( timeout != AGT_WAIT && timeout != AGT_DO_NOT_WAIT);
    auto& exec = *member_to_struct_cast<&local_busy_executor::stateEpoch>(static_cast<agt_u32_t*>(ctxexec));

    constexpr static DWORD TimerTimedOut = WAIT_OBJECT_0;
    constexpr static DWORD WakeEventIsSet = WAIT_OBJECT_0 + 1;

    const HANDLE waitObjects[] = {
        exec.timer,
        exec.wakeEvent
    };

    const auto startTime = hwnow();
    const auto deadline = add_duration(ctx, startTime, timeout);
    const auto systemStartTime = []{
      LARGE_INTEGER lrgInt;
      FILETIME filetime;
      GetSystemTimePreciseAsFileTime(&filetime);
      lrgInt.LowPart = filetime.dwLowDateTime;
      lrgInt.HighPart = filetime.dwHighDateTime;
      return lrgInt;
    }();
    agt_bool_t returnValue = AGT_FALSE;

    exec.state = AGT_BUSY_STATE_TIMED_WAIT;
    std::atomic_signal_fence(std::memory_order_release);

    if (timeout >= HighPrecisionTimerResolution) {
      LARGE_INTEGER sleepDueTime;
      const auto roundedSleepDuration = (timeout - (timeout % HighPrecisionTimerResolution)) / 100;
      sleepDueTime.QuadPart = systemStartTime.QuadPart + roundedSleepDuration;
      const auto timerDeadline = add_duration(ctx, startTime, roundedSleepDuration * 100);
      SetWaitableTimerEx(exec.timer, &sleepDueTime, 0, nullptr, nullptr, nullptr, 0);
      do {
        switch (WaitForMultipleObjectsEx(2, waitObjects, FALSE, INFINITE, TRUE)) {
          case TimerTimedOut:

          case WakeEventIsSet:
          case WAIT_IO_COMPLETION:

          AGT_no_default;
        }
        auto result = WaitForSingleObjectEx(exec.timer, INFINITE, TRUE);
        if (result == WAIT_OBJECT_0)
          break; // timed out :(
        if (result == WAIT_IO_COMPLETION) {
          std::atomic_signal_fence(std::memory_order_acquire);
          if (exec.state == AGT_BUSY_STATE_WOKEN_UP) {
            exec.state = AGT_BUSY_STATE_RUNNING;
            std::atomic_signal_fence(std::memory_order_release);
            return AGT_SUCCESS;
          }
        }
      } while(hwnow() < timerDeadline); // This is needed because it is *possible*, though unlikely, that this is woken prematurely.
    }


    uint32_t backoff = 0;
    while(hwnow() < deadline) {
      std::atomic_signal_fence(std::memory_order_acquire);
      // Note we don't need to do any extra stuff here, cause the special APC will interupt user code
      if (exec.state == AGT_BUSY_STATE_WOKEN_UP) {
        exec.state = AGT_BUSY_STATE_RUNNING;
        std::atomic_signal_fence(std::memory_order_release);
        return AGT_SUCCESS;
      }
      DUFFS_MACHINE(backoff);
    }



    state_t prevState, nextState;

    prevState = atomic_relaxed_load(get_compound_state(exec));
    nextState.state = AGT_BUSY_STATE_RUNNING;

    do {
      if (prevState.state == AGT_BUSY_STATE_WOKEN_UP) {
        returnValue = AGT_TRUE;
        nextState.epoch = prevState.epoch;
      }
      else {
        AGT_assert( prevState.state == AGT_BUSY_STATE_TIMED_WAIT );
        returnValue = AGT_FALSE;
        nextState.epoch = prevState.epoch + 1;
      }
    } while(!atomic_cas(get_compound_state(exec), prevState, nextState));

    return returnValue;
  }

  agt_bool_t suspend_busy_executor_default_until(agt_ctx_t ctx, agt_ctxexec_t ctxexec, agt_timeout_t timeout) noexcept {
    AGT_assert( timeout != AGT_WAIT && timeout != AGT_DO_NOT_WAIT);
    auto& exec = *member_to_struct_cast<&local_busy_executor::stateEpoch>(static_cast<agt_u32_t*>(ctxexec));

    const auto startTime = hwnow();
    const auto deadline = add_duration(ctx, startTime, timeout);
    const auto systemStartTime = []{
      LARGE_INTEGER lrgInt;
      FILETIME filetime;
      GetSystemTimePreciseAsFileTime(&filetime);
      lrgInt.LowPart = filetime.dwLowDateTime;
      lrgInt.HighPart = filetime.dwHighDateTime;
      return lrgInt;
    }();
    bool hasWoken = false;
    agt_bool_t returnValue = AGT_FALSE;
    uint32_t backoff = 0;
    LARGE_INTEGER sleepDueTime;

    exec.state = AGT_BUSY_STATE_TIMED_WAIT;
    atomic_store(exec.shouldWake, AGT_FALSE);


    const auto roundedSleepDuration = (timeout - (timeout % HighPrecisionTimerResolution)) / 100;
    sleepDueTime.QuadPart = systemStartTime.QuadPart + roundedSleepDuration;
    const auto timerDeadline = add_duration(ctx, startTime, roundedSleepDuration * 100);
    SetWaitableTimerEx(exec.timer, &sleepDueTime, 0, nullptr, nullptr, nullptr, 0);


    do {
      auto result = WaitForSingleObjectEx(exec.timer, INFINITE, TRUE);
      if (result == WAIT_OBJECT_0)
        goto next_epoch;

      if (result == WAIT_IO_COMPLETION) {
        std::atomic_signal_fence(std::memory_order_acquire);
        if (exec.state == AGT_BUSY_STATE_WOKEN_UP) {
          returnValue = AGT_TRUE;
          goto wake_self;
        }
      }
    } while(hwnow() < timerDeadline); // This is needed because it is *possible*, though unlikely, that this is woken prematurely.



    while(hwnow() < deadline) {

      if (atomic_relaxed_load(exec.shouldWake) == AGT_TRUE) {
        returnValue = AGT_TRUE;
        break;
      }

      DUFFS_MACHINE(backoff);
    }

  next_epoch:
    ++exec.stateEpoch;
  wake_self:
    exec.state = AGT_BUSY_STATE_RUNNING;
    atomic_store(exec.shouldWake, AGT_FALSE);

    return returnValue;
  }

  void resume_busy_executor_with_special_apc(agt_ctx_t ctx, agt_ctxexec_t ctxexec, agt_utask_t task){
    auto& exec = *member_to_struct_cast<&local_busy_executor::stateEpoch>(static_cast<agt_u32_t*>(ctxexec));
    auto epoch = std::bit_cast<state_t>(task).epoch;

    if (exec.stateEpoch == epoch) [[likely]] {
      // This could still yield some false positives, but that's okay.
      // Queueing a wakeful APC that will set off after a time out or
      // something isn't an error, but the queuing is a slower operation,
      // so we try to avoid where possible.

      auto data = atomic_exchange(exec.pWakeData, nullptr);
      if (!data) {
        data = alloc<local_busy_executor_cbdata>(ctx); // Again, this isn't an issue, just slower.
        data->exec = &exec;
      }
      data->epoch = epoch;

      if (atomic_load(exec.state) != AGT_BUSY_STATE_RUNNING) {
        auto result = exec.queueUserAPC2(wake_up_busy_executor_with_special_apc, exec.threadHandle, reinterpret_cast<ULONG_PTR>(data), QUEUE_USER_APC_FLAGS_SPECIAL_USER_APC);
        AGT_assert( result == TRUE );
      }
    }
  }

  void resume_busy_executor_default(agt_ctx_t ctx, agt_ctxexec_t ctxexec, agt_utask_t task){
    auto& exec = *member_to_struct_cast<&local_busy_executor::stateEpoch>(static_cast<agt_u32_t*>(ctxexec));
    auto epoch = std::bit_cast<state_t>(task).epoch;

    if (exec.stateEpoch == epoch) [[likely]] {
      // This could still yield some false positives, but that's okay.
      // Queueing a wakeful APC that will set off after a time out or
      // something isn't an error, but the queuing is a slower operation,
      // so we try to avoid where possible.

      auto data = atomic_exchange(exec.pWakeData, nullptr);
      if (!data) {
        data = alloc<local_busy_executor_cbdata>(ctx); // Again, this isn't an issue, just slower.
        data->exec = &exec;
      }
      data->epoch = epoch;

      const auto currentState = atomic_load(exec.state);

      if (currentState != AGT_BUSY_STATE_RUNNING) {
        auto err = QueueUserAPC(wake_up_busy_executor, exec.threadHandle, reinterpret_cast<ULONG_PTR>(ctxexec));
        AGT_assert( err == 0 );
        if (currentState == AGT_BUSY_STATE_TIMED_WAIT) {
          atomic_store(exec.shouldWake, AGT_TRUE);
        }
      }
    }
  }



  // cooollldddddd

  [[msvc::noinline]] void init_vtable(agt_ctx_t ctx) noexcept {
    std::lock_guard lock{g_busyExecutorMutex};
    std::atomic_signal_fence(std::memory_order_seq_cst); // ensure we check the actual memory location again
    if (g_busyExecutorVTables.uexecVTable.resume == nullptr) [[likely]] {
      auto kernelLib = GetModuleHandle("Kernel32");
      AGT_assert( kernelLib != nullptr );
      auto pQueueUserAPC2 = (pfn_queue_user_apc_t)GetProcAddress(kernelLib, "QueueUserAPC2");
      g_busyExecutorVTables.queueUserAPC2 = pQueueUserAPC2;
      bool hasQUAPC2 = pQueueUserAPC2 != nullptr;

      if (hasQUAPC2) {
        // test whether it is actually functional; it is possible for the exported function to simply be a stub that always fails
        std::atomic_flag flag;
        auto result = pQueueUserAPC2([](ULONG_PTR arg){
          reinterpret_cast<std::atomic_flag*>(arg)->test_and_set();
        }, GetCurrentThread(), reinterpret_cast<ULONG_PTR>(&flag), QUEUE_USER_APC_FLAGS_SPECIAL_USER_APC);
        if (!result || !flag.test())
          hasQUAPC2 = false;
      }
      auto& table = g_busyExecutorVTables.uexecVTable;
      table.yield = yield_busy_executor;

      if (hasQUAPC2) {
        table.suspend = suspend_busy_executor_with_special_apc;
        table.suspend_for = suspend_busy_executor_with_special_apc_until;
        table.resume = resume_busy_executor_with_special_apc;
      }
      else {
        table.suspend = suspend_busy_executor_default;
        table.suspend_for = suspend_busy_executor_default_until;
        table.resume = resume_busy_executor_default;
      }
    }
  }

  const agt_uexec_vtable_t* get_vptr(agt_ctx_t ctx) noexcept {
    if (g_busyExecutorVTables.uexecVTable.resume == nullptr) [[unlikely]]
      init_vtable(ctx);
    return &g_busyExecutorVTables.uexecVTable;
  }
}



/*
local_busy_executor* create_exec(agt_ctx_t ctx) noexcept {
  if (g_busyExecutorVTables.createExecutor == nullptr) [[unlikely]]
    init_vtable(ctx);
  std::atomic_signal_fence(std::memory_order_acquire);
  const auto createExec = g_busyExecutorVTables.createExecutor;
  local_busy_executor* exec = nullptr;
  createExec();
}*/
