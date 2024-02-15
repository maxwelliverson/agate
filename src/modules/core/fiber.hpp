//
// Created by maxwe on 2023-07-30.
//

#ifndef AGATE_CORE_FIBER_HPP
#define AGATE_CORE_FIBER_HPP

#include "config.hpp"

#include "rc.hpp"


#include <bitset>

#include <utility>

namespace agt {

  inline constexpr static agt_fiber_flags_t eFiberPartialRestore = 0x40000000;

  inline constexpr static agt_flags32_t eFiberIsConverted = 0x1;


  using fiber_jump_flags_t = agt_flags32_t;

  enum {
    FIBER_JUMP_SWITCH_FIBER = 0x1
  };

  enum {
    FIBER_SAVE_SUPPORTS_XSAVEOPT = 0x1000
  };

  struct AGT_cache_aligned xsave_data {
    uint16_t fcw;
    uint16_t fsw;
    uint8_t  ftw;
    uint8_t  rsvd;
    uint16_t fop;
    uint64_t fip;
    uint64_t fdp;
    uint32_t mxcsr;
    uint32_t mxcsrMask;
    std::byte mmx0[10];
    std::byte rsvd0[6];
    std::byte mmx1[10];
    std::byte rsvd1[6];
    std::byte mmx2[10];
    std::byte rsvd2[6];
    std::byte mmx3[10];
    std::byte rsvd3[6];
    std::byte mmx4[10];
    std::byte rsvd4[6];
    std::byte mmx5[10];
    std::byte rsvd5[6];
    std::byte mmx6[10];
    std::byte rsvd6[6];
    std::byte mmx7[10];
    std::byte rsvd7[6];
    std::byte xmm0[16];
    std::byte xmm1[16];
    std::byte xmm2[16];
    std::byte xmm3[16];
    std::byte xmm4[16];
    std::byte xmm5[16];
    std::byte xmm6[16];
    std::byte xmm7[16];
    std::byte xmm8[16];
    std::byte xmm9[16];
    std::byte xmm10[16];
    std::byte xmm11[16];
    std::byte xmm12[16];
    std::byte xmm13[16];
    std::byte xmm14[16];
    std::byte xmm15[16];
    std::byte padding[96];
    std::bitset<64> xstateBV;
    std::bitset<64> xcompBV;
  };

  struct AGT_cache_aligned fiber_data {
    fiber_data*       prevData;
    uintptr_t         rip; // return address
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
    xsave_data        xsave;
    // alignas(AGT_CACHE_LINE) std::byte data[FiberSaveDataSize];
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

  AGT_final_object_type(pooled_fctx, ref_counted) {

  };

  AGT_final_object_type(fctx) {
    agt_u32_t              fiberCount;
    agt_u32_t              maxFiberCount;
    agt_u32_t              stackReserveSize;
    agt_u32_t              stackCommitSize;
    agt_fiber_t            rootFiber;
    fiber_data*            enterData;
    agt_fiber_param_t      exitParam;
    int                    exitCode;
    pooled_fctx*           pooledFctx;
  };


  AGT_final_object_type(fiber) {
    agt_u32_t         saveRegionSize;
    uint64_t          storeStateFlags;
    agt::fiber_data*  privateData;
    void*             userData;
    uintptr_t         stackBase;
    uintptr_t         stackLimit;
    uintptr_t         deallocStack;

    AGT_cache_aligned
    agt_ctx_t         ctx;
    agt_fiber_t       defaultJumpTarget;
    agt_executor_t    executor;
    fctx*             fctx;
    agt_async_t       blockingAsync;
    agt_timestamp_t   blockDeadline;
    agt_eagent_t      blockedExecAgentTag;
    agt::fiber_state  state;
  };




  namespace impl {

    inline constexpr static size_t BasicSaveDataSize = 128;
    inline constexpr static size_t FullSaveDataSize  = 1024;
    inline constexpr static size_t SaveDataAlignment = 128;

    // fiber*              new_fiber(fiber_pool* pool) noexcept;
    // fiber*              free_fiber(fiber_pool* pool) noexcept;

    struct fiber_stack_info {
      void*            address;          // Address of the bottom of the stack
      size_t           reserveSize;      // Reserved size of the stack (maximum stack size)
      size_t           commitSize;       // Committed size of the stack (initial stack size)
      agt_u32_t        guardPageSize;    // Size of guard pages used to detect stack overflow
      agt_u32_t        randomOffset;     // Random value within [0, pageSize) that will be subtracted from the stackBase to obtain the initial stack address.
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
      extern "C" void                 afiber_init(agt_fiber_t fiber, agt_fiber_proc_t proc, const fiber_stack_info& stackInfo);
    }

  }



  inline static agt_fiber_t current_fiber() noexcept {
    return static_cast<agt_fiber_t>(reinterpret_cast<PNT_TIB>(NtCurrentTeb())->ArbitraryUserPointer);
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
  agt_fiber_t       defaultJumpTarget;

  agt_ctx_t         ctx;

  agt_executor_t    executor;
  fctx*             fctx;
  agt_async_t       blockingAsync;
  agt_timestamp_t   blockDeadline;
  agt_eagent_t      blockedExecAgentTag;
  agt::fiber_state  state;
  agt_flags32_t     flags;
  // agt_fiber_flags_t flags;
};


}

#endif//AGATE_CORE_FIBER_HPP
