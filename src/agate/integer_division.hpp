//
// Created by maxwe on 2023-07-25.
//

#ifndef AGATE_INTERNAL_INTEGER_DIVISION_HPP
#define AGATE_INTERNAL_INTEGER_DIVISION_HPP

#include "config.hpp"

#include <bit>
#include <limits>

#if defined(_MSC_VER) && !defined(__clang__)
#include <immintrin.h>
#endif

namespace agt {

  struct alignas(32) ratio_multiplier {
    uint64_t mult;
    uint64_t offset;
    uint32_t shift;
    uint32_t flags;
    uint32_t num;
    uint32_t denom;
  };

  namespace impl {
    inline static uint32_t calculate_gcd(uint32_t a, uint32_t b) noexcept {
      // euclid's algorithm
      if (b < a) {
        std::swap(a, b);
      }

      do {
        auto c = b % a;
        if (c == 0)
          return a;
        b = a;
        a = c;
      } while(true);
    }

    inline static uint64_t calculate_gcd(uint64_t a, uint64_t b) noexcept {
      // euclid's algorithm
      if (b < a) {
        std::swap(a, b);
      }

      do {
        auto c = b % a;
        if (c == 0)
          return a;
        b = a;
        a = c;
      } while(true);
    }
  }

  inline static void precompile_ratio(ratio_multiplier& rat, uint32_t num, uint32_t denom) noexcept {
    // Reduce to standard form

    rat.num = num;
    rat.denom = denom;

    rat.offset = 0;

    const auto gcd = impl::calculate_gcd(num, denom);
    if (gcd != 1) {
      num   /= gcd;
      denom /= gcd;
    }



    rat.flags = 0;

    if (std::has_single_bit(denom)) {
      rat.mult   = num;
      rat.offset = 0;
      rat.shift  = std::countr_zero(denom);
      return;
    }


    AGT_assert( (denom & 0x10000000u) == 0 ); // ie. less than 2^31
    const uint64_t m = 63 - std::countl_zero(denom); // Note: countl_zero is applied to the 32 bit value on purpose. This calculates 32 + floor(log2(value)).
    const uint64_t value64 = denom;
    const uint64_t B = (1ULL << 31) / value64;
    const uint64_t a = ((1ULL << (m + 1)) + value64) / ( 2 * value64 );
    const int64_t delta = static_cast<int64_t>(a * value64) - (1LL << m);
    const uint64_t d = B * std::abs(delta);
    rat.shift        = static_cast<uint32_t>(m);
    rat.mult         = a * num;
    rat.offset       = d;
  }

  AGT_forceinline static uint64_t multiply(uint64_t value, const ratio_multiplier& rat) noexcept {
    /*if (rat.flags & eRatioIsTrivial)
    return value * rat.mult;
  if (rat.flags & eRatioShouldAddOne)
    ++value;
  uint64_t hi;
  _umul128(value, rat.mult, &hi);
  return hi >> rat.shift;*/
    return ((value * rat.mult) + rat.offset) >> rat.shift;
  }
}

#endif//AGATE_INTERNAL_INTEGER_DIVISION_HPP
