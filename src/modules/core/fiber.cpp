//
// Created by maxwe on 2023-07-30.
//

#include "fiber.hpp"
#include "agate/time.hpp"

#include "agents/agents.hpp"

#include <utility>

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

  void insert_timed_blocked_list(agt_async_t async, agt::fiber_pool& pool, agt::fiber* fiber, agt_timestamp_t deadline) noexcept {

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
  }
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




[[nodiscard]] bool fiber_is_blocked(agt::fiber* fiber) noexcept {}



void         agt::make_fibers(agt_ctx_t ctx, const fiber_create_info& createInfo) noexcept {

}

agt_fiber_transfer_t agt_fiber_switch_noexcept(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags) {

}

agt_fiber_transfer_t agt_fiber_switch_except(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags) {
  try {

  } catch (...) {
    auto ptr = std::current_exception();

  }
}


agt_status_t agt::fiber_block_and_switch(agt_async_t async_, agt_u64_t *pResult, agt::fiber* nextFiber) noexcept {
  auto async = (agt::async_base*)async_;
  auto currentFiber = tl_currentFiber;
  async->boundFiber = currentFiber;
  currentFiber->blockingAsync = async_;
  currentFiber->blockDeadline = (agt_timestamp_t)0;
  push_back_fiber(nextFiber->pool->blockedList, currentFiber);


}