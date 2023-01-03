//
// Created by maxwe on 2022-12-30.
//

#ifndef AGATE_PRIORITY_QUEUE_HPP
#define AGATE_PRIORITY_QUEUE_HPP

#include "config.hpp"

#include "core/object_pool.hpp"

namespace agt {
  enum class priority : agt_u32_t {
    critical,
    high,
    normal,
    low,
    background
  };

  template <typename T>
  struct pqueue_traits {
    /*
     * using next_type = ...;
     * using
     * */
  };

  template <typename T>
  concept queueable =
      requires {
        typename T::next_type;
      } &&
      requires(const T& constLValue, T& lValue) {
        lValue.pqueue_enqueue(std::addressof(lValue));
        { constLValue.pqueue_next() } noexcept -> std::same_as<>;
      };

  template <typename T, thread_safety SafetyModel>
  class intrusive_pqueue {

  };

  namespace impl {
    template <thread_safety SafetyModel>
    class value_pqueue_base {
    protected:
      struct element_base : object {
        element_base* next;
      };
    public:


    private:

    };
  }

  template <typename T, thread_safety SafetyModel>
  class value_pqueue {
    inline constexpr static agt_u32_t PriorityClassCount = static_cast<agt_u32_t>(priority::background) + 1;

    class queue_element {
    public:

    private:
      T* m_ptr;
    };

    using intrusive_pqueue_type = intrusive_priority_queue<>;
  public:

  private:

  };
}

#endif//AGATE_PRIORITY_QUEUE_HPP
