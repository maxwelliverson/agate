//
// Created by maxwe on 2022-02-11.
//

#ifndef JEMSYS_AGATE2_INTERNAL_ALIGN_HPP
#define JEMSYS_AGATE2_INTERNAL_ALIGN_HPP

#include "agate.h"


namespace agt {
  AGT_forceinline size_t alignSize(size_t size, size_t align) noexcept {
    return ((size - 1) | (align - 1)) + 1;
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
