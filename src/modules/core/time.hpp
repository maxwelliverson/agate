//
// Created by maxwe on 2024-04-16.
//

#ifndef AGATE_INTERNAL_CORE_TIME_HPP
#define AGATE_INTERNAL_CORE_TIME_HPP

#include "config.hpp"
#include "agate/integer_division.hpp"
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
    return multiply(static_cast<agt_u64_t>(hwnow()), instance->tscToNs);
  }

  [[nodiscard]] AGT_forceinline agt_timestamp_t now(agt_ctx_t ctx) noexcept {
    return now(ctx->instance);
  }



  [[nodiscard]] AGT_forceinline hwtime         add_duration(agt_instance_t instance, hwtime time, agt_duration_t duration) noexcept {
    return static_cast<hwtime>(std::to_underlying(time) + multiply(duration, instance->nsToTsc));
  }

  [[nodiscard]] AGT_forceinline hwtime         add_duration(agt_ctx_t ctx, hwtime time, agt_duration_t duration) noexcept {
    return add_duration(ctx->instance, time, duration);
  }

  [[nodiscard]] AGT_forceinline hwtime         minus_duration(agt_instance_t instance, hwtime time, agt_duration_t duration) noexcept {
    return static_cast<hwtime>(std::to_underlying(time) - multiply(duration, instance->nsToTsc));
  }

  [[nodiscard]] AGT_forceinline hwtime         minus_duration(agt_ctx_t ctx, hwtime time, agt_duration_t duration) noexcept {
    return minus_duration(ctx->instance, time, duration);
  }



  [[nodiscard]] AGT_forceinline agt_duration_t duration_between(agt_instance_t instance, hwtime start, hwtime end) noexcept {
    return multiply(std::to_underlying(end) - std::to_underlying(start), instance->tscToNs);
  }

  [[nodiscard]] AGT_forceinline agt_duration_t duration_between(agt_ctx_t ctx, hwtime start, hwtime end) noexcept {
    return duration_between(ctx->instance, start, end);
  }
}

#endif//AGATE_INTERNAL_CORE_TIME_HPP
