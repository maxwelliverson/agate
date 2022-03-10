//
// Created by maxwe on 2021-11-29.
//

#ifndef JEMSYS_AGATE2_SIGNAL_HPP
#define JEMSYS_AGATE2_SIGNAL_HPP

#include "fwd.hpp"

namespace Agt {

  agt_async_data_t signalGetAttachment(const agt_signal_st* signal) noexcept;

  void         signalAttach(agt_signal_t signal, agt_async_t async) noexcept;

  void         signalDetach(agt_signal_t signal) noexcept;

  void         signalRaise(agt_signal_t signal) noexcept;

  void         signalClose(agt_signal_t signal) noexcept;

}

#endif//JEMSYS_AGATE2_SIGNAL_HPP
