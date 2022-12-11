//
// Created by maxwe on 2022-12-02.
//

#ifndef AGATE_SUPPORT_EPOCH_PTR_HPP
#define AGATE_SUPPORT_EPOCH_PTR_HPP

#include "atomic.hpp"
#include "tagged_value.hpp"

namespace agt {

  template <typename T>
  class atomic_epoch_ptr;

  template <typename T>
  class epoch_ptr {


    friend atomic_epoch_ptr<T>;

  public:

    using tagged_type = tagged_value<T*, agt_u32_t>;
    using epoch_type = agt_u32_t;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;

    epoch_ptr() = default;
    epoch_ptr(tagged_type value) noexcept : m_value(value) { }
    epoch_ptr(pointer val, agt_u32_t epoch) noexcept : m_value(val, epoch) { }

    [[nodiscard]] pointer   operator->() const noexcept {
      return m_value.value();
    }
    [[nodiscard]] reference operator*() const noexcept {
      return *m_value.value();
    }

    [[nodiscard]] pointer    get()   const noexcept {
      return m_value.value();
    }
    [[nodiscard]] epoch_type epoch() const noexcept {
      return m_value.tag();
    }

    void set(pointer p) noexcept {
      m_value.value() = p;
    }

    void setEpoch(epoch_type epoch) noexcept {
      m_value.tag() = epoch;
    }


    [[nodiscard]]       pointer&  ptr()       &  noexcept {
      return m_value.value();
    }
    [[nodiscard]] const pointer&  ptr() const &  noexcept {
      return m_value.value();
    }
    [[nodiscard]]       pointer&& ptr()       && noexcept {
      return std::move(m_value.value());
    }
    [[nodiscard]] const pointer&& ptr() const && noexcept {
      return std::move(m_value.value());
    }

  private:
    tagged_type m_value;
  };

  template <typename T>
  class atomic_epoch_ptr {
    using tagged_type = tagged_atomic<T*, agt_u32_t>;
  public:

    using stored_type = epoch_ptr<T>;
    using epoch_type = agt_u32_t;
    using value_type = T;
    using pointer    = T*;

    atomic_epoch_ptr() = default;
    atomic_epoch_ptr(stored_type value) noexcept : m_value(value.m_value) { }
    atomic_epoch_ptr(pointer p, epoch_type epoch) noexcept : m_value(p, epoch) { }


    [[nodiscard]] stored_type load() const noexcept {
      return m_value.load();
    }

    [[nodiscard]] stored_type relaxedLoad() const noexcept {
      return m_value.relaxedLoad();
    }

    void        store(stored_type newValue) noexcept {
      m_value.store(newValue.m_value);
    }

    void        relaxedStore(stored_type tagged) noexcept {
      m_value.relaxedStore(tagged.m_value);
    }

    [[nodiscard]] stored_type exchange(stored_type newValue) noexcept {
      return m_value.exchange(newValue.m_value);
    }

    // May be called with a value_type second argument,
    // given that tagged_type is implicitly convertible from value_type.
    bool        compareAndSwap(stored_type& expectedOrCaptured, stored_type newValue) noexcept {
      return m_value.compareAndSwap(expectedOrCaptured.m_value, newValue.m_value);
    }
    bool        compareAndSwapWeak(stored_type& expectedOrCaptured, stored_type newValue) noexcept {
      return m_value.compareAndSwapWeak(expectedOrCaptured.m_value, newValue.m_value);
    }



    void wakeOne() const noexcept;

    void wakeAll() const noexcept;

    void wait(agt_u32_t epoch) const noexcept;

    bool waitUntil(agt_u32_t epoch, deadline dl) const noexcept;

    void waitAndLoad(stored_type& value) const noexcept;

    bool waitAndLoadUntil(stored_type& value, deadline dl) const noexcept;

    [[nodiscard]] epoch_type epoch() const noexcept {
      return m_value.load();
    }


  private:
    tagged_type m_value;
  };

}

#endif//AGATE_SUPPORT_EPOCH_PTR_HPP
