//
// Created by maxwe on 2023-01-13.
//

#include "ring_queue.hpp"
#include "sender.hpp"
#include "receiver.hpp"

#include "debug.hpp"

agt_status_t agt::acquire_local_sp_ring(sender_t sender, size_t messageSize, agt_message_t& message) noexcept {
  AGT_return_not_implemented();
}
agt_status_t agt::acquire_local_mp_ring(sender_t sender, size_t messageSize, agt_message_t& message) noexcept {
  AGT_return_not_implemented();
}
agt_status_t agt::acquire_shared_sp_ring(sender_t sender, size_t messageSize, agt_message_t& message) noexcept {
  AGT_return_not_implemented();
}
agt_status_t agt::acquire_shared_mp_ring(sender_t sender, size_t messageSize, agt_message_t& message) noexcept {
  AGT_return_not_implemented();
}

agt_status_t agt::send_local_sp_ring(sender_t sender,  agt_message_t message) noexcept {
  AGT_return_not_implemented();
}
agt_status_t agt::send_local_mp_ring(sender_t sender,  agt_message_t message) noexcept {
  AGT_return_not_implemented();
}
agt_status_t agt::send_shared_sp_ring(sender_t sender, agt_message_t message) noexcept {
  AGT_return_not_implemented();
}
agt_status_t agt::send_shared_mp_ring(sender_t sender, agt_message_t message) noexcept {
  AGT_return_not_implemented();
}

agt_status_t agt::receive_local_sp_ring(receiver_t receiver,   agt_message_t& message, agt_timeout_t timeout) noexcept {
  AGT_return_not_implemented();
}
agt_status_t agt::receive_local_mp_ring(receiver_t receiver,   agt_message_t& message, agt_timeout_t timeout) noexcept {
  AGT_return_not_implemented();
}
agt_status_t agt::receive_shared_sp_ring(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
  AGT_return_not_implemented();
}
agt_status_t agt::receive_shared_mp_ring(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
  AGT_return_not_implemented();
}

void         agt::retire_local_sp_ring(receiver_t receiver, agt_message_t message) noexcept {
  AGT_not_implemented();
}
void         agt::retire_local_mp_ring(receiver_t receiver, agt_message_t message) noexcept {
  AGT_not_implemented();
}
void         agt::retire_shared_sp_ring(receiver_t receiver, agt_message_t message) noexcept {
  AGT_not_implemented();
}
void         agt::retire_shared_mp_ring(receiver_t receiver, agt_message_t message) noexcept {
  AGT_not_implemented();
}