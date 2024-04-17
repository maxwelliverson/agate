//
// Created by maxwe on 2022-12-02.
//

#ifndef AGATE_CORE_MSG_SENDER_HPP
#define AGATE_CORE_MSG_SENDER_HPP

#include "config.hpp"
#include "core/object.hpp"
// #include "core/rc.hpp"


namespace agt {

  /*struct message;

  enum sender_kind_t : agt_u32_t {
    local_spsc_sender_kind,
    local_mpsc_sender_kind,
    local_spmc_sender_kind,
    local_mpmc_sender_kind,
    shared_spsc_sender_kind,
    shared_mpsc_sender_kind,
    shared_spmc_sender_kind,
    shared_mpmc_sender_kind,
    private_sender_kind,
    local_sp_bqueue_sender_kind,
    local_mp_bqueue_sender_kind,
    shared_sp_bqueue_sender_kind,
    shared_mp_bqueue_sender_kind,
    sender_kind_max_enum
  };*/


  /*AGT_abstract_object(sender, ref_counted) {

  };*/

  /*struct sender : object {
    // sender_kind_t kind;
    // agt_u32_t     flags;
  };*/


  // For any of the basic senders, acquire should rather be 'take the msgPool at an offset of 16, and acquire from that'

  agt_status_t acquire_local_spsc(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_local_mpsc(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_local_spmc(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_local_mpmc(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_shared_spsc(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_shared_mpsc(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_shared_spmc(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_shared_mpmc(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_private_queue(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_local_sp_broadcast(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_local_mp_broadcast(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_shared_sp_broadcast(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_shared_mp_broadcast(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_local_sp_ring(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_local_mp_ring(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_shared_sp_ring(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;
  agt_status_t acquire_shared_mp_ring(sender_t sender, size_t messageSize, agt_message_t& message) noexcept;



  agt_status_t send_local_spsc(sender_t sender,     agt_message_t message) noexcept;
  agt_status_t send_local_mpsc(sender_t sender,     agt_message_t message) noexcept;
  agt_status_t send_local_spmc(sender_t sender,     agt_message_t message) noexcept;
  agt_status_t send_local_mpmc(sender_t sender,     agt_message_t message) noexcept;
  agt_status_t send_shared_spsc(sender_t sender,    agt_message_t message) noexcept;
  agt_status_t send_shared_mpsc(sender_t sender,    agt_message_t message) noexcept;
  agt_status_t send_shared_spmc(sender_t sender,    agt_message_t message) noexcept;
  agt_status_t send_shared_mpmc(sender_t sender,    agt_message_t message) noexcept;
  agt_status_t send_private_queue(sender_t sender,  agt_message_t message) noexcept;
  agt_status_t send_local_sp_broadcast(sender_t sender,  agt_message_t message) noexcept;
  agt_status_t send_local_mp_broadcast(sender_t sender,  agt_message_t message) noexcept;
  agt_status_t send_shared_sp_broadcast(sender_t sender, agt_message_t message) noexcept;
  agt_status_t send_shared_mp_broadcast(sender_t sender, agt_message_t message) noexcept;
  agt_status_t send_local_sp_ring(sender_t sender,  agt_message_t message) noexcept;
  agt_status_t send_local_mp_ring(sender_t sender,  agt_message_t message) noexcept;
  agt_status_t send_shared_sp_ring(sender_t sender, agt_message_t message) noexcept;
  agt_status_t send_shared_mp_ring(sender_t sender, agt_message_t message) noexcept;


  inline agt_status_t acquire(sender_t s, size_t messageSize, agt_message_t& message) noexcept {
    using acquire_pfn = agt_status_t(*)(sender_t, size_t, agt_message_t&);
    constexpr static acquire_pfn jmp_table[] = {
        acquire_local_spsc,
        acquire_local_mpsc,
        acquire_local_spmc,
        acquire_local_mpmc,
        acquire_shared_spsc,
        acquire_shared_mpsc,
        acquire_shared_spmc,
        acquire_shared_mpmc,
        acquire_private_queue,
        acquire_local_sp_broadcast,
        acquire_local_mp_broadcast,
        acquire_shared_sp_broadcast,
        acquire_shared_mp_broadcast,
        acquire_local_sp_ring,
        acquire_local_mp_ring,
        acquire_shared_sp_ring,
        acquire_shared_mp_ring,
    };
    const auto obj = reinterpret_cast<object*>(s);
    AGT_assert_is_type(obj, sender);
    return (jmp_table[AGT_get_type_index(obj, sender)])(s, messageSize, message);
  }

  inline agt_status_t send(sender_t s, agt_message_t message) noexcept {
    using send_pfn = agt_status_t(*)(sender_t, agt_message_t);
    constexpr static send_pfn jmp_table[] = {
        send_local_spsc,
        send_local_mpsc,
        send_local_spmc,
        send_local_mpmc,
        send_shared_spsc,
        send_shared_mpsc,
        send_shared_spmc,
        send_shared_mpmc,
        send_private_queue,
        send_local_sp_broadcast,
        send_local_mp_broadcast,
        send_shared_sp_broadcast,
        send_shared_mp_broadcast,
        send_local_sp_ring,
        send_local_mp_ring,
        send_shared_sp_ring,
        send_shared_mp_ring,
    };
    const auto obj = reinterpret_cast<object*>(s);
    AGT_assert_is_type(obj, sender);
    return (jmp_table[AGT_get_type_index(obj, sender)])(s, message);
  }
}

#endif//AGATE_CORE_MSG_SENDER_HPP
