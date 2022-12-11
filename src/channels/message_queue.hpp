//
// Created by maxwe on 2022-05-27.
//

#ifndef AGATE_MESSAGE_QUEUE_HPP
#define AGATE_MESSAGE_QUEUE_HPP

#include "fwd.hpp"

#include "sender.hpp"
#include "receiver.hpp"
#include "messages/message.hpp"
#include "support/tagged_value.hpp"
#include "support/epoch_ptr.hpp"

namespace agt {

  struct basic_message;

  agt_status_t   enqueue(message_queue_t queue, basic_message* message) noexcept;
  basic_message* dequeue(message_queue_t queue, agt_timeout_t timeout) noexcept;


  // Declared in this order, this enum can be broken down as bitflags
  // bit 0: multi producer
  // bit 1: multi consumer
  // bit 2: private
  /*enum message_queue_kind {
    local_spsc_queue_kind,
    local_mpsc_queue_kind,
    local_spmc_queue_kind,
    local_mpmc_queue_kind,
    private_queue_kind,
    queue_kind_max_value
  };

  struct message_queue_header {
    message_queue_kind kind;
    agt_u32_t          flags;
  };*/
  
  
  // These simply act as tagged types for better code readability


  struct local_spsc_sender;
  struct local_spsc_receiver;


  /*struct local_spsc_queue : message_queue_header {
    basic_message*    head;
    basic_message**   tail;
  };*/

  struct private_sender;

  struct private_receiver : receiver {
    agt_message_st*          head;
    agt_message_st***        tail;
    unsafe_weak_ref<private_sender, bool> sender;
  };

  struct private_sender   : sender {
    agt_message_st** tail;
    unsafe_weak_ref<private_receiver> receiver;
  };





  struct local_spsc_sender      : sender {
    agt_message_st**     tail;
    local_spsc_receiver* receiver;
    agt_u32_t            receiverEpoch;
  };

  struct local_spsc_receiver    : receiver {
    agt_message_st*    head;
    local_spsc_sender* sender;
    agt_u32_t          senderEpoch;
  };



  struct local_mpsc_receiver : receiver {
    // local_mpsc_queue* queue;
    agt_message_st*  head;
    agt_message_st** tail;
    agt_u32_t        maxSenders;
    agt_u32_t        senderCount;
    // agt_u32_t        hasReceiver;
  };

  struct local_mpsc_sender   : sender {
    local_mpsc_receiver* receiver;
  };

  struct local_spmc_sender   : sender {
    agt_message_st*  head;
    agt_message_st** tail;
    agt_u32_t        maxReceivers;
    agt_u32_t        receiverCount;
    agt_u32_t        hasSender;
  };

  struct local_spmc_receiver : receiver {
    local_spmc_sender* sender;
  };


  struct local_mpmc_queue    : object {
    atomic_epoch_ptr<agt_message_st>                   head;
    atomic_epoch_ptr<atomic_epoch_ptr<agt_message_st>> tail;
    agt_u32_t maxSenders;
    agt_u32_t maxReceivers;
    agt_u32_t senderCount;
    agt_u32_t receiverCount;
  };

  struct local_mpmc_sender   : sender {
    local_mpmc_queue* queue;
  };

  struct local_mpmc_receiver : receiver {
    local_mpmc_queue* queue;
  };





  struct shared_spsc_sender : sender {};

  struct shared_spsc_receiver : receiver {};

  struct shared_mpsc_sender : sender {};

  struct shared_mpsc_receiver : receiver {};

  struct shared_spmc_sender : sender {};

  struct shared_spmc_receiver : receiver {};

  struct shared_mpmc_sender : sender {};

  struct shared_mpmc_receiver : receiver {};





  // These are the underlying function calls, which are differentiated by name rather than by
  // parameter types such that the generic enqueue/dequeue calls can dispatch with jump tables


  // Trampoline functions for better code legibility

  AGT_forceinline agt_status_t send(local_spsc_sender* sender,  agt_message_t message) noexcept {
    AGT_assert_is_a(sender, local_spsc_queue_sender);
    return sendLocalSPSCQueue(sender, message);
  }
  AGT_forceinline agt_status_t receive(local_spsc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, local_spsc_queue_receiver);
    return receiveLocalSPSCQueue(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(local_mpsc_sender* sender,  agt_message_t message) noexcept {
    AGT_assert_is_a(sender, local_mpsc_queue_sender);
    return sendLocalMPSCQueue(sender, message);
  }
  AGT_forceinline agt_status_t receive(local_mpsc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, local_mpsc_queue_receiver);
    return receiveLocalMPSCQueue(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(local_spmc_sender* sender,  agt_message_t message) noexcept {
    AGT_assert_is_a(sender, local_spmc_queue_sender);
    return sendLocalSPMCQueue(sender, message);
  }
  AGT_forceinline agt_status_t receive(local_spmc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, local_spmc_queue_receiver);
    return receiveLocalSPMCQueue(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(local_mpmc_sender* sender,  agt_message_t message) noexcept {
    AGT_assert_is_a(sender, local_mpmc_queue_sender);
    return sendLocalMPMCQueue(sender, message);
  }
  AGT_forceinline agt_status_t receive(local_mpmc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, local_mpmc_queue_receiver);
    return receiveLocalMPMCQueue(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(shared_spsc_sender* sender, agt_message_t message) noexcept {
    AGT_assert_is_a(sender, shared_spsc_queue_sender);
    return sendSharedSPSCQueue(sender, message);
  }
  AGT_forceinline agt_status_t receive(shared_spsc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, shared_spsc_queue_receiver);
    return receiveSharedSPSCQueue(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(shared_mpsc_sender* sender, agt_message_t message) noexcept {
    AGT_assert_is_a(sender, shared_mpsc_queue_sender);
    return sendSharedMPSCQueue(sender, message);
  }
  AGT_forceinline agt_status_t receive(shared_mpsc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, shared_mpsc_queue_receiver);
    return receiveSharedMPSCQueue(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(shared_spmc_sender* sender, agt_message_t message) noexcept {
    AGT_assert_is_a(sender, shared_spmc_queue_sender);
    return sendSharedSPMCQueue(sender, message);
  }
  AGT_forceinline agt_status_t receive(shared_spmc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, shared_spmc_queue_receiver);
    return receiveSharedSPMCQueue(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(shared_mpmc_sender* sender, agt_message_t message) noexcept {
    AGT_assert_is_a(sender, shared_mpmc_queue_sender);
    return sendSharedMPMCQueue(sender, message);
  }
  AGT_forceinline agt_status_t receive(shared_mpmc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, shared_mpmc_queue_receiver);
    return receiveSharedMPMCQueue(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(private_sender* sender,     agt_message_t message) noexcept {
    AGT_assert_is_a(sender, private_queue_sender);
    return sendPrivateQueue(sender, message);
  }
  AGT_forceinline agt_status_t receive(private_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, private_queue_receiver);
    return receivePrivateQueue(receiver, message, timeout);
  }
}

#endif//AGATE_MESSAGE_QUEUE_HPP
