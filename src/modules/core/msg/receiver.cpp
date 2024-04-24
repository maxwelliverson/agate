//
// Created by maxwe on 2022-12-02.
//

#include "receiver.hpp"
#include "message_queue.hpp"
#include "message.hpp"

#include "agate/cast.hpp"

#include "core/wait.hpp"

#include "debug.hpp"

agt_message_t agt::try_receive_local_mpsc(receiver_t receiver) noexcept {
  auto r = unsafe_nonnull_object_cast<agt::local_mpsc_receiver>(receiver);
  if (auto head = atomic_relaxed_load(r->head)) {
    do {
      auto newHead = atomic_relaxed_load(head->next);
      if (newHead == nullptr) { // if the queue only had a single entry
        r->head = nullptr;
        if (atomic_try_replace(r->tail, &head->next, &r->head)) [[likely]] // this will fail if another message is queued between the load of head->next and now, which while unlikely, is possible.
          return head;
        // If failed, reset and try again. By then, the new tail ***SHOULD*** be set.
        _mm_pause(); // this is x64 specific
        continue;
      }
      r->head = newHead;
      std::atomic_thread_fence(std::memory_order::release);
      return head;
    } while(true);
  }
  return nullptr;
}

agt_status_t agt::receive_local_spsc(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept {
  auto r = static_cast<agt::local_spsc_receiver*>(receiver);

  // This can almost certainly be done much more efficiently/simply by storing a blocked queue inline
  auto status = wait_on_local_for(r->ctx, r->head, nullptr, timeout, &message);

  if (status == AGT_SUCCESS) {
    atomic_store(r->head, message->next);
    auto newTail = &message->next;
    atomic_try_replace(*r->pTail, newTail, &r->head); // ie. the received message is the queue's tail, then set tail to point back to head.
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

  // We need to know if the current uexec is fiber based or not;
  // A suspend on a thread-private queue will never wake if it results in the OS thread being suspended.
  // If it is fiber based, we operate as normal.
  // Otherwise, any non-zero timeout will be ignored.


  auto r = static_cast<private_receiver*>(receiver);
  AGT_assert_is_a(r, private_receiver);

  if ((message = r->head)) {
    r->head = message->next;
    if (*r->tail == &message->next)
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