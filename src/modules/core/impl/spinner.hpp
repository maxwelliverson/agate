//
// Created by maxwe on 2024-04-16.
//

#ifndef AGATE_INTERNAL_CORE_IMPL_SPINNER_HPP
#define AGATE_INTERNAL_CORE_IMPL_SPINNER_HPP

#include "config.hpp"

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace agt {
  inline static bool yield(agt_ctx_t ctx) noexcept;

  namespace impl {
    /*enum class rate {
      quadratic,
      linear,
    };

    inline constexpr static rate eQuadraticRate = rate::quadratic;
    inline constexpr static rate eLinearRate = rate::linear;


    template <uint32_t N = 6, rate IntervalGrowthRate = eQuadraticRate>
    class spinner {
      AGT_forceinline void spin_quadratic(uint32_t count) noexcept {
        if (count == 0)
          YieldProcessor();
        else {
          spin_quadratic(count - 1);
          spin_quadratic(count - 1);
        }
      }

      AGT_forceinline void spin_linear(uint32_t count) noexcept {

      }

    public:

      inline void spin(agt_ctx_t ctx) noexcept {
        if (m_backoff >= N) {
          if (yield(ctx)) {
            m_backoff = 0;
            return;
          }
        }

        if constexpr (IntervalGrowthRate == eQuadraticRate) {

        }
      }


    private:
      uint32_t m_backoff = 0;
    };*/

    class spinner {
    public:
      spinner() = default;


      AGT_noinline void spin(agt_ctx_t ctx) noexcept {
        switch (backoff) {
          default:
            if (yield(ctx)) {
              backoff = 0;
              return;
            }
            [[fallthrough]];
          case 6:
            PAUSE_x32();
            [[fallthrough]];
          case 5:
            PAUSE_x16();
            [[fallthrough]];
          case 4:
            PAUSE_x8();
            [[fallthrough]];
          case 3:
            PAUSE_x4();
            [[fallthrough]];
          case 2:
            PAUSE_x2();
            [[fallthrough]];
          case 1:
            PAUSE_x1();
            [[fallthrough]];
          case 0:
            PAUSE_x1();
        }
        ++backoff;
      }

    private:
      uint32_t backoff = 0;
    };
  }
}

#endif//AGATE_INTERNAL_CORE_IMPL_SPINNER_HPP
