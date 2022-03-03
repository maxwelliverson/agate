//
// Created by maxwe on 2022-02-11.
//

#ifndef JEMSYS_AGATE2_INTERNAL_ALIGN_HPP
#define JEMSYS_AGATE2_INTERNAL_ALIGN_HPP

#include "agate.h"


namespace Agt {
  AGT_forceinline AgtSize alignSize(AgtSize size, AgtSize align) noexcept {
    return ((size - 1) | (align - 1)) + 1;
  }
}

#endif //JEMSYS_AGATE2_INTERNAL_ALIGN_HPP
