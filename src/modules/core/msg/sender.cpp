//
// Created by maxwe on 2022-12-02.
//

#include "sender.hpp"
#include "message_queue.hpp"



agt_status_t agt::send_local_spsc(sender_t sender, agt_message_t message) noexcept {
  AGT_invariant( message != nullptr );

  auto s = static_cast<local_spsc_sender*>(sender);
  AGT_assert_is_a(s, local_spsc_sender);
  // AGT_assert( AGT_queue_kind(queue) == local_spsc_queue_kind );

  std::atomic_thread_fence(std::memory_order_acquire);

  message->next = nullptr;



  auto tail = s->tail;
  s->tail = &message->next;
  std::atomic_thread_fence(std::memory_order_acq_rel);
  atomic_store(*tail, message);

  return AGT_SUCCESS;
}


namespace {
  AGT_noinline void close_mpsc_sender_and_receiver(local_mpsc_sender* s) noexcept {
    s->receiver.reset();
    release(s); // for now, simply close sender
  }
}




agt_status_t agt::send_local_mpsc(sender_t sender,  agt_message_t message) noexcept {
  auto s = static_cast<local_mpsc_sender*>(sender);
  AGT_assert_is_a(s, local_mpsc_sender);

  message->next = nullptr;

  if (atomic_load(s->receiver->isAlive) == AGT_FALSE) [[unlikely]] {
    close_mpsc_sender_and_receiver(s);
    return AGT_ERROR_NO_RECEIVERS;
  }

  auto tail = atomic_exchange(s->receiver->tail, &message->next);
  atomic_store(*tail, message);

  return AGT_SUCCESS;
}
agt_status_t agt::send_local_spmc(sender_t sender,  agt_message_t msg) noexcept {
  auto s = static_cast<local_spmc_sender*>(sender);
  AGT_assert_is_a(s, local_spmc_sender);

  msg->next = nullptr;

  if (atomic_relaxed_load(s->receiverCount) == 0)
    return AGT_ERROR_NO_RECEIVERS;

  // This can probably be optimized
  auto tail = atomic_exchange(s->tail, &msg->next);
  atomic_store(*tail, msg);

  return AGT_SUCCESS;
}
agt_status_t agt::send_local_mpmc(sender_t sender,  agt_message_t message) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::send_shared_spsc(sender_t sender, agt_message_t message) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::send_shared_mpsc(sender_t sender, agt_message_t message) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::send_shared_spmc(sender_t sender, agt_message_t message) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::send_shared_mpmc(sender_t sender, agt_message_t message) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}


agt_status_t agt::send_private_queue(sender_t sender, agt_message_t msg) noexcept {
  auto s = static_cast<agt::private_sender*>(sender);
  AGT_assert_is_a( s, private_sender );

  if (!s->receiver)
    return AGT_ERROR_NO_RECEIVERS;

  msg->next = nullptr;

  *s->tail = msg;
  s->tail = &msg->next;

  return AGT_SUCCESS;
}

agt_status_t agt::send_local_sp_broadcast(sender_t sender,   agt_message_t message) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::send_local_mp_broadcast(sender_t sender,   agt_message_t message) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::send_shared_sp_broadcast(sender_t sender,  agt_message_t message) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::send_shared_mp_broadcast(sender_t sender,  agt_message_t message) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}