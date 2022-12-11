//
// Created by maxwe on 2022-12-02.
//

#include "sender.hpp"
#include "message_queue.hpp"



agt_status_t agt::sendLocalSPSCQueue(sender_t sender,  agt_message_t message) noexcept {
  AGT_assert( sender->kind == local_spsc_sender_kind );
  auto s = static_cast<agt::local_spsc_sender*>(sender);

  std::atomic_thread_fence(std::memory_order_acquire);

  if (s->receiverIsDiscarded) [[unlikely]]
    return AGT_ERROR_NO_RECEIVERS;


  message->next.set(nullptr);

  agt_message_st** tail = std::exchange(s->tail, &message->next.ptr());
  // s->tail = &message->next.ptr();
  std::atomic_thread_fence(std::memory_order_release);
  atomicStore(*tail, message);

  return AGT_SUCCESS;
}
agt_status_t agt::sendLocalMPSCQueue(sender_t sender,  agt_message_t message) noexcept {

}
agt_status_t agt::sendLocalSPMCQueue(sender_t sender,  agt_message_t message) noexcept;
agt_status_t agt::sendLocalMPMCQueue(sender_t sender,  agt_message_t message) noexcept;
agt_status_t agt::sendSharedSPSCQueue(sender_t sender, agt_message_t message) noexcept;
agt_status_t agt::sendSharedMPSCQueue(sender_t sender, agt_message_t message) noexcept;
agt_status_t agt::sendSharedSPMCQueue(sender_t sender, agt_message_t message) noexcept;
agt_status_t agt::sendSharedMPMCQueue(sender_t sender, agt_message_t message) noexcept;
agt_status_t agt::sendPrivateQueue(sender_t sender,    agt_message_t message) noexcept;
agt_status_t agt::sendSpLocalBQueue(sender_t sender,   agt_message_t message) noexcept;
agt_status_t agt::sendMpLocalBQueue(sender_t sender,   agt_message_t message) noexcept;
agt_status_t agt::sendSpSharedBQueue(sender_t sender,  agt_message_t message) noexcept;
agt_status_t agt::sendMpSharedBQueue(sender_t sender,  agt_message_t message) noexcept;