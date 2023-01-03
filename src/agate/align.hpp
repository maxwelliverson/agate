//
// Created by maxwe on 2022-02-11.
//

#ifndef JEMSYS_AGATE2_INTERNAL_ALIGN_HPP
#define JEMSYS_AGATE2_INTERNAL_ALIGN_HPP

#include "agate.h"

#include <concepts>
#include <bit>

namespace agt {

  template <std::unsigned_integral I>
  AGT_forceinline static constexpr bool   is_pow2_or_zero(I val) noexcept {
    return (val & (val - 1)) == 0;
  }

  template <std::unsigned_integral I>
  AGT_forceinline static constexpr bool   is_pow2(I val) noexcept {
    return val != 0 && is_pow2_or_zero(val);
  }


  AGT_forceinline static constexpr size_t align_to(size_t size, size_t align) noexcept {
    return ((size - 1) | (align - 1)) + 1;
  }

  template <size_t Align>
  AGT_forceinline static constexpr size_t align_to(size_t size) noexcept {
    return ((size - 1) | (Align - 1)) + 1;
  }

}

#endif //JEMSYS_AGATE2_INTERNAL_ALIGN_HPP
