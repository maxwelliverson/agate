//
// Created by maxwe on 2022-02-19.
//

#ifndef JEMSYS_INTERNAL_AGATE2_ERROR_HPP
#define JEMSYS_INTERNAL_AGATE2_ERROR_HPP

#include "../fwd.hpp"

namespace Agt {

  enum class ErrorState : AgtUInt32 {
    noReceivers,
    noSenders
  };
}

#endif //JEMSYS_INTERNAL_AGATE2_ERROR_HPP
