//
// Created by maxwe on 2024-04-06.
//

#ifndef AGATE_INTERNAL_CORE_WAIT_HPP
#define AGATE_INTERNAL_CORE_WAIT_HPP

#include "config.hpp"
#include "agate/atomic.hpp"
#include "agate/time.hpp"

#include <concepts>
#include <bit>

namespace agt {
  using address_predicate_t = bool(*)(agt_ctx_t ctx, const void* address, void* userData) noexcept;


  namespace impl {
    agt_status_t wait_on_local_address_nocheck(agt_ctx_t ctx, const void* address, agt_timeout_t timeout, agt_u64_t value, size_t valueSize) noexcept;

    agt_status_t wait_on_local_address_nocheck(agt_ctx_t ctx, const void* address, agt_timeout_t timeout, address_predicate_t predicate, void* userData) noexcept;
  }


  agt_status_t wait_on_local_address(agt_ctx_t ctx, const void* address, agt_timeout_t timeout, const void* cmpValue, size_t valueSize) noexcept;

  agt_status_t wait_on_local_address(agt_ctx_t ctx, const void* address, agt_timeout_t timeout, address_predicate_t predicate, void* userData) noexcept;


  // Waits for the atomic value at addr to compare unequal to cmpValue, at which point it is returned.
  template <impl::atomic_capable T>
  inline static T wait_on_local(agt_ctx_t ctx, const T& addr, std::type_identity_t<T> cmpValue) noexcept {
    auto value = atomic_relaxed_load(addr);

    while (value == cmpValue) {
      impl::wait_on_local_address_nocheck(ctx, &addr, AGT_WAIT, std::bit_cast<impl::atomic_type_t<T>>(cmpValue), sizeof(T));
      value = atomic_relaxed_load(addr);
    }

    return value;
  }

  template <impl::atomic_capable T>
  inline static agt_status_t wait_on_local_for(agt_ctx_t ctx, const T& addr, std::type_identity_t<T> cmpValue, agt_timeout_t timeout, T* pOut = nullptr) noexcept {

    if (timeout == AGT_WAIT) {
      auto value = wait_on_local(ctx, addr, cmpValue);
      if (pOut)
        *pOut = value;
      return AGT_SUCCESS;
    }

    agt_status_t status = AGT_SUCCESS;
    auto value = atomic_relaxed_load(addr);

    if (value == cmpValue) {

      if (timeout == 0)
        return AGT_NOT_READY;

      agt_timeout_t remainingTimeout = timeout;
      const auto deadline = now() + timeout;

      do {
        status = impl::wait_on_local_address_nocheck(ctx, &addr, remainingTimeout, std::bit_cast<impl::atomic_type_t<T>>(cmpValue), sizeof(T));
        if (status != AGT_SUCCESS)
          return status;
        value = atomic_relaxed_load(addr);
        if (value != cmpValue)
          break;
        const auto currentTime = now();
        if (deadline <= currentTime)
          return AGT_TIMED_OUT;
        remainingTimeout = deadline - currentTime;
      } while(true);
    }

    if (pOut)
      *pOut = value;

    return status;
  }


  template <impl::atomic_capable T, std::predicate<T> Fn>
  inline static T wait_on_local(agt_ctx_t ctx, const T& addr, Fn&& predicate) noexcept {
    auto value = atomic_relaxed_load(addr);

    while (!std::invoke(predicate, value)) {
      // NOTE: It's okay that the callback pointer here points to local memory; this callback will only ever be called within the scope of this function.
      impl::wait_on_local_address_nocheck(ctx, std::addressof(addr), AGT_WAIT, [](agt_ctx_t ctx, const void* addr, void* userData) -> bool {
        auto value = atomic_relaxed_load(*static_cast<const T*>(addr));
        return std::invoke(*static_cast<std::remove_cvref_t<Fn>*>(userData), value);
      }, (void*)std::addressof(predicate));
      value = atomic_relaxed_load(addr);
    }

    return value;
  }

  // Overload if the predicate function needs access to the context.
  template <impl::atomic_capable T, std::predicate<agt_ctx_t, T> Fn>
  inline static T wait_on_local(agt_ctx_t ctx, const T& addr, Fn&& predicate) noexcept {
    auto value = atomic_relaxed_load(addr);

    while (!std::invoke(predicate, ctx, value)) {
      // NOTE: It's okay that the callback pointer here points to local memory; this callback will only ever be called within the scope of this function.
      impl::wait_on_local_address_nocheck(ctx, std::addressof(addr), AGT_WAIT, [](agt_ctx_t ctx, const void* addr, void* userData) -> bool {
        auto value = atomic_relaxed_load(*static_cast<const T*>(addr));
        return std::invoke(*static_cast<std::remove_cvref_t<Fn>*>(userData), ctx, value);
      }, (void*)std::addressof(predicate));
      value = atomic_relaxed_load(addr);
    }

    return value;
  }

  template <impl::atomic_capable T, std::predicate<T> Fn>
  inline static agt_status_t wait_on_local_for(agt_ctx_t ctx, const T& addr, agt_timeout_t timeout, Fn&& predicate, T* pOut = nullptr) noexcept {

    if (timeout == AGT_WAIT) {
      auto value = wait_on_local(ctx, addr, std::forward<Fn>(predicate));
      if (pOut)
        *pOut = value;
      return AGT_SUCCESS;
    }

    agt_status_t status = AGT_SUCCESS;
    auto value = atomic_relaxed_load(addr);

    if (!std::invoke(predicate, value)) {

      if (timeout == 0)
        return AGT_NOT_READY;

      agt_timeout_t remainingTimeout = timeout;
      const auto deadline = now() + timeout;

      do {
        status = impl::wait_on_local_address_nocheck(ctx, std::addressof(addr), remainingTimeout, [](agt_ctx_t ctx, const void* addr, void* userData) -> bool {
          auto value = atomic_relaxed_load(*static_cast<const T*>(addr));
          return std::invoke(*static_cast<std::remove_cvref_t<Fn>*>(userData), value);
        }, (void*)std::addressof(predicate));
        if (status != AGT_SUCCESS)
          return status;
        value = atomic_relaxed_load(addr);
        if (std::invoke(predicate, value))
          break;
        const auto currentTime = now();
        if (deadline <= currentTime)
          return AGT_TIMED_OUT;
        remainingTimeout = deadline - currentTime;
      } while(true);
    }

    if (pOut)
      *pOut = value;

    return status;
  }

  // Overload if the predicate function needs access to the context.
  template <impl::atomic_capable T, std::predicate<agt_ctx_t, T> Fn>
  inline static agt_status_t wait_on_local_for(agt_ctx_t ctx, const T& addr, agt_timeout_t timeout, Fn&& predicate, T* pOut = nullptr) noexcept {

    if (timeout == AGT_WAIT) {
      auto value = wait_on_local(ctx, addr, std::forward<Fn>(predicate));
      if (pOut)
        *pOut = value;
      return AGT_SUCCESS;
    }

    agt_status_t status = AGT_SUCCESS;
    auto value = atomic_relaxed_load(addr);

    if (!std::invoke(predicate, ctx, value)) {

      if (timeout == 0)
        return AGT_NOT_READY;

      agt_timeout_t remainingTimeout = timeout;
      const auto deadline = now() + timeout;

      do {
        status = impl::wait_on_local_address_nocheck(ctx, std::addressof(addr), remainingTimeout, [](agt_ctx_t ctx, const void* addr, void* userData) -> bool {
          auto value = atomic_relaxed_load(*static_cast<const T*>(addr));
          return std::invoke(*static_cast<std::remove_cvref_t<Fn>*>(userData), ctx, value);
        }, (void*)std::addressof(predicate));
        if (status != AGT_SUCCESS)
          return status;
        value = atomic_relaxed_load(addr);
        if (std::invoke(predicate, ctx, value))
          break;
        const auto currentTime = now();
        if (deadline <= currentTime)
          return AGT_TIMED_OUT;
        remainingTimeout = deadline - currentTime;
      } while(true);
    }

    if (pOut)
      *pOut = value;

    return status;
  }


  void wake_single_local(agt_ctx_t ctx, const void* address) noexcept;

  void wake_all_local(agt_ctx_t ctx, const void* address) noexcept;
}

#endif//AGATE_INTERNAL_CORE_WAIT_HPP
