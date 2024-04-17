//
// Created by maxwe on 2022-12-02.
//

#ifndef AGATE_CORE_MSG_RECEIVER_HPP
#define AGATE_CORE_MSG_RECEIVER_HPP

#include "config.hpp"
#include "core/object.hpp"
// #include "core/rc.hpp"

namespace agt {

  agt_status_t receive_local_spsc(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_local_mpsc(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_local_spmc(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_local_mpmc(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_shared_spsc(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_shared_mpsc(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_shared_spmc(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_shared_mpmc(receiver_t receiver, agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_private_queue(receiver_t receiver,    agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_local_sp_broadcast(receiver_t receiver,   agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_local_mp_broadcast(receiver_t receiver,   agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_shared_sp_broadcast(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_shared_mp_broadcast(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_local_sp_ring(receiver_t receiver,   agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_local_mp_ring(receiver_t receiver,   agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_shared_sp_ring(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;
  agt_status_t receive_shared_mp_ring(receiver_t receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept;


  // IMPORTANT: Unlike the public API call agt_retire_msg, this does *not* signal the sender. That must be done explicitly.
  void         retire_local_spsc(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_local_mpsc(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_local_spmc(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_local_mpmc(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_shared_spsc(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_shared_mpsc(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_shared_spmc(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_shared_mpmc(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_private_queue(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_local_sp_broadcast(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_local_mp_broadcast(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_shared_sp_broadcast(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_shared_mp_broadcast(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_local_sp_ring(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_local_mp_ring(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_shared_sp_ring(receiver_t receiver, agt_message_t message) noexcept;
  void         retire_shared_mp_ring(receiver_t receiver, agt_message_t message) noexcept;


  
  inline agt_status_t receive(receiver_t r, agt_message_t& message, agt_timeout_t timeout) noexcept {
    using receive_pfn = agt_status_t(*)(receiver_t, agt_message_t&, agt_timeout_t);
    constexpr static receive_pfn jmp_table[] = {
        receive_local_spsc,
        receive_local_mpsc,
        receive_local_spmc,
        receive_local_mpmc,
        receive_shared_spsc,
        receive_shared_mpsc,
        receive_shared_spmc,
        receive_shared_mpmc,
        receive_private_queue,
        receive_local_sp_broadcast,
        receive_local_mp_broadcast,
        receive_shared_sp_broadcast,
        receive_shared_mp_broadcast,
        receive_local_sp_ring,
        receive_local_mp_ring,
        receive_shared_sp_ring,
        receive_shared_mp_ring,
    };
    const auto obj = reinterpret_cast<object*>(r);
    AGT_assert_is_type(obj, receiver);
    return (jmp_table[AGT_get_type_index(obj, receiver)])(r, message, timeout);
  }


  inline static void retire(receiver_t r, agt_message_t message) noexcept {
    using retire_pfn = void(*)(receiver_t, agt_message_t);
    constexpr static retire_pfn jmp_table[] = {
        retire_local_spsc,
        retire_local_mpsc,
        retire_local_spmc,
        retire_local_mpmc,
        retire_shared_spsc,
        retire_shared_mpsc,
        retire_shared_spmc,
        retire_shared_mpmc,
        retire_private_queue,
        retire_local_sp_broadcast,
        retire_local_mp_broadcast,
        retire_shared_sp_broadcast,
        retire_shared_mp_broadcast,
        retire_local_sp_ring,
        retire_local_mp_ring,
        retire_shared_sp_ring,
        retire_shared_mp_ring,
    };
    const auto obj = reinterpret_cast<object*>(r);
    AGT_assert_is_type(obj, receiver);
    return (jmp_table[AGT_get_type_index(obj, receiver)])(r, message);
  }
}

#endif//AGATE_CORE_MSG_RECEIVER_HPP
