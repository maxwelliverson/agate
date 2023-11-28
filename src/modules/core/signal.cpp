//
// Created by maxwe on 2021-12-07.
//

#include "signal.hpp"
#include "async.hpp"


extern "C" {

struct agt_signal_st : agt::object {
  agt::async_key_t  asyncKey;
  agt_ctx_t         context;
  agt::async_data_t asyncData;
};


}


agt_async_data_t agt::signal_get_attachment(const agt_signal_st *signal) noexcept {
  return signal->asyncData;
}

void agt::signal_attach(agt_signal_t signal, agt_async_t& async) noexcept {
  // TODO: Implement Agt::signalAttach
}

void agt::signal_detach(agt_signal_t signal) noexcept {
  // TODO: Implement Agt::signalDetach
}
void agt::signalRaise(agt_signal_t signal) noexcept {
  // TODO: Implement Agt::signalRaise
}
void agt::signalClose(agt_signal_t signal) noexcept {
  // TODO: Implement Agt::signalClose
}
