//
// Created by maxwe on 2022-12-02.
//

#ifndef AGATE_SENDER_HPP
#define AGATE_SENDER_HPP

#include "fwd.hpp"
#include "core/objects.hpp"

namespace agt {

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
  };

  struct sender : object {
    // sender_kind_t kind;
    // agt_u32_t     flags;
  };



  agt_status_t sendLocalSPSCQueue(sender_t sender,  agt_message_t message) noexcept;
  agt_status_t sendLocalMPSCQueue(sender_t sender,  agt_message_t message) noexcept;
  agt_status_t sendLocalSPMCQueue(sender_t sender,  agt_message_t message) noexcept;
  agt_status_t sendLocalMPMCQueue(sender_t sender,  agt_message_t message) noexcept;
  agt_status_t sendSharedSPSCQueue(sender_t sender, agt_message_t message) noexcept;
  agt_status_t sendSharedMPSCQueue(sender_t sender, agt_message_t message) noexcept;
  agt_status_t sendSharedSPMCQueue(sender_t sender, agt_message_t message) noexcept;
  agt_status_t sendSharedMPMCQueue(sender_t sender, agt_message_t message) noexcept;
  agt_status_t sendPrivateQueue(sender_t sender,    agt_message_t message) noexcept;
  agt_status_t sendLocalSpBQueue(sender_t sender,   agt_message_t message) noexcept;
  agt_status_t sendLocalMpBQueue(sender_t sender,   agt_message_t message) noexcept;
  agt_status_t sendSharedSpBQueue(sender_t sender,  agt_message_t message) noexcept;
  agt_status_t sendSharedMpBQueue(sender_t sender,  agt_message_t message) noexcept;

  inline agt_status_t send(sender_t s, agt_message_t message) noexcept {
    using send_pfn = agt_status_t(*)(sender_t, agt_message_t);
    constexpr static send_pfn jmp_table[] = {
        sendLocalSPSCQueue,
        sendLocalMPSCQueue,
        sendLocalSPMCQueue,
        sendLocalMPMCQueue,
        sendSharedSPSCQueue,
        sendSharedMPSCQueue,
        sendSharedSPMCQueue,
        sendSharedMPMCQueue,
        sendPrivateQueue,
        sendLocalSpBQueue,
        sendLocalMpBQueue,
        sendSharedSpBQueue,
        sendSharedMpBQueue
    };
    AGT_assert_is_type(s, sender);
    return (jmp_table[AGT_get_type_index(s, sender)])(s, message);
  }
}

#endif//AGATE_SENDER_HPP
