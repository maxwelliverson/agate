//
// Created by maxwe on 2022-12-02.
//

#include "receiver.hpp"
#include "message_queue.hpp"
#include "message.hpp"

#include "core/wait.hpp"


agt_status_t agt::receive_local_spsc(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept {
  auto r = static_cast<agt::local_spsc_receiver*>(receiver);

  // This can almost certainly be done much more efficiently/simply by storing a blocked queue inline
  auto status = wait_on_local_for(r->ctx, r->head, nullptr, timeout, &message);

  if (status == AGT_SUCCESS) {
    atomicStore(r->head, message->next);
    auto newTail = &message->next;
    atomicCompareExchange(*r->pTail, newTail, &r->head); // ie. the received message is the queue's tail, then set tail to point back to head.
  }

  return status;
}

agt_status_t agt::receive_local_mpsc(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::receive_local_spmc(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::receive_local_mpmc(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::receive_shared_spsc(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::receive_shared_mpsc(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::receive_shared_spmc(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::receive_shared_mpmc(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::receive_private_queue(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept {
  (void)timeout;
  // The queue is thread local, and as such, timeout is redundant; the only thread that
  // could enqueue a new message would be the thread currently waiting for one. In such
  // cases, return immediately if queue is empty, no matter the timeout.

  // Because of this however, the public dequeue API CANNOT guarantee a message will be returned
  // when timeout is infinite and the queue kind is unknown.

  auto r = static_cast<private_receiver*>(receiver);
  AGT_assert_is_a(r, private_receiver);


  auto& msg = *reinterpret_cast<agt::message*>(&message);
  // agt::message msg;

  if ((msg = r->head)) {
    r->head = msg.next();
    if (*r->tail == &msg.next())
      *r->tail = &r->head;
    return AGT_SUCCESS;
  }

  return AGT_ERROR_NO_MESSAGES;
}
agt_status_t agt::receive_local_sp_broadcast(receiver_t receiver,   agt_message_t& message, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::receive_local_mp_broadcast(receiver_t receiver,   agt_message_t& message, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::receive_shared_sp_broadcast(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::receive_shared_mp_broadcast(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}