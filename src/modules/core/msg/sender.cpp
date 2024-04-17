//
// Created by maxwe on 2022-12-02.
//

#include "sender.hpp"
#include "message_queue.hpp"



agt_status_t agt::send_local_spsc(sender_t sender, agt_message_t message) noexcept {
  auto s = static_cast<local_spsc_sender*>(sender);
  AGT_assert_is_a(s, local_spsc_sender);
  // AGT_assert( AGT_queue_kind(queue) == local_spsc_queue_kind );


  std::atomic_thread_fence(std::memory_order_acquire);

  auto& msg = *reinterpret_cast<agt::message*>(&message);

  msg.next() = nullptr;

  auto tail = s->tail;
  s->tail = &msg.next();
  std::atomic_thread_fence(std::memory_order_acq_rel);
  atomicStore(tail->c_ref(), message);

  return AGT_SUCCESS;
}
agt_status_t agt::send_local_mpsc(sender_t sender,  agt_message_t message) noexcept {
  auto s = static_cast<local_mpsc_sender*>(sender);
  AGT_assert_is_a(s, local_mpsc_sender);

  agt::message msg = message;
  msg.next() = nullptr;

  if (auto queue = s->receiver.actualize(s->receiverEpoch)) [[likely]] {
    // While seemingly overly simplistic, this actually works fine.
    // Doing the updates in this order ensures consistency, because the receiver never cares about the value of queue->tail.
    auto tail = atomicExchange(queue->tail, &msg.next());
    atomicStore(tail->c_ref(), message);

    return AGT_SUCCESS;
  }

  // uh oh, no more receiver :(

  return AGT_ERROR_NO_RECEIVERS;

  // message->next = nullptr;

  // basic_message** expectedTail = &q->head;
  // while (!atomicCompareExchange(q->tail, expectedTail, &message->next));
  // atomicStore(*expectedTail, message);


}
agt_status_t agt::send_local_spmc(sender_t sender,  agt_message_t message) noexcept {
  auto s = static_cast<local_spmc_sender*>(sender);
  AGT_assert_is_a(s, local_spmc_sender);

  agt::message msg = message;
  msg.next() = nullptr;

  if (atomicRelaxedLoad(s->receiverCount) == 0)
    return AGT_ERROR_NO_RECEIVERS;

  // This can probably be optimized
  auto tail = atomicExchange(s->tail, &msg.next());
  atomicStore(tail->c_ref(), message);

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


agt_status_t agt::send_private_queue(sender_t sender, agt_message_t message) noexcept {
  auto s = static_cast<agt::private_sender*>(sender);
  AGT_assert_is_a( s, private_sender );

  if (!s->receiver)
    return AGT_ERROR_NO_RECEIVERS;

  agt::message msg = message;

  msg.next() = nullptr;

  *s->tail = msg;
  s->tail = &msg.next();

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