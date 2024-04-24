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
    using tagged_type = tagged_value<T*, agt_u32_t>;
  public:

    using stored_type = epoch_ptr<T>;
    using epoch_type = agt_u32_t;
    using value_type = T;
    using pointer    = T*;

    atomic_epoch_ptr() = default;
    atomic_epoch_ptr(stored_type value) noexcept : m_value(value.m_value) { }
    atomic_epoch_ptr(pointer p, epoch_type epoch) noexcept : m_value(p, epoch) { }




    [[nodiscard]] epoch_type epoch() const noexcept {
      return m_value.tag();
    }


  private:
    tagged_type m_value;
  };

}

#endif//AGATE_SUPPORT_EPOCH_PTR_HPP
