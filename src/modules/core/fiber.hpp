//
// Created by maxwe on 2023-07-30.
//

#ifndef AGATE_CORE_FIBER_HPP
#define AGATE_CORE_FIBER_HPP

#include "config.hpp"

#include "rc.hpp"

#include <utility>

namespace agt {

  inline constexpr static size_t FiberSaveDataSize = 512;

  using fiber_jump_flags_t = agt_flags32_t;

  enum {
    FIBER_JUMP_SWITCH_FIBER = 0x1
  };

  enum {
    FIBER_SAVE_SUPPORTS_XSAVEOPT = 0x1000
  };

  struct alignas(AGT_CACHE_LINE) fiber_data {
    fiber_data*       prevData;
    uintptr_t         rip;
    uintptr_t         rbx;
    uintptr_t         rbp;
    uintptr_t         rdi;
    uintptr_t         rsi;
    uintptr_t         r12;
    uintptr_t         r13;
    uintptr_t         r14;
    uintptr_t         r15;
    agt_fiber_flags_t saveFlags;
    agt_u32_t         reserved;
    void*             transferAddress;
    uintptr_t         stack;
    alignas(AGT_CACHE_LINE) std::byte data[FiberSaveDataSize];
  };

  enum fiber_state {
    FIBER_STATE_IDLE,
    FIBER_STATE_RUNNING,
    FIBER_STATE_BLOCKED,
    FIBER_STATE_READY
  };

  struct fiber;
  struct fiber_pool;
  struct fiber_header;

  AGT_virtual_object_type(fiber_pool, ref_counted) {
    agt_u32_t             fiberCount;
    agt_u32_t             stackReserveSize;
    agt_u32_t             stackCommitSize;
    agt_u32_t             maxFiberCount;
    agt_flags32_t         flags;
    /*fiber_list            idleList;
    fiber_list            readyQueue;
    fiber_list            blockedList;
    fiber_list            timedBlockedList;
    agt_timestamp_t       firstTimestamp;
    agt_timestamp_t       lastTimestamp;
    agt::fiber_sched_proc schedProc;
    void*                 procData;*/

  };




  namespace impl {

    inline constexpr static size_t BasicSaveDataSize = 128;
    inline constexpr static size_t FullSaveDataSize  = 1024;
    inline constexpr static size_t SaveDataAlignment = 128;

    // fiber*              new_fiber(fiber_pool* pool) noexcept;
    // fiber*              free_fiber(fiber_pool* pool) noexcept;

    struct fiber_init_info {
      fiber_pool*            pool;
      agt_fiber_proc_t       proc;
      void*                  userData;
      agt_u64_t              stateSaveMask;
      agt_fiber_t            root;
    };

    namespace assembly {


      // Operations:
      //   - push_context(fiber, flags) -> optional<param>
      //       * Let selfFiber be the fiber currently bound
      //         to the executing thread.
      //       * Saves the current execution context, and
      //         pushes it to selfFiber's context stack.
      //       * The value of flags controls which aspects
      //         of the context may be saved.
      //       * Returns nothing from the initial call, but
      //         if the context is later resumed by a call to
      //         restore(selfFiber, param), execution will
      //         return param. (ie. after push_context is
      //         called, the return value must be checked
      //         to determine whether or not this is the
      //         initial return, or if it is a context
      //         restoration).
      //   - pop_context(fiber)    -> void
      //       * Pops the execution context from the top of
      //         the fiber currently bound to the executing
      //         thread.
      //   - restore(fiber, param) -> void [[noreturn]]
      //       * Resumes the execution context at the top
      //         of fiber's context stack.
      //       * This function does *not* return, and
      //         execution jumps back to the point in the
      //         code where push_context created the
      //         execution context being resumed.
      //       * Said push_context call then returns
      //         the param value passed to restore.


      /**
       * result = push_context(fiber, flags)
       * if result is null
       *   result = proc(fiber->fctx, fiber->userData)
       * pop_context(fiber)
       * return result
       * **/
      extern "C" agt_fiber_transfer_t afiber_fork(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags);

      /// afiber_switch is essentially an optimized version of
      /**
       * result = push_context(fromFiber, flags)
       * if result is null
       *   restore(toFiber, param)
       * pop_context(fromFiber)
       * return result
       * **/
      extern "C" agt_fiber_transfer_t afiber_switch(agt_fiber_t toFiber, agt_fiber_param_t param, agt_fiber_flags_t flags);

      /**
       * restore(toFiber, param)
       * **/
      extern "C" [[noreturn]] void    afiber_jump(agt_fiber_t toFiber, agt_fiber_param_t param/*, fiber_jump_flags_t flags*/);

      /**
       * result = push_context(fiber, flags)
       * if result is null
       *   result = param
       * result = proc(fiber->fctx, fiber->flags, result, fiber->userData)
       * pop_context(fiber)
       * return result
       * **/
      extern "C" agt_fiber_transfer_t afiber_loop(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags);

      /**
       * Initialize the initial context for fiber,
       * such that it may be switch/jumped to like any other context.
       * **/
      extern "C" void                 afiber_init(agt_fiber_t fiber, agt_fiber_proc_t proc, bool isConvertingThread);
    }

  }

}

extern "C" {

struct alignas(AGT_CACHE_LINE) agt_fiber_st {
  agt::fiber_data*  privateData;
  void*             userData;
  uint64_t          storeStateFlags;
  size_t            saveRegionSize;
  uintptr_t         stackBase;
  uintptr_t         stackLimit;
  uintptr_t         deallocStack;

  agt_ctx_t         ctx;

  agt_executor_t    executor;
  agt::fiber_pool*  pool;
  agt_async_t       blockingAsync;
  agt_timestamp_t   blockDeadline;
  agt_fiber_t       root;
  agt::fiber_state  state;
  agt_fiber_flags_t flags;
};


}

#endif//AGATE_CORE_FIBER_HPP
