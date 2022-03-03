//
// Created by maxwe on 2021-11-29.
//

#ifndef JEMSYS_AGATE2_SIGNAL_HPP
#define JEMSYS_AGATE2_SIGNAL_HPP

#include "fwd.hpp"

namespace Agt {

  AgtAsyncData signalGetAttachment(const AgtSignal_st* signal) noexcept;

  void         signalAttach(AgtSignal signal, AgtAsync async) noexcept;

  void         signalDetach(AgtSignal signal) noexcept;

  void         signalRaise(AgtSignal signal) noexcept;

  void         signalClose(AgtSignal signal) noexcept;

}

#endif//JEMSYS_AGATE2_SIGNAL_HPP
