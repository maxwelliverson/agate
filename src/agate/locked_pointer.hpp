//
// Created by maxwe on 2023-01-06.
//

#ifndef AGATE_LOCKED_POINTER_HPP
#define AGATE_LOCKED_POINTER_HPP

#include "config.hpp"

#include "atomic.hpp"


namespace agt {


  inline static agt_u32_t get_ctx_id(agt_ctx_t ctx) noexcept;


  template <typename T>
  struct unlocked_ptr {
    T*        value;
    agt_u32_t epoch;
    agt_u32_t threadId;
  };


  struct list_base {
    list_base* next;
  };

  struct locked_stack {
    list_base* head;
    agt_u32_t  depth;
    agt_u32_t  epoch;
  };

  struct queue_base {
    queue_base* next;
    agt_u32_t   epoch;
    agt_u32_t   self;
  };

  struct locked_queue {
    agt_u32_t   head;
    agt_u32_t   tail;
    agt_u32_t   length;
    agt_u32_t   epoch;
    queue_base* array;
  };

  /*void enqueue(locked_queue& queue, queue_base* item) noexcept {
    struct AGT_alignas(16) {
      agt_u32_t head;
      agt_u32_t tail;
      agt_u32_t length;
      agt_u32_t epoch;
    } old, next;

    std::atomic_thread_fence(std::memory_order_acquire);
    std::memcpy(&old, &queue, sizeof old);

    const agt_u32_t selfId = item->self;

    next.tail = selfId;

    do {

      if (old.length == 0) {
        next.head   = selfId;
        next.length = 1;
      }
      else {
        auto tail = queue.array + old.tail;
        next.head   = old.head;
        next.length = old.length + 1;
        next.epoch  = old.epoch + 1;
      }



      next.length = old.length + 1;
    } while(!atomicCompareExchange16(&queue, &old, &next));
  }

  queue_base* dequeue(locked_queue& queue) noexcept {

  }*/


  template <typename T>
  class locked_ptr {
  public:

    using unlocked_type = unlocked_ptr<T>;

    [[nodiscard]] unlocked_type take(agt_ctx_t ctx) noexcept {
      unlocked_type old, next;


    }


  private:
    unlocked_ptr<T> m_;
  };



}

#endif//AGATE_LOCKED_POINTER_HPP
