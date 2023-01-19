//
// Created by maxwe on 2022-07-13.
//

#ifndef AGATE_CHANNELS_H
#define AGATE_CHANNELS_H

#include <agate/core.h>

AGT_begin_c_namespace



// typedef struct agt_bqueue_st*     agt_bqueue_t;
// typedef struct agt_ring_queue_st* agt_ring_queue_t;
// typedef struct agt_msg_st*        agt_msg_t;
// typedef struct agt_queue_st*      agt_queue_t;

typedef struct agt_sender_st*     agt_sender_t;
typedef struct agt_receiver_st*   agt_receiver_t;




typedef struct agt_broadcast_queue_desc_t {
  size_t           fixedMessageSize;  ///< If 0, message sizes are dynamically sized at the cost of some overhead.
  agt_u32_t        maxReceivers;      ///< Maximum number of listeners/receivers.
  agt_scope_t      scope;             ///< Scope of
  agt_async_t*     async;             ///< Async handle for allowing deferred creation
  agt_name_token_t nameToken;         ///< Name token
} agt_broadcast_queue_desc_t;

typedef struct agt_ring_queue_desc_t {
  size_t           fixedBufferSize;   ///< Size of the fixed buffer;
  agt_u32_t        maxSenders;        ///< Maximum number of senders (if equal to 1, some optimizations can be made)
  agt_scope_t      scope;             ///<
  agt_async_t*     async;             ///<
  agt_name_token_t nameToken;         ///<
} agt_ring_queue_desc_t;



AGT_api agt_status_t agt_create_broadcast_queue(agt_ctx_t ctx, agt_sender_t* pSender) AGT_noexcept;

// Only a receiver is returned, given that only
AGT_api agt_status_t agt_create_ring_queue(agt_ctx_t ctx, agt_receiver_t* pReceiver, const agt_ring_queue_desc_t* pRingQueue) AGT_noexcept;



AGT_api agt_status_t agt_open_senders(agt_receiver_t toReceiver, agt_sender_t* pSender, size_t senderCount) AGT_noexcept;

AGT_api agt_status_t agt_clone_sender(agt_sender_t srcSender, agt_sender_t* pDstSender, size_t senderCount) AGT_noexcept;

AGT_api void         agt_close_sender(agt_sender_t sender) AGT_noexcept;



AGT_api agt_status_t agt_open_receivers(agt_sender_t toSender, agt_receiver_t* pReceiver, size_t receiverCount) AGT_noexcept;

AGT_api agt_status_t agt_clone_receiver(agt_receiver_t srcReceiver, agt_receiver_t* pDstReceiver, size_t receiverCount) AGT_noexcept;

AGT_api void         agt_close_receiver(agt_receiver_t receiver) AGT_noexcept;




AGT_api agt_status_t agt_acquire_msg(agt_sender_t sender, size_t desiredMessageSize, void** ppMsgBuffer, agt_timeout_t timeout) AGT_noexcept;

AGT_api void         agt_send_msg(agt_sender_t sender, void* msgBuffer, agt_async_t* async) AGT_noexcept;

AGT_api agt_status_t agt_receive_msg(agt_receiver_t receiver, void** pMsgBuffer, size_t* pMsgSize, agt_timeout_t timeout) AGT_noexcept;






AGT_end_c_namespace

#endif//AGATE_CHANNELS_H
