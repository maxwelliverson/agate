//
// Created by maxwe on 2024-04-16.
//

#ifndef AGATE_INTERNAL_CORE_TIME_HPP
#define AGATE_INTERNAL_CORE_TIME_HPP

#include "agate/ratio_multiplier.hpp"
#include "config.hpp"
#include "instance.hpp"

#include <immintrin.h>

namespace agt {

  enum class hwtime : uint64_t;

  // Returns hardware time; not standardized
  [[nodiscard]] AGT_forceinline hwtime          hwnow() noexcept {
    return static_cast<hwtime>(__rdtsc());
  }

  // Returns time in ns
  [[nodiscard]] AGT_forceinline agt_timestamp_t now(agt_instance_t instance) noexcept {
    return multiply(static_cast<agt_u64_t>(hwnow()), instance->clockToNsRatio);
  }

  [[nodiscard]] AGT_forceinline agt_timestamp_t now(agt_ctx_t ctx) noexcept {
    return now(get_instance(ctx));
  }



  [[nodiscard]] AGT_forceinline hwtime         add_duration(agt_instance_t instance, hwtime time, agt_duration_t duration) noexcept {
    return static_cast<hwtime>(std::to_underlying(time) + multiply(duration, instance->nsToClockRatio));
  }

  [[nodiscard]] AGT_forceinline hwtime         add_duration(agt_ctx_t ctx, hwtime time, agt_duration_t duration) noexcept {
    return add_duration(get_instance(ctx), time, duration);
  }

  [[nodiscard]] AGT_forceinline hwtime         minus_duration(agt_instance_t instance, hwtime time, agt_duration_t duration) noexcept {
    return static_cast<hwtime>(std::to_underlying(time) - multiply(duration, instance->nsToClockRatio));
  }

  [[nodiscard]] AGT_forceinline hwtime         minus_duration(agt_ctx_t ctx, hwtime time, agt_duration_t duration) noexcept {
    return minus_duration(get_instance(ctx), time, duration);
  }



  [[nodiscard]] AGT_forceinline agt_duration_t duration_between(agt_instance_t instance, hwtime start, hwtime end) noexcept {
    return multiply(std::to_underlying(end) - std::to_underlying(start), instance->clockToNsRatio);
  }

  [[nodiscard]] AGT_forceinline agt_duration_t duration_between(agt_ctx_t ctx, hwtime start, hwtime end) noexcept {
    return duration_between(get_instance(ctx), start, end);
  }


  [[nodiscard]] AGT_forceinline hwtime          hwdeadline(agt_instance_t instance, agt_timeout_t timeout) noexcept {
    return add_duration(instance, hwnow(), timeout);
  }

  [[nodiscard]] AGT_forceinline hwtime          hwdeadline(agt_ctx_t ctx, agt_timeout_t timeout) noexcept {
    return hwdeadline(get_instance(ctx), timeout);
  }


  [[nodiscard]] AGT_forceinline hwtime          to_hwtime(agt_instance_t instance, agt_timestamp_t timestamp) noexcept {
    return static_cast<hwtime>(multiply(timestamp, instance->nsToClockRatio));
  }

  [[nodiscard]] AGT_forceinline hwtime          to_hwtime(agt_ctx_t ctx, agt_timestamp_t timestamp) noexcept {
    return to_hwtime(get_instance(ctx), timestamp);
  }

  [[nodiscard]] AGT_forceinline agt_timestamp_t to_timestamp(agt_instance_t instance, hwtime time) noexcept {
    return multiply(std::to_underlying(time), instance->clockToNsRatio);
  }

  [[nodiscard]] AGT_forceinline agt_timestamp_t to_timestamp(agt_ctx_t ctx, hwtime time) noexcept {
    return to_timestamp(get_instance(ctx), time);
  }
}

#endif//AGATE_INTERNAL_CORE_TIME_HPP
