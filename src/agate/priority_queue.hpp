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
    using next_type = T*;
    /*
     * using next_type = ...;
     * using
     * */
  };



  namespace impl {
    template <typename T>
    struct get_priority_type {
      using type = priority;
    };
    template <typename T> requires(requires { typename T::priority_type; } )
    struct get_priority_type<T> {
      using type = typename T::priority_type;
    };
    template <typename T>
    struct get_min_priority;
    template <typename T> requires(std::same_as<typename get_priority_type<T>::type, priority> && !requires { T::minimum_priority; } )
    struct get_min_priority<T> {
      inline constexpr static priority value = priority::background;
    };
    template <typename T> requires(requires { T::minimum_priority; } )
    struct get_min_priority<T> {
      inline constexpr static auto value = T::minimum_priority;
    };
    template <typename T>
    struct get_max_priority;
    template <typename T> requires(std::same_as<typename get_priority_type<T>::type, priority> && !requires { T::maximum_priority; } )
    struct get_max_priority<T> {
      inline constexpr static priority value = priority::critical;
    };
    template <typename T> requires(requires { T::maximum_priority; } )
    struct get_max_priority<T> {
      inline constexpr static auto value = T::minimum_priority;
    };

    template <typename T>
    struct get_previous_type {
      using type = typename T::next_type*;
    };
    template <typename T> requires( requires{ typename T::previous_type; })
    struct get_previous_type<T> {
      using type = typename T::previous_type;
    };

    template <typename T, typename Traits>
    struct get_accessor_type {
      struct type {
        AGT_forceinline static T* access(T* next) noexcept {
          return next;
        }
      };
    };
    template <typename T, typename Traits> requires( requires{ typename Traits::accessor_type; })
    struct get_accessor_type<T, Traits> {
      using type = typename Traits::accessor_type;
    };

    template <typename Traits>
    struct get_should_clamp_priority {
      inline constexpr static bool value = false;
    };
    template <typename Traits> requires ( requires{ Traits::should_clamp_priority; })
    struct get_should_clamp_priority<Traits> {
      inline constexpr static bool value = Traits::should_clamp_priority;
    };

    template <typename Traits>
    struct priority_is_inverted;
    template <typename Traits> requires(std::same_as<typename get_priority_type<Traits>::type, priority> && !requires { Traits::inverted_priority; } )
    struct priority_is_inverted<Traits> {
      inline constexpr static bool value = true;
    };
    template <typename Traits> requires( requires { Traits::inverted_priority; })
    struct priority_is_inverted<Traits> {
      inline constexpr static bool value = Traits::inverted_priority;
    };

    template <typename T, typename Traits>
    using accessor_type_t = typename get_accessor_type<T, Traits>::type;
  }



  template <typename T,
            thread_safety SafetyModel,
            typename PQueueTraits = pqueue_traits<T>>
  class intrusive_pqueue : impl::accessor_type_t<T, PQueueTraits> {
    using traits = PQueueTraits;
  public:

    using value_type    = T;
    using pointer       = T*;
    using next_type     = typename traits::next_type;
    using accessor_type = impl::accessor_type_t<T, PQueueTraits>;
    using priority_type = typename impl::get_priority_type<traits>::type;


  private:

    using previous_type = typename impl::get_previous_type<traits>::type;

    using priority_int_type = typename std::conditional_t<
        std::is_enum_v<priority_type>,
            std::underlying_type<priority_type>,
                std::type_identity<priority_type>>::type;

    inline constexpr static priority_type MinPriority = impl::get_min_priority<traits>::value;
    inline constexpr static priority_type MaxPriority = impl::get_max_priority<traits>::value;

    inline constexpr static priority_int_type MinPriorityInt = static_cast<priority_int_type>(MinPriority);
    inline constexpr static priority_int_type MaxPriorityInt = static_cast<priority_int_type>(MaxPriority);

    inline constexpr static bool ShouldClampPriority = impl::get_should_clamp_priority<traits>::value;
    inline constexpr static bool InvertedPriority    = impl::priority_is_inverted<traits>::value;

    inline constexpr static size_t PriorityRangeSize = (InvertedPriority ? MinPriorityInt - MaxPriorityInt : MaxPriorityInt - MinPriorityInt) + 1;


  public:

    void enqueue(pointer el, priority_type prio) noexcept {
      auto& tail = _get_tail(prio);
    }

    [[nodiscard]] pointer dequeue() noexcept {

    }

    [[nodiscard]] pointer try_dequeue() noexcept {

    }

    [[nodiscard]] pointer try_dequeue_for(agt_timeout_t timeout) noexcept {

    }

    [[nodiscard]] pointer try_dequeue_until(deadline dl) noexcept {

    }

  private:



    struct proxy_type {
      next_type next;
    };

    struct tail_type {
      next_type     next;
      previous_type prev;
    };

    struct head_and_tail {
      next_type head;
      next_type tail;
    };

    AGT_forceinline tail_type& _get_tail(priority_type prio) noexcept {
      if constexpr (ShouldClampPriority)
        prio = std::clamp(prio, MinPriority, MaxPriority);
      const auto index = static_cast<priority_int_type>(prio);
      if constexpr (InvertedPriority)
        return m_tails[ index - MaxPriorityInt ];
      else
        return m_tails[ index - MinPriorityInt ];
    }

    proxy_type m_head;
    pointer    m_tailObjects[PriorityRangeSize];
    tail_type  m_tails[PriorityRangeSize];
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

    struct value_pqueue_element_base {
      value_pqueue_element_base* next;
    };

    template <typename T>
    struct value_pqueue_element : value_pqueue_element_base {
      T value;
    };

    template <typename T, typename Prio>
    struct value_pqueue_object : object {
      Prio priority;
      value_pqueue_element<T> element;
    };

    template <typename T, typename Traits>
    struct value_pqueue_traits {
      using next_type = value_pqueue_element_base*;

      using priority_type = typename get_priority_type<Traits>::type;

      inline constexpr static priority_type minimum_priority = get_min_priority<Traits>::value;
      inline constexpr static priority_type maximum_priority = get_max_priority<Traits>::value;

      inline constexpr static bool          should_clamp_priority = get_should_clamp_priority<Traits>::value;
      inline constexpr static bool          inverted_priority = priority_is_inverted<Traits>::value;


      struct accessor_type {
        AGT_forceinline static value_pqueue_element<T>* access(value_pqueue_element_base* next) noexcept {
          return static_cast<value_pqueue_element<T>*>(next);
        }
      };
    };
  }

  template <typename T,
            thread_safety SafetyModel,
            typename PQueueTraits = pqueue_traits<T>>
  class value_pqueue {

    using queue_element = impl::value_pqueue_element<T>;


    using real_traits = impl::value_pqueue_traits<T, PQueueTraits>;

    using intrusive_pqueue_type = intrusive_pqueue<queue_element, SafetyModel, real_traits>;
  public:

    using value_type = T;
    using priority_type = typename intrusive_pqueue_type::priority_type;

  private:

    using queue_object  = impl::value_pqueue_object<T, priority_type>;

    inline constexpr static size_t ObjectSize = sizeof(queue_object);



    inline constexpr static size_t BlocksPerChunk = 4096 / ObjectSize;

  public:

    value_pqueue() {
      m_pool     = make_private_pool(ObjectSize, BlocksPerChunk);
      m_realPool = &m_pool->pool;
      m_inst     = m_pool->instance;
    }


    void enqueue(value_type value, priority_type prio) noexcept {
      auto obj = static_cast<queue_object*>(impl::raw_pool_alloc(*m_realPool, m_inst));
      obj->priority = prio;
      obj->element.value = value;
      m_pqueue.enqueue(&obj->element, prio);
    }

    [[nodiscard]] value_type dequeue() noexcept {
      auto ptr = m_pqueue.dequeue();
      value_type value = ptr->value;
      release(cast_to_object(ptr));
      return value;
    }

    [[nodiscard]] std::optional<value_type> try_dequeue() noexcept {
      if (auto ptr = m_pqueue.try_dequeue()) {
        value_type value = ptr->value;
        release(cast_to_object(ptr));
        return value;
      }
      return std::nullopt;
    }

    [[nodiscard]] std::optional<value_type> try_dequeue_for(agt_timeout_t timeout) noexcept {
      if (auto ptr = m_pqueue.try_dequeue_for(timeout)) {
        value_type value = ptr->value;
        release(cast_to_object(ptr));
        return value;
      }
      return std::nullopt;
    }

    [[nodiscard]] std::optional<value_type> try_dequeue_until(deadline dl) noexcept {
      if (auto ptr = m_pqueue.try_dequeue_until(dl)) {
        value_type value = ptr->value;
        release(cast_to_object(ptr));
        return value;
      }
      return std::nullopt;
    }

  private:

    [[nodiscard]] AGT_forceinline static queue_object* cast_to_object(queue_element* el) noexcept {
      return reinterpret_cast<queue_object*>(reinterpret_cast<char*>(el) - offsetof(queue_object, element));
    }

    private_object_pool*  m_pool;
    impl::private_pool*   m_realPool;
    agt_instance_t        m_inst;
    intrusive_pqueue_type m_pqueue;
  };
}

#endif//AGATE_PRIORITY_QUEUE_HPP
