//
// Created by maxwe on 2022-12-02.
//

#ifndef AGATE_RECEIVER_HPP
#define AGATE_RECEIVER_HPP

#include "config.hpp"
#include "core/object.hpp"

namespace agt {

  struct receiver;

  enum receiver_kind_t {
    local_spsc_receiver_kind,
    local_mpsc_receiver_kind,
    local_spmc_receiver_kind,
    local_mpmc_receiver_kind,
    shared_spsc_receiver_kind,
    shared_mpsc_receiver_kind,
    shared_spmc_receiver_kind,
    shared_mpmc_receiver_kind,
    private_receiver_kind,
    local_sp_bqueue_receiver_kind,
    local_mp_bqueue_receiver_kind,
    shared_sp_bqueue_receiver_kind,
    shared_mp_bqueue_receiver_kind,
    receiver_kind_max_enum
  };

  struct receiver : object {
    /*receiver_kind_t kind;
    agt_u32_t       flags;*/
  };


  agt_status_t receiveLocalSPSCQueue(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receiveLocalMPSCQueue(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receiveLocalSPMCQueue(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receiveLocalMPMCQueue(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receiveSharedSPSCQueue(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receiveSharedMPSCQueue(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receiveSharedSPMCQueue(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receiveSharedMPMCQueue(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receivePrivateQueue(receiver_t receiver,    agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receiveSpLocalBQueue(receiver_t receiver,   agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receiveMpLocalBQueue(receiver_t receiver,   agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receiveSpSharedBQueue(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receiveMpSharedBQueue(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;

  
  inline agt_status_t receive(receiver_t r, agt_message_t& message, agt_timeout_t timeout) noexcept {
    using receive_pfn = agt_status_t(*)(receiver_t, agt_message_t&, agt_timeout_t);
    constexpr static receive_pfn jmp_table[] = {
        receiveLocalSPSCQueue,
        receiveLocalMPSCQueue,
        receiveLocalSPMCQueue,
        receiveLocalMPMCQueue,
        receiveSharedSPSCQueue,
        receiveSharedMPSCQueue,
        receiveSharedSPMCQueue,
        receiveSharedMPMCQueue,
        receivePrivateQueue,
        receiveSpLocalBQueue,
        receiveMpLocalBQueue,
        receiveSpSharedBQueue,
        receiveMpSharedBQueue
    };
    AGT_assert_is_type(r, receiver);
    return (jmp_table[AGT_get_type_index(r, receiver)])(r, message, timeout);
  }
}

#endif//AGATE_RECEIVER_HPP
