//
// Created by maxwe on 2021-11-29.
//

#ifndef JEMSYS_AGATE2_SIGNAL_HPP
#define JEMSYS_AGATE2_SIGNAL_HPP

#include "config.hpp"

namespace agt {

  agt_async_data_t signal_get_attachment(const agt_signal_st* signal) noexcept;

  void             signal_attach(agt_signal_t signal, agt_async_t& async) noexcept;

  void             signal_detach(agt_signal_t signal) noexcept;

  void             signal_raise(agt_signal_t signal) noexcept;

  void             signal_close(agt_signal_t signal) noexcept;

}

#endif//JEMSYS_AGATE2_SIGNAL_HPP
