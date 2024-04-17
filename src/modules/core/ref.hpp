//
// Created by maxwe on 2024-03-18.
//

#ifndef AGATE_CORE_REF_HPP
#define AGATE_CORE_REF_HPP

#include "config.hpp"
#include "object.hpp"

namespace agt {
  template <typename T>
  class weak_ref;
  template <typename T>
  class ref;


  template <typename T>
  class ref {
    friend weak_ref<T>;
  public:

    using pointer   = T*;
    using reference = T&;

    ref() = default;
    ref(const ref&) = delete;
    ref(ref&& other) noexcept : m_ptr(std::exchange(other.m_ptr, nullptr)) { }
    ref(pointer p) noexcept
        : m_ptr(p) {
      static_assert(std::derived_from<T, rc_object>);
    }

    ref(const weak_ref<T>& weak, agt_epoch_t epoch) noexcept;

    ref& operator=(const ref&) = delete;
    ref& operator=(ref&& other) noexcept {
      if (m_ptr)
        agt::release(m_ptr);
      m_ptr = std::exchange(other.m_ptr, nullptr);
      return *this;
    }

    ~ref() {
      if (m_ptr)
        agt::release(m_ptr);
    }


    [[nodiscard]] inline pointer   operator->() const noexcept {
      return m_ptr;
    }
    [[nodiscard]] inline reference operator*()  const noexcept {
      return *m_ptr;
    }


    [[nodiscard]] inline pointer get() const noexcept {
      return m_ptr;
    }

    [[nodiscard]] inline ref clone() const noexcept {
      if (m_ptr)
        agt::retain(m_ptr);
      return ref(m_ptr);
    }

    template <typename ...Args>
    inline void    recycle(Args&& ...args) noexcept {
      AGT_assert( m_ptr != nullptr );
      m_ptr = agt::recycle(m_ptr, std::forward<Args>(args)...);
    }

    inline pointer take() noexcept {
      return std::exchange(m_ptr, nullptr);
    }

    template <typename ...Args>
    inline void    reset(pointer p, Args&& ...args) noexcept {
      if (m_ptr)
        agt::release(m_ptr, std::forward<Args>(args)...);
      m_ptr = p;
    }

    // Disable overload if Args is a single argument that is convertible to pointer. In that case, we wish to select the other overload.
    template <typename ...Args> requires(!(sizeof...(Args) == 1 && (std::convertible_to<Args, pointer> && ...)))
    inline void    reset(Args&& ...args) noexcept {
      if (m_ptr)
        agt::release(m_ptr, std::forward<Args>(args)...);
      m_ptr = nullptr;
    }

    [[nodiscard]] inline explicit operator bool() const noexcept {
      return m_ptr != nullptr;
    }

  private:
    pointer m_ptr = nullptr;
  };

  template <typename T>
  class weak_ref {
    friend ref<T>;

    weak_ref(T* ptr) noexcept : m_ptr(ptr) { }
  public:

    using pointer   = T*;
    using reference = T&;

    weak_ref() = default;
    weak_ref(const weak_ref&) = delete;
    weak_ref(weak_ref&& other) noexcept
      : m_ptr(std::exchange(other.m_ptr, nullptr)) { }

    weak_ref(const ref<T>& obj, agt_epoch_t& key) noexcept
      : weak_ref(obj.get(), key) { }
    weak_ref(pointer ptr, agt_epoch_t& key) noexcept
      : m_ptr(ptr) {
      static_assert(std::derived_from<T, rc_object>);
      key = take_weak_ref(ptr);
    }

    weak_ref& operator=(const weak_ref&) = delete;
    weak_ref& operator=(weak_ref&& other) noexcept {
      if (m_ptr)
        drop_weak_ref(m_ptr);
      m_ptr = std::exchange(other.m_ptr, nullptr);
      return *this;
    }

    ~weak_ref() {
      if (m_ptr)
        drop_weak_ref(m_ptr);
    }


    weak_ref<T> clone(agt_epoch_t epoch) noexcept {
      if (m_ptr && !retain_weak_ref(m_ptr, epoch))
        return nullptr;
      return m_ptr;
    }

    ref<T>      actualize(agt_epoch_t epoch) noexcept {
      return { *this, epoch };
    }


    void        drop() noexcept {
      if (m_ptr) {
        drop_weak_ref(m_ptr);
        m_ptr = nullptr;
      }
    }


    [[nodiscard]] inline explicit operator bool() const noexcept {
      return m_ptr != nullptr;
    }

  private:
    pointer m_ptr = nullptr;
  };


  template <typename T>
  ref<T>::ref(const weak_ref<T>& weak, agt_epoch_t epoch) noexcept
    : m_ptr(static_cast<pointer>(actualize_weak_ref(weak.m_ptr, epoch))) { }

  template <typename To, typename From>
  ref<To> ref_cast(ref<From>&& r) noexcept {
    return static_cast<To*>(r.take());
  }
}

#endif //AGATE_CORE_REF_HPP
