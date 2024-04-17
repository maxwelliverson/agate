//
// Created by maxwe on 2022-07-13.
//

#ifndef AGATE_CHANNELS_H
#define AGATE_CHANNELS_H

#include <agate/core.h>



#define AGT_NO_LIMIT 0

#define AGT_DEFAULT_MSG_POOL ((agt_msg_pool_t)AGT_NULL_HANDLE)

AGT_begin_c_namespace



// typedef struct agt_bqueue_st*     agt_bqueue_t;
// typedef struct agt_ring_queue_st* agt_ring_queue_t;
// typedef struct agt_msg_st*        agt_msg_t;
// typedef struct agt_queue_st*      agt_queue_t;


typedef struct agt_msg_pool_st*   agt_msg_pool_t;
typedef struct agt_sender_st*     agt_sender_t;
typedef struct agt_receiver_st*   agt_receiver_t;
typedef struct agt_channel_st*    agt_channel_t;

/*typedef struct agt_channel_t {
  agt_sender_t   sender;
  agt_receiver_t receiver;
} agt_channel_t;*/


typedef agt_flags32_t agt_channel_flags_t;




typedef struct agt_broadcast_queue_desc_t {
  agt_async_t      async;             ///< Async handle for allowing deferred creation
  agt_name_t       name;              ///< Name token
  agt_scope_t      scope;             ///< Scope of
  size_t           fixedMessageSize;  ///< If 0, message sizes are dynamically sized at the cost of some overhead.
  agt_u32_t        maxReceivers;      ///< Maximum number of listeners/receivers.
} agt_broadcast_queue_desc_t;

typedef struct agt_ring_queue_desc_t {
  agt_async_t      async;             ///<
  agt_name_t       name;              ///<
  agt_scope_t      scope;             ///<
  size_t           fixedBufferSize;   ///< Size of the fixed buffer;
  agt_u32_t        maxSenders;        ///< Maximum number of senders (if equal to 1, some optimizations can be made)
} agt_ring_queue_desc_t;


typedef struct agt_channel_desc_t {
  agt_async_t         async;             ///<
  agt_name_t          name;              ///<
  agt_scope_t         scope;             ///<
  agt_channel_flags_t flags;             ///< For now, must be 0.
  agt_msg_pool_t      messagePool;       ///< The message pool from which this channel allocates messages. May be AGT_DEFAULT_MSG_POOL, in which case a default message pool is used.
  agt_u32_t           maxSenders;
  agt_u32_t           maxReceivers;
} agt_channel_desc_t;


AGT_core_api agt_status_t AGT_stdcall agt_open_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t* pChannelDesc) AGT_noexcept;


AGT_core_api agt_status_t AGT_stdcall agt_create_broadcast_queue(agt_ctx_t ctx, agt_sender_t* pSender, const agt_broadcast_queue_desc_t* pBQueue) AGT_noexcept;

// Only a receiver is returned, given that only
AGT_core_api agt_status_t AGT_stdcall agt_create_ring_queue(agt_ctx_t ctx, agt_receiver_t* pReceiver, const agt_ring_queue_desc_t* pRingQueue) AGT_noexcept;


AGT_core_api agt_status_t AGT_stdcall agt_open_senders(agt_ctx_t ctx, agt_receiver_t toReceiver, agt_sender_t* pSender, size_t senderCount) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_open_receivers(agt_ctx_t ctx, agt_sender_t toSender, agt_receiver_t* pReceiver, size_t receiverCount) AGT_noexcept;



/**
 *
 * Come up with a decent API for testing connectivity so that it doesn't have to be done for every send operation.
 */


/**
 * In reality, a "send" operation comprises three distinct steps,
 *   - acquire a message
 *   - write message contents
 *   - enqueue message
 *
 * Agate provides two distinct paths
 *
 *   - Call agt_acquire_msg, manually write to the returned message buffer in user code, and then call agt_send_msg.
 *   - Write to a temporary buffer in user code, and call agt_send_msg, which will then perform the acquire, copy, enqueue operations all in one go.
 *
 * These two approaches have different pros and cons, largely dependent on the type of queue that sender is bound to.
 *
 * By and large, the former approach is more efficient, and allows for errors to be detected prior to
 * (potentially expensive) message buffer writing.
 *
 * If a message is acquired with agt_acquire_msg successfully, the subsequent operation to agt_send_msg is guaranteed to
 * be successful; the return value does not need to be checked in this case.
 *
 * */





AGT_core_api agt_status_t AGT_stdcall agt_acquire_msg(agt_sender_t sender, size_t desiredMessageSize, void** ppMsgBuffer) AGT_noexcept;

/**
 *
 * */
AGT_core_api agt_status_t AGT_stdcall agt_send_msg(agt_sender_t sender, void* msgBuffer, size_t size, agt_async_t async) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_receive_msg(agt_receiver_t receiver, void** pMsgBuffer, size_t* pMsgSize, agt_timeout_t timeout) AGT_noexcept;

AGT_core_api void         AGT_stdcall agt_retire_msg(agt_receiver_t receiver, void* msgBuffer, agt_u64_t response) AGT_noexcept;




AGT_end_c_namespace

#endif//AGATE_CHANNELS_H
