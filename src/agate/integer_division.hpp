//
// Created by maxwe on 2023-07-25.
//

#ifndef AGATE_INTERNAL_INTEGER_DIVISION_HPP
#define AGATE_INTERNAL_INTEGER_DIVISION_HPP

#include "config.hpp"

#include <bit>
#include <limits>

namespace agt {
  
  struct integer_divisor {
    agt_u32_t divisor;
    agt_u32_t shift;
    agt_u64_t multiplicand;
    agt_u64_t offset;
  };
  
  
  inline static void set_divisor(integer_divisor& div, agt_u32_t value) noexcept {
    assert(value != 0 );

    div.divisor = value;

    if (std::has_single_bit(value)) {
      div.multiplicand = 1;
      div.offset       = 0;
      div.shift        = std::countr_zero(value);
    }
    else {
      assert( (value & 0x10000000u) == 0 ); // ie. less than 2^31
      const agt_u64_t m = 63 - std::countl_zero(value); // Note: countl_zero is applied to the 32 bit value on purpose. This calculates 32 + floor(log2(value)).
      const agt_u64_t value64 = value;
      const agt_u64_t B = (1ULL << 31) / value64;
      const agt_u64_t a = ((1ULL << (m + 1)) + value64) / ( 2 * value64 );
      const agt_i64_t delta = static_cast<agt_i64_t>(a * value64) - (1LL << m);
      const agt_u64_t d = B * std::abs(delta);
      div.shift        = static_cast<agt_u32_t>(m);
      div.multiplicand = a;
      div.offset       = d;
    }
  }
  
  inline static agt_u64_t divide(agt_u64_t value, const integer_divisor& div) noexcept {
    return ((value * div.multiplicand) + div.offset) >> div.shift;
  }
}

#endif//AGATE_INTERNAL_INTEGER_DIVISION_HPP
