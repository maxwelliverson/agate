//
// Created by maxwe on 2021-12-07.
//

#include "signal.hpp"
#include "async.hpp"


extern "C" {

struct AgtSignal_st {
  AgtContext   context;
  AgtAsyncData asyncData;
  AgtUInt32    dataKey;
};


}


AgtAsyncData Agt::signalGetAttachment(const AgtSignal_st* signal) noexcept {
  return signal->asyncData;
}

void Agt::signalAttach(AgtSignal signal, AgtAsync async) noexcept {
  // TODO: Implement Agt::signalAttach
}

void Agt::signalDetach(AgtSignal signal) noexcept {
  // TODO: Implement Agt::signalDetach
}
void Agt::signalRaise(AgtSignal signal) noexcept {
  // TODO: Implement Agt::signalRaise
}
void Agt::signalClose(AgtSignal signal) noexcept {
  // TODO: Implement Agt::signalClose
}
