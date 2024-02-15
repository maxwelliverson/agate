//
// Created by maxwe on 2023-07-30.
//

#include "fiber.hpp"

#include "attr.hpp"

#include "agate/time.hpp"
#include "ctx.hpp"

#include "agents/agents.hpp"

#include <utility>

#include <exception>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/*namespace {
  inline constinit thread_local agt::fiber* tl_primaryFiber = nullptr;
  inline constinit thread_local agt::fiber* tl_currentFiber = nullptr;


  struct fiber_list {
    agt::fiber_header head;
    size_t            length;
  };


  void init_list(fiber_list& list) noexcept {
    list.length = 0;
    list.head.next.ptr = &list.head;
    list.head.prev.ptr = &list.head;
  }

  void insert_fiber(agt::fiber_ref pos, agt::fiber_header* fiber) noexcept {
    fiber->next = pos.ptr->next;
    fiber->prev = pos;
    pos.ptr->next.ptr->prev.ptr = fiber;
    pos.ptr->next.ptr = fiber;
  }

  void remove_fiber(agt::fiber_header* fiber) noexcept {
    fiber->next.ptr->prev = fiber->next;
    fiber->prev.ptr->next = fiber->prev;
  }

  void push_back_fiber(fiber_list& list, agt::fiber* fiber) noexcept {
    insert_fiber(list.head.prev, fiber);
    list.length += 1;
  }

  void push_front_fiber(fiber_list& list, agt::fiber* fiber) noexcept {
    insert_fiber({ .ptr = &list.head }, fiber);
    list.length += 1;
  }

  agt::fiber* pop_front_fiber(fiber_list& list) noexcept {
    if (list.length == 0)
      return nullptr;
    auto front = list.head.next.ptr;
    remove_fiber(front);
    return static_cast<agt::fiber*>(front);
  }

  void insert_fiber_range(agt::fiber_header* prev, agt::fiber_header* first, agt::fiber_header* last) noexcept {
    agt::fiber_header* next = prev->next.ptr;
    prev->next.ptr = first;
    next->prev.ptr = last;
    first->prev.ptr = prev;
    last->next.ptr = next;
  }

  void merge_list_to_back(fiber_list& dstList, fiber_list& srcList) noexcept {
    if (srcList.length > 0) {
      insert_fiber_range(dstList.head.prev.ptr, srcList.head.next.ptr, srcList.head.prev.ptr);
      dstList.length += std::exchange(srcList.length, 0);
    }
  }


  struct fwd_fiber_list {
    agt::fiber_header*  head;
    agt::fiber_header** tail;
    size_t              length;
    agt::fiber_header*  last;
  };

  void init_fwd_list(fwd_fiber_list& list) noexcept {
    list.head   = nullptr;
    list.tail   = &list.head;
    list.length = 0;
    list.last   = nullptr;
  }

  void push(fwd_fiber_list& fwdList, agt::fiber_header* fiber) noexcept {
    assert( fwdList.tail != nullptr );
    *fwdList.tail = fiber;
    fwdList.tail  = &fiber->next.ptr;
    fwdList.length += 1;
  }

  void double_link_list(fwd_fiber_list& list) noexcept {
    if (list.length > 0) {
      assert( list.head != nullptr );
      assert( list.tail != &list.head );
      if (list.length == 1) {
        list.last = list.head;
        return;
      }

      agt::fiber_header* prevFiber = list.head;
      agt::fiber_header* nextFiber = prevFiber->next.ptr;

      for (size_t i = 1; i < list.length; ++i) {
        nextFiber->prev.ptr = prevFiber;
        prevFiber = nextFiber;
        nextFiber = nextFiber->next.ptr;
      }

      list.last = prevFiber;
    }
  }
}


struct agt::fiber_pool {
  // agt::fiber*           fibers;
  agt_u32_t             fiberCount;
  agt_flags32_t         flags;
  fiber_list            idleList;
  fiber_list            readyQueue;
  fiber_list            blockedList;
  fiber_list            timedBlockedList;
  agt_timestamp_t       firstTimestamp;
  agt_timestamp_t       lastTimestamp;
  agt::fiber_sched_proc schedProc;
  void*                 procData;
  agt_u32_t             stackReserveSize;
  agt_u32_t             stackCommitSize;
  agt_u32_t             maxFiberCount;
};*/


namespace {

  inline constexpr agt_timestamp_t NoTimestamp = 0;

  /*void insert_timed_blocked_list(agt_async_t async, agt::fiber_pool& pool, agt::fiber* fiber, agt_timestamp_t deadline) noexcept {

    fiber->blockingAsync = async;
    fiber->blockDeadline = deadline;

    if (pool.lastTimestamp < deadline) {
      pool.lastTimestamp = deadline;
      push_back_fiber(pool.timedBlockedList, fiber);
      if (pool.firstTimestamp == NoTimestamp)
        pool.firstTimestamp = deadline;
    }
    else if (deadline < pool.firstTimestamp) {
      pool.firstTimestamp = deadline;
      push_front_fiber(pool.timedBlockedList, fiber);
      // If the timedBlockedList was previously empty, the first if/else clause would have been taken, so we are guaranteed here that the list isn't empty.
    }
    else {

      agt::fiber_header* prevFiber = pool.timedBlockedList.head.next.ptr;
      agt::fiber_header* nextFiber = prevFiber->next.ptr;

      assert( nextFiber != &pool.timedBlockedList.head );

      while (static_cast<agt::fiber*>(nextFiber)->blockDeadline <= deadline) {
        prevFiber = nextFiber;
        nextFiber = prevFiber->next.ptr;
        assert( nextFiber != &pool.timedBlockedList.head );
      }

      fiber->next.ptr = nextFiber;
      fiber->prev.ptr = prevFiber;
      prevFiber->next.ptr = fiber;
      nextFiber->prev.ptr = fiber;

      pool.timedBlockedList.length += 1;
    }
  }

  void make_fibers_ready(agt::fiber_pool& pool) noexcept {

    fwd_fiber_list unblockList;
    bool isInited = false;

    if (pool.blockedList.length > 0) {
      init_fwd_list(unblockList);
      isInited = true;

      agt::fiber_header* prev = &pool.blockedList.head;
      agt::fiber_header* curr = prev->next.ptr;
      agt::fiber_header* const end = &pool.blockedList.head;

      size_t consecutiveRemovedFibers = 0;

      do {

        auto next  = curr->next.ptr;
        auto fib   = static_cast<agt::fiber*>(curr);
        auto async = (agt::async_base*)fib->blockingAsync;
        if (async->status != AGT_NOT_READY) {
          push(unblockList, curr);
          ++consecutiveRemovedFibers;
        }
        else {
          if (consecutiveRemovedFibers > 0) {
            prev->next.ptr = curr;
            curr->prev.ptr = prev;
            consecutiveRemovedFibers = 0;
          }
          prev = curr;
        }

        curr = curr->next.ptr;
      } while(curr != end);

      if (consecutiveRemovedFibers > 0) {
        prev->next.ptr = curr;
        curr->prev.ptr = prev;
      }

      pool.blockedList.length -= unblockList.length;
    }

    if (pool.timedBlockedList.length > 0) {
      if (!isInited)
        init_fwd_list(unblockList);
      size_t initialBlockListLength = unblockList.length;
      const auto now = agt::get_fast_timestamp();

      agt::fiber_header* first;
      agt::fiber_header* last;
      size_t             count;

      if (pool.lastTimestamp < now) {
        first = pool.timedBlockedList.head.next.ptr;
        last  = pool.timedBlockedList.head.prev.ptr;
      }
      else if (now < pool.firstTimestamp) {

      }
      else {

      }
    }
  }*/
}


enum op_kind {
  BLOCK_AND_START_AGENT,
  BLOCK_AND_UNBLOCK,
  YIELD_AND_
};


struct afiber_save {
  agt_u32_t shouldFullyRestore;
  uintptr_t rsp;
  uintptr_t retAddr;
  uintptr_t stackBase;
  uintptr_t stackLimit;
  uintptr_t xsaveMask;

};

typedef struct afiber_st {
  agt_self_t boundSelf;
  void*      saveData;
  void    (* retFunc)(void*);
  void*      funcData;
  uintptr_t  stackBase;
  uintptr_t  stackLimit;
  uintptr_t  stackInitial;
}* afiber_t;




[[nodiscard]] bool fiber_is_blocked(agt::fiber* fiber) noexcept {
  return false;
}



/*void         agt::make_fibers(agt_ctx_t ctx, const fiber_create_info& createInfo) noexcept {

}*/


/*
agt_fiber_transfer_t agt_fiber_switch_except(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags) {
  try {

  } catch (...) {
    auto ptr = std::current_exception();

  }
}
*/


/*agt_status_t agt::fiber_block_and_switch(agt_async_t async_, agt_u64_t *pResult, agt::fiber* nextFiber) noexcept {
  auto async = (agt::async_base*)async_;
  auto currentFiber = tl_currentFiber;
  async->boundFiber = currentFiber;
  currentFiber->blockingAsync = async_;
  currentFiber->blockDeadline = (agt_timestamp_t)0;
  push_back_fiber(nextFiber->pool->blockedList, currentFiber);


}*/


namespace {

  struct local_stack_info {
    uintptr_t base;
    uintptr_t limit;
    uintptr_t deallocStack;
  };


  local_stack_info getLocalStackInfo() noexcept {
    auto tebAddr = NtCurrentTeb();
    auto tib = reinterpret_cast<PNT_TIB>(tebAddr);
    local_stack_info info;
    info.base = reinterpret_cast<uintptr_t>(tib->StackBase);
    info.limit = reinterpret_cast<uintptr_t>(tib->StackLimit);
    std::memcpy(&info.deallocStack, reinterpret_cast<std::byte*>(tebAddr) + 0x1478, sizeof(uintptr_t));
    return info;
  }

  void set_current_fiber(agt_fiber_t fiber) noexcept {
    reinterpret_cast<PNT_TIB>(NtCurrentTeb())->ArbitraryUserPointer = fiber;
  }

  agt::impl::fiber_stack_info allocStack(agt::fctx* fctx) noexcept {
    impl::fiber_stack_info stack{};
    auto reservedMemory = VirtualAlloc(nullptr, fctx->stackReserveSize, MEM_COMMIT, PAGE_READWRITE);
    DWORD oldOptions;
    agt_u32_t guardSize = fctx->stackCommitSize;
    VirtualProtect(reservedMemory, guardSize, PAGE_GUARD, &oldOptions);
    stack.address = reservedMemory;
    stack.commitSize = guardSize;
    stack.guardPageSize = guardSize;
    stack.reserveSize = fctx->stackReserveSize;
    stack.randomOffset = 0;
    return stack;
  }

  void freeStack(agt_fiber_t fiber) noexcept {
    auto f = fiber->fctx;
    VirtualFree(reinterpret_cast<PVOID>(fiber->stackBase - f->stackReserveSize), f->stackReserveSize, MEM_RELEASE);
  }
}


namespace agt {


  static fctx* make_fctx(agt_ctx_t ctx, const agt_fctx_desc_t& desc) noexcept {
    // auto f = alloc<fctx>(ctx);
    auto f = new fctx;

    uint32_t maxFiberCount = desc.maxFiberCount;
    if (maxFiberCount == 0)
      maxFiberCount = attr::thread_count(ctx->instance); // maybe add an attribute for max fiber count?
    f->fiberCount = 0;

    if (desc.parent) {
      auto sharedFctx = desc.parent->fctx;
      auto pooledFctx = sharedFctx->pooledFctx;
      assert( pooledFctx == nullptr ); // for now, cause pooled fctxs aren't implemented yet
    }

    agt_u32_t stackSize;
    if (desc.stackSize) {
      const auto stackAlignment = attr::stack_size_alignment(ctx->instance);
      stackSize = align_to(desc.stackSize, stackAlignment);
    }
    else
      stackSize = attr::default_fiber_stack_size(ctx->instance);

    f->stackReserveSize = stackSize;
    f->stackCommitSize = 4096; // do better
    f->enterData = nullptr;
    f->exitCode = 0;
    f->exitParam = 0;
    f->pooledFctx = nullptr;

    return f;
  }

  static void destroy_fctx(agt_ctx_t ctx, fctx* f) noexcept {
    // release(f);
    delete f;
  }

  agt_status_t AGT_stdcall enter_fctx(agt_ctx_t ctx, const agt_fctx_desc_t* pFCtxDesc, int* pExitCode) AGT_noexcept {

    if (!ctx) {
      ctx = get_ctx();
      if (!ctx)
        return AGT_ERROR_CTX_NOT_ACQUIRED;
    }

    if (!pFCtxDesc)
      return AGT_ERROR_INVALID_ARGUMENT;

    const auto exports = ctx->exports;
    const auto fiber = new agt_fiber_st; // TODO: Change to ctx allocation once fiber is a proper object.

    const auto inst = ctx->instance;

    auto&& desc = *pFCtxDesc;
    auto&& localStackInfo = getLocalStackInfo();

    fiber->privateData = nullptr;
    fiber->userData    = desc.userData;
    fiber->storeStateFlags = attr::full_state_save_mask(inst);
    fiber->saveRegionSize = attr::max_state_save_size(inst);
    fiber->stackBase = localStackInfo.base;
    fiber->stackLimit = localStackInfo.limit;
    fiber->deallocStack = localStackInfo.deallocStack;
    fiber->defaultJumpTarget = nullptr;
    fiber->ctx = ctx;

    fiber->executor = nullptr;
    fiber->fctx = make_fctx(ctx, desc);
    fiber->blockingAsync = nullptr;
    fiber->blockDeadline = 0;
    fiber->blockedExecAgentTag = nullptr;
    fiber->state = agt::FIBER_STATE_RUNNING;
    fiber->flags = eFiberIsConverted;
    // fiber->flags = desc.flags;

    // ctx->boundFiber = fiber;

    const auto currentFiberData = current_fiber();

    assert( currentFiberData == nullptr );

    set_current_fiber(fiber);



    auto resultParam = exports->_pfn_fiber_loop(desc.proc, desc.initialParam, AGT_FIBER_SAVE_EXTENDED_STATE);


    // Also worth asking whether to wait for all outstanding fibers here like exit_fctx?

    set_current_fiber(nullptr);

    if (pExitCode)
      *pExitCode = fiber->fctx->exitCode;

    destroy_fctx(fiber->ctx, fiber->fctx);

    (void)resultParam; // :(

    delete fiber;

    return AGT_SUCCESS;
  }

  void         AGT_stdcall exit_fctx(agt_ctx_t ctx, int exitCode) AGT_noexcept {
    // Dunno how exactly this function should work??
    // What should it destroy?
    // What should it do????
    // I could set up a fiber_fork that *then* loops in enter_fctx, save the forked data, and then jump back to it here,
    // but how do you manage other fibers in a potentially blocked state or whatever? should this fiber wait, transfer to them to finish,
    // and ultimately only exit when all other fibers are done executing?
    // Maybe???

    auto fiber = current_fiber();
    fiber->fctx->exitCode = exitCode;
  }

  agt_status_t AGT_stdcall new_fiber(agt_fiber_t* pFiber, agt_fiber_proc_t proc, void* userData) AGT_noexcept {
    const auto fiber = current_fiber();
    if ( fiber == nullptr )
      return AGT_ERROR_NO_FCTX;

    if (!pFiber)
      return AGT_ERROR_INVALID_ARGUMENT;

    const auto ctx = fiber->ctx;

    auto newFiber = new agt_fiber_st;

    const auto stackInfo = allocStack(fiber->fctx);

    newFiber->privateData = nullptr;
    newFiber->userData = userData;
    newFiber->storeStateFlags = fiber->storeStateFlags;
    newFiber->saveRegionSize = fiber->saveRegionSize;

    newFiber->defaultJumpTarget = fiber;
    newFiber->ctx = ctx;
    newFiber->executor = nullptr;
    newFiber->fctx = fiber->fctx;
    newFiber->blockingAsync = nullptr;
    newFiber->blockDeadline = 0;
    newFiber->blockedExecAgentTag = nullptr;
    newFiber->state = FIBER_STATE_IDLE;
    newFiber->flags = 0;

    impl::assembly::afiber_init(newFiber, proc, stackInfo);

    *pFiber = newFiber;

    return AGT_SUCCESS;
  }

  agt_status_t AGT_stdcall destroy_fiber(agt_fiber_t fiber) AGT_noexcept {

    auto thisFiber = current_fiber();

    if (thisFiber == nullptr)
      return AGT_ERROR_NO_FCTX;

    if (thisFiber == fiber || fiber == nullptr || (fiber->flags & eFiberIsConverted))
      return AGT_ERROR_INVALID_ARGUMENT;

    freeStack(fiber);

    auto fctx = fiber->fctx;

    fctx->fiberCount -= 1;

    // maybe see if fctx needs to die????

    delete fiber;

    return AGT_SUCCESS;
  }

  void*        AGT_stdcall set_fiber_data(agt_fiber_t fiber, void* userData) AGT_noexcept {
    return std::exchange(fiber->userData, userData);
  }

  void*        AGT_stdcall get_fiber_data(agt_fiber_t fiber) AGT_noexcept {
    return fiber->userData;
  }


  agt_fiber_transfer_t AGT_stdcall fiber_switch_except(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags) {

    /*if (int exceptionCount = std::uncaught_exceptions()) [[unlikely]] {

    }

    auto exceptionPtr = std::current_exception();*/
    return {};
  }

  AGT_noreturn void    AGT_stdcall fiber_jump_except(agt_fiber_t fiber, agt_fiber_param_t param) {

  }

  agt_fiber_transfer_t AGT_stdcall fiber_fork_except(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags) {
    return {};
  }

  agt_fiber_param_t    AGT_stdcall fiber_loop_except(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags) {
    return {};
  }

}












