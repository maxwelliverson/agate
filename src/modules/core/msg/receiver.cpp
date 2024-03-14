//
// Created by maxwe on 2022-12-02.
//

#include "receiver.hpp"
#include "message_queue.hpp"


agt_status_t agt::receiveLocalSPSCQueue(receiver_t receiver, message*& message, agt_timeout_t timeout) noexcept {
  auto r = static_cast<agt::local_spsc_receiver*>(receiver);


  if (atomicWaitFor(r->head, message, timeout, nullptr)) {
    atomicStore(r->head, message->next);
    auto newTail = &message->next;
    atomicCompareExchange(*r->pTail, newTail, &r->head); // ie. if the sender's tail points to message->next, then set tail to point to r->head.

    return AGT_SUCCESS;
  }

  return AGT_NOT_READY;
}

agt_message_t agt::receiveLocalMPSCQueue(receiver_t receiver,  agt_timeout_t timeout) noexcept;
agt_message_t agt::receiveLocalSPMCQueue(receiver_t receiver,  agt_timeout_t timeout) noexcept;
agt_message_t agt::receiveLocalMPMCQueue(receiver_t receiver,  agt_timeout_t timeout) noexcept;
agt_message_t agt::receiveSharedSPSCQueue(receiver_t receiver, agt_timeout_t timeout) noexcept;
agt_message_t agt::receiveSharedMPSCQueue(receiver_t receiver, agt_timeout_t timeout) noexcept;
agt_message_t agt::receiveSharedSPMCQueue(receiver_t receiver, agt_timeout_t timeout) noexcept;
agt_message_t agt::receiveSharedMPMCQueue(receiver_t receiver, agt_timeout_t timeout) noexcept;
agt_message_t agt::receivePrivateQueue(receiver_t receiver,    agt_timeout_t timeout) noexcept;
agt_message_t agt::receiveSpLocalBQueue(receiver_t receiver,   agt_timeout_t timeout) noexcept;
agt_message_t agt::receiveMpLocalBQueue(receiver_t receiver,   agt_timeout_t timeout) noexcept;
agt_message_t agt::receiveSpSharedBQueue(receiver_t receiver,  agt_timeout_t timeout) noexcept;
agt_message_t agt::receiveMpSharedBQueue(receiver_t receiver,  agt_timeout_t timeout) noexcept;