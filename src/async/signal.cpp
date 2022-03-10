//
// Created by maxwe on 2021-12-07.
//

#include "signal.hpp"
#include "async.hpp"


extern "C" {

struct agt_signal_st {
  agt_ctx_t        context;
  agt_async_data_t asyncData;
  agt_u32_t        dataKey;
};


}


agt_async_data_t Agt::signalGetAttachment(const agt_signal_st* signal) noexcept {
  return signal->asyncData;
}

void Agt::signalAttach(agt_signal_t signal, agt_async_t async) noexcept {
  // TODO: Implement Agt::signalAttach
}

void Agt::signalDetach(agt_signal_t signal) noexcept {
  // TODO: Implement Agt::signalDetach
}
void Agt::signalRaise(agt_signal_t signal) noexcept {
  // TODO: Implement Agt::signalRaise
}
void Agt::signalClose(agt_signal_t signal) noexcept {
  // TODO: Implement Agt::signalClose
}
