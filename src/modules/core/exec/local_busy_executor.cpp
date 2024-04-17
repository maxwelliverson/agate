//
// Created by Maxwell on 2024-03-10.
//


#include "agate/cast.hpp"

#include "core/exec.hpp"

#define NOMINMAX
#include <Windows.h>



enum busy_state {
  AGT_BUSY_STATE_RUNNING,
  AGT_BUSY_STATE_WAITING,
  AGT_BUSY_STATE_TIMED_OUT,
  AGT_BUSY_STATE_WOKEN_UP,
};


AGT_object(local_busy_self, extends(agt_self_st)) {
  busy_state state; // these are only ever updated by this thread, so they need not have atomic semantics (they do need to be signal safe though).
  agt_u32_t  stateEpoch;
  HANDLE     threadHandle;
  HANDLE     timer;
};

AGT_object(local_busy_executor_timeout_data, extends(object), aligned(8)) {
  agt_u32_t epoch;
  local_busy_self* self;
};


AGT_object(local_busy_executor, extends(agt_executor_st)) {
  agt_ctx_t ctx;
  local_busy_self self; // given there can only be one self at any given point, simply store it inline.
};


namespace {

  AGT_forceinline void enter_wait_state(local_busy_self& self) noexcept {
    ++self.stateEpoch;
    self.state = AGT_BUSY_STATE_WAITING;
    std::atomic_signal_fence(std::memory_order_release);
  }


  void AGT_stdcall wake_up_after_timeout(LPVOID arg, DWORD dwTimerLowValue, DWORD dwTimerHighValue) noexcept {
    (void)dwTimerLowValue;
    (void)dwTimerHighValue;
    auto data = static_cast<local_busy_executor_timeout_data*>(arg);
    if (data->self->stateEpoch == data->epoch && data->self->state == AGT_BUSY_STATE_WAITING) {
      data->self->state = AGT_BUSY_STATE_TIMED_OUT;
    }
    release(data);
  }

  void AGT_stdcall wake_up_busy_executor(ULONG_PTR arg) noexcept {
    const auto self = reinterpret_cast<local_busy_self*>(arg);
    std::atomic_signal_fence(std::memory_order_acquire);
    if (self->state == AGT_BUSY_STATE_WAITING)
      self->state = AGT_BUSY_STATE_WOKEN_UP;
  }

  inline constexpr static agt_timeout_t MininumLongTimeout = 20000; // 20 ms

  void AGT_stdcall block_busy_executor_on_address(agt_executor_t exec, agt_self_t agent) {
    AGT_invariant( agent != nullptr );
    auto& self = object_cast<local_busy_self>(*agent);

    enter_wait_state(self);

    do {
      DWORD result = SleepEx(INFINITE, TRUE);
      AGT_invariant( result == WAIT_IO_COMPLETION );
      std::atomic_signal_fence(std::memory_order_acquire);
    } while(self.state == AGT_BUSY_STATE_WAITING);

    AGT_invariant( self.state == AGT_BUSY_STATE_WOKEN_UP );

    self.state = AGT_BUSY_STATE_RUNNING;
  }

  agt_status_t AGT_stdcall block_for_busy_executor_on_address(agt_executor_t exec_, agt_self_t agent, agt_timeout_t timeout) {
    AGT_invariant( agent != nullptr );
    auto& self = object_cast<local_busy_self>(*agent);
    auto& exec = object_cast<local_busy_executor>(*exec_);


    enter_wait_state(self);

    if (timeout < MininumLongTimeout) {
      auto timeoutData = alloc<local_busy_executor_timeout_data>(exec.ctx);
      timeoutData->epoch = self.stateEpoch;
      timeoutData->self  = &self;
      LARGE_INTEGER liTimeout;
      liTimeout.QuadPart = -static_cast<LONGLONG>(timeout * 10);
      // If we use a callback with SetWaitableTimer, the callback will always go off, even if

      BOOL success = SetWaitableTimer(self.timer, &liTimeout, 0, wake_up_after_timeout, timeoutData, FALSE);
      assert( success );

      do {
        DWORD result = SleepEx(INFINITE, TRUE);
        AGT_invariant( result == WAIT_IO_COMPLETION );
        std::atomic_signal_fence(std::memory_order_acquire);
      } while(self.state == AGT_BUSY_STATE_WAITING);

      /*DWORD result = WaitForSingleObjectEx(self.timer, INFINITE, TRUE); // Must be EX so that we may enter an alertable state, allowing APC callbacks to be executed.
      AGT_invariant( result == WAIT_IO_COMPLETION || result == WAIT_OBJECT_0);
      if (result == WAIT_IO_COMPLETION) {
        std::atomic_signal_fence(std::memory_order_acquire);
        // another APC
        while (self.state == AGT_BUSY_STATE_WAITING) {
          result = WaitForSingleObjectEx(self.timer, INFINITE, TRUE);
          if (result == WAIT_OBJECT_0)
            self.state = AGT_BUSY_STATE_TIMED_OUT;
        }
      }*/
    }
    else {
      // long timeout
      // convert timeout from us to ms
      const auto deadline = now() + (timeout * 10); // worry about this maybe?
      DWORD result = SleepEx(static_cast<DWORD>(timeout / 1000), TRUE);

      if (result == WAIT_IO_COMPLETION) {
        // If we are woken up by some other APC call, self.state will still be 'waiting'.
        while(self.state == AGT_BUSY_STATE_WAITING) {
          const auto currentTime = now();
          if (currentTime < deadline ) {
            const auto remainingUS = deadline - currentTime;

            result = SleepEx(static_cast<DWORD>(remainingUS / 1000), TRUE);
            if (!result)
              continue;
          }

          self.state = AGT_BUSY_STATE_TIMED_OUT;
          break;
        }
      }
      else {
        self.state = AGT_BUSY_STATE_TIMED_OUT;
      }
    }


    bool timedOut = self.state == AGT_BUSY_STATE_TIMED_OUT;
    self.state = AGT_BUSY_STATE_RUNNING;
    return timedOut ? AGT_TIMED_OUT : AGT_SUCCESS;
  }

  void AGT_stdcall wake_busy_executor(agt_executor_t exec, agt_self_t target) noexcept {
    AGT_invariant( target != nullptr );
    auto& self = object_cast<local_busy_self>(*target);

    std::atomic_signal_fence(std::memory_order_acquire);
    if (self.state == AGT_BUSY_STATE_WAITING) [[likely]] {
      auto result = QueueUserAPC(wake_up_busy_executor, self.threadHandle, reinterpret_cast<ULONG_PTR>(&self));
      if (!result) {
        // :(
        // ???
      }
    }
  }
}