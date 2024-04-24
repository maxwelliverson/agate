//
// Created by maxwe on 2022-05-27.
//

#ifndef AGATE_CORE_MSG_MESSAGE_QUEUE_HPP
#define AGATE_CORE_MSG_MESSAGE_QUEUE_HPP

#include "config.hpp"

#include "agate/epoch_ptr.hpp"
#include "agate/tagged_value.hpp"
#include "core/ref.hpp"
#include "message.hpp"
#include "receiver.hpp"
#include "sender.hpp"


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

  // All senders must have a message pool field at an offset of 16 bytes


  /*struct local_spsc_queue : message_queue_header {
    basic_message*    head;
    basic_message**   tail;
  };*/

  struct private_sender;

  AGT_object(private_receiver) {
    agt_u32_t       refCount;
    agt_message_t   head;
    agt_message_t** tail;
    private_sender* sender;
  };

  AGT_object(private_sender) {
    agt_u32_t         refCount;
    agt_message_t*    tail;
    private_receiver* receiver; // If receiver dies, it sets this variable to null.
    agt_u32_t         maxSenders;
    agt_u32_t         maxReceivers; // This is stored in the sender for data packing purposes.
    unsafe_signal_task_queue waiters;
  };

  struct spsc_channel_msg {
    spsc_channel_msg* next;
    size_t            size;
  };


  struct spsc_channel_waiter {
    void              (* resume)(agt_ctx_t ctx, agt_ctxexec_t prevCtxExec, agt_utask_t task);
    agt_ctxexec_t        ctxexec;
    agt_utask_t          task;
    spsc_channel_msg*    msg;
  };

  struct spsc_channel : rc_object {
    agt_msg_pool_t     msgPool;
    spsc_channel_msg*  head;
    spsc_channel_msg** tail;
    agt_ctx_t          senderCtx;
    agt_ctx_t          receiverCtx;
    uintptr_t          waiterQueue;
  };


  // Have a local blocked_queue specialized for spsc use here (note that spsc does not necessarily mean that there can only be one waiter, as it's multiplexed by executor).
  AGT_object(local_spsc_sender) {
    agt_u32_t            refCount; // Not using ref counted api cause this is a specialized case
    agt_message_t*       tail;
    agt_msg_pool_t       msgPool;
    agt_ctx_t            ctx;
    local_spsc_receiver* receiver;
  };

  AGT_object(local_spsc_receiver) {
    agt_message_t      head;
    agt_msg_pool_t     msgPool;
    agt_ctx_t          ctx;
    agt_message_t**    pTail;
    local_spsc_sender* sender;
  };



  AGT_object(local_mpsc_receiver, ref_counted) {
    agt_msg_pool_t  msgPool;
    agt_message_t   head;
    agt_message_t*  tail;
    agt_u32_t       senderCount;
    agt_bool_t      isAlive;
    agt_ctx_t       ctx;
  };

  AGT_object(local_mpsc_sender) {
    ref<local_mpsc_receiver> receiver;
    agt_msg_pool_t           msgPool;
  };



  AGT_object(local_spmc_sender, ref_counted) {
    agt_msg_pool_t  msgPool;
    agt_message_t   head;
    agt_message_t*  tail;
    agt_u32_t       receiverCount;
    agt_bool_t      isAlive;
  };

  AGT_object(local_spmc_receiver) {
    ref<local_spmc_sender> sender;
    agt_msg_pool_t         msgPool;
  };


  AGT_object(local_mpmc_queue, ref_counted) {
    agt_message_t head;
    agt_message_t* tail;
    agt_u32_t      senderCount;
    agt_u32_t      receiverCount;
    agt_msg_pool_t msgPool;
  };

  AGT_object(local_mpmc_sender) {
    ref<local_mpmc_queue> queue;
    agt_msg_pool_t        msgPool;
  };

  AGT_object(local_mpmc_receiver) {
    ref<local_mpmc_queue> queue;
    agt_msg_pool_t        msgPool;
  };



  AGT_object(shared_spsc_sender) {};

  AGT_object(shared_spsc_receiver) {};

  AGT_object(shared_mpsc_sender) {};

  AGT_object(shared_mpsc_receiver) {};

  AGT_object(shared_spmc_sender) {};

  AGT_object(shared_spmc_receiver) {};

  AGT_object(shared_mpmc_sender) {};

  AGT_object(shared_mpmc_receiver) {};


  agt_status_t     open_private_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept;
  agt_status_t  open_local_spsc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept;
  agt_status_t  open_local_mpsc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept;
  agt_status_t  open_local_spmc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept;
  agt_status_t  open_local_mpmc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept;
  agt_status_t open_shared_spsc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept;
  agt_status_t open_shared_mpsc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept;
  agt_status_t open_shared_spmc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept;
  agt_status_t open_shared_mpmc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept;


  // These are the underlying function calls, which are differentiated by name rather than by
  // parameter types such that the generic enqueue/dequeue calls can dispatch with jump tables


  // Trampoline functions for better code legibility

  AGT_forceinline agt_status_t send(local_spsc_sender* sender, agt_message_t message) noexcept {
    AGT_assert_is_a(sender, local_spsc_sender);
    return send_local_spsc(sender, message);
  }
  AGT_forceinline agt_status_t receive(local_spsc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, local_spsc_receiver);
    return receive_local_spsc(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(local_mpsc_sender* sender,  agt_message_t message) noexcept {
    AGT_assert_is_a(sender, local_mpsc_sender);
    return send_local_mpsc(sender, message);
  }
  AGT_forceinline agt_status_t receive(local_mpsc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, local_mpsc_receiver);
    return receive_local_mpsc(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(local_spmc_sender* sender,  agt_message_t message) noexcept {
    AGT_assert_is_a(sender, local_spmc_sender);
    return send_local_spmc(sender, message);
  }
  AGT_forceinline agt_status_t receive(local_spmc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, local_spmc_receiver);
    return receive_local_spmc(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(local_mpmc_sender* sender,  agt_message_t message) noexcept {
    AGT_assert_is_a(sender, local_mpmc_sender);
    return send_local_mpmc(sender, message);
  }
  AGT_forceinline agt_status_t receive(local_mpmc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, local_mpmc_receiver);
    return receive_local_mpmc(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(shared_spsc_sender* sender, agt_message_t message) noexcept {
    AGT_assert_is_a(sender, shared_spsc_sender);
    return send_shared_spsc(sender, message);
  }
  AGT_forceinline agt_status_t receive(shared_spsc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, shared_spsc_receiver);
    return receive_shared_spsc(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(shared_mpsc_sender* sender, agt_message_t message) noexcept {
    AGT_assert_is_a(sender, shared_mpsc_sender);
    return send_shared_mpsc(sender, message);
  }
  AGT_forceinline agt_status_t receive(shared_mpsc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, shared_mpsc_receiver);
    return receive_shared_mpsc(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(shared_spmc_sender* sender, agt_message_t message) noexcept {
    AGT_assert_is_a(sender, shared_spmc_sender);
    return send_shared_spmc(sender, message);
  }
  AGT_forceinline agt_status_t receive(shared_spmc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, shared_spmc_receiver);
    return receive_shared_spmc(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(shared_mpmc_sender* sender, agt_message_t message) noexcept {
    AGT_assert_is_a(sender, shared_mpmc_sender);
    return send_shared_mpmc(sender, message);
  }
  AGT_forceinline agt_status_t receive(shared_mpmc_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, shared_mpmc_receiver);
    return receive_shared_mpmc(receiver, message, timeout);
  }

  AGT_forceinline agt_status_t send(private_sender* sender,     agt_message_t message) noexcept {
    AGT_assert_is_a(sender, private_sender);
    return send_private_queue(sender, message);
  }
  AGT_forceinline agt_status_t receive(private_receiver* receiver,  agt_message_t& message, agt_timeout_t timeout) noexcept {
    AGT_assert_is_a(receiver, private_receiver);
    return receive_private_queue(receiver, message, timeout);
  }
}

#endif//AGATE_CORE_MSG_MESSAGE_QUEUE_HPP
