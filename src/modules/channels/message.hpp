//
// Created by maxwe on 2021-11-28.
//

#ifndef JEMSYS_AGATE2_MESSAGE_HPP
#define JEMSYS_AGATE2_MESSAGE_HPP

#include "agate/epoch_ptr.hpp"
#include "agate/flags.hpp"
#include "config.hpp"

#include <utility>

namespace agt {


  enum agent_cmd_t {
    AGT_CMD_NOOP,                    ///< As the name would imply, this is a noop.

    AGT_CMD_PING_STATUS,             ///< Sent every so often by other executors to ensure

    AGT_CMD_KILL,                    ///< Command sent on abnormal termination. Minimal cleanup is performed, typically indicates some unhandled error

    AGT_CMD_SEND,                    ///< Process message sent with agt_send

    AGT_CMD_SEND_AS,                 ///< Process message sent with agt_send_as

    AGT_CMD_SEND_MANY,               ///< Process message sent with agt_send_many

    AGT_CMD_SEND_MANY_AS,            ///< Process message sent with agt_send_many_as

    AGT_CMD_ATTACH_AGENT,            ///< Attach agent

    AGT_CMD_DETACH_AGENT,            ///< Detach agent from this executor

    AGT_CMD_REBIND_AGENT,            ///< Rebind agent to another executor

    AGT_CMD_PROC_MESSAGE_SHARED,     ///< Process shared message

    // AGT_CMD_PROC_INDIRECT_MESSAGE,   ///< Process indirect message

    AGT_CMD_NAKED_MESSAGE,           ///< Message type of messages sent using raw channel API

    AGT_CMD_CLOSE_QUEUE,             ///< Normal termination command. Any messages sent before this will be processed as normal, but no new messages will be sent. As soon as the queue is empty, the agent is destroyed.
    AGT_CMD_INVALIDATE_QUEUE,        ///< Current message queue is discarded without having been processed, but the queue is not closed, nor is the agent destroyed.

    AGT_CMD_BARRIER_ARRIVE,          ///< When this message is dequeued, the arrival count of the provided barrier is incremented. If the post-increment arrival count is equal to the expected arrival count, any agents waiting on the barrier are unblocked, and if the barrier was set with a continuation callback, it is called (in the context of the last agent to arrive).
    AGT_CMD_BARRIER_WAIT,            ///< If the arrival count of the provided barrier is less than the expected arrival count, the agent is blocked until they are equal. Otherwise, this is a noop.
    AGT_CMD_BARRIER_ARRIVE_AND_WAIT, ///< Equivalent to sending AGT_CMD_BARRIER_ARRIVE immediately followed by AGT_CMD_BARRIER_WAIT
    AGT_CMD_BARRIER_ARRIVE_AND_DROP, ///<
    AGT_CMD_ACQUIRE_SEMAPHORE,       ///<
    AGT_CMD_RELEASE_SEMAPHORE,       ///<

    AGT_CMD_QUERY_UUID,              ///<
    AGT_CMD_QUERY_NAME,              ///<
    AGT_CMD_QUERY_DESCRIPTION,       ///<
    AGT_CMD_QUERY_PRODUCER_COUNT,    ///<
    AGT_CMD_QUERY_METHOD,            ///<

    AGT_CMD_QUERY_PROPERTY,          ///<
    AGT_CMD_QUERY_SUPPORT,           ///<
    AGT_CMD_SET_PROPERTY,            ///<
    AGT_CMD_WRITE,                   ///<
    AGT_CMD_READ,                    ///<

    AGT_CMD_FLUSH,                   ///<
    AGT_CMD_START,                   ///< Sent as the initial message to an eager agent; invokes an agent's start routine
    AGT_CMD_INVOKE_METHOD_BY_ID,     ///<
    AGT_CMD_INVOKE_METHOD_BY_NAME,   ///<
    AGT_CMD_REGISTER_METHOD,         ///<
    AGT_CMD_UNREGISTER_METHOD,       ///<
    AGT_CMD_INVOKE_CALLBACK,         ///<
    AGT_CMD_INVOKE_COROUTINE,        ///<
    AGT_CMD_REGISTER_HOOK,           ///<
    AGT_CMD_UNREGISTER_HOOK          ///<
  };


  enum msg_layout_t {
    AGT_MSG_LAYOUT_BASIC,
    AGT_MSG_LAYOUT_INDIRECT,
    AGT_MSG_LAYOUT_AGENT_CMD,
    AGT_MSG_LAYOUT_AGENT_INDIRECT_CMD,
    AGT_MSG_LAYOUT_RPC_INVOCATION,
    AGT_MSG_LAYOUT_PACKET
  };


  struct message_header_common {
    agt_u64_t    padding;
    msg_layout_t layout;
  };



  struct AGT_cache_aligned inline_buffer {};

  /*struct StagedMessage {
    HandleHeader* receiver;
    void*         message;
    void*         reserved[2];
    HandleHeader* returnHandle;
    size_t       messageSize;
    agt_message_id_t  id;
    void*         payload;
  };*/

  AGT_BITFLAG_ENUM(message_flags, agt_u32_t) {
    // dispatchKindById    = AGT_DISPATCH_BY_ID,
    // dispatchKindByName  = AGT_DISPATCH_BY_NAME,

    isOutOfLine         = 0x04,
    isMultiFrame        = 0x08,

    ownedByChannel      = 0x40,
    isAgentInvocation   = 0x80,
    isShared            = 0x100,
    shouldDoFastCleanup = 0x200,
    indirectMsgIsShared = 0x400
  };

  AGT_BITFLAG_ENUM(message_state, agt_u32_t) {
    isQueued    = 0x1,
    isOnHold    = 0x2,
    isCondemned = 0x4
  };

  inline constexpr static message_state default_message_state = {};

  /**
   * @returns true if successful, false if there was an allocation error
   * */
  bool  initMessageArray(local_channel_header* owner) noexcept;

  /**
   * @returns the address of the message array if successful, or a null point on allocation error
   * */
  void* initMessageArray(shared_handle_header* owner, shared_channel_header* channel) noexcept;

  void initMessage(agt_message_t message) noexcept;

  void setMessageId(agt_message_t message, agt_message_id_t id) noexcept;

  void cleanupMessage(agt_message_t message) noexcept;


  std::pair<agt_agent_t, shared_handle> getMessageSender(agt_message_t message) noexcept;

  agt_agent_t getRecipientFromRaw(agt_raw_msg_t msg) noexcept;


  void returnMessage(agt_message_t message) noexcept;

  void prepareLocalMessage(agt_message_t message, agt_agent_t receiver, agt_agent_t sender, agt_async_t* async, agent_cmd_t msgCmd) noexcept;

  void prepareSharedMessage(agt_message_t message, agt_ctx_t ctx, agt_agent_t receiver, agt_agent_t sender, agt_async_t* async, agent_cmd_t msgCmd) noexcept;

  void prepareLocalIndirectMessage(agt_message_t message, agt_agent_t sender, agt_async_t* async, agt_u32_t expectedCount, agent_cmd_t msgCmd) noexcept;

  void prepareSharedIndirectMessage(agt_message_t message, agt_ctx_t ctx, agt_agent_t sender, agt_async_t* async, agt_u32_t expectedCount, agent_cmd_t msgCmd) noexcept;

  void writeUserMessage(agt_message_t message, const agt_send_info_t& sendInfo) noexcept;

  void writeIndirectUserMessage(agt_message_t message, agt_message_t indirectMsg) noexcept;
}

struct agt_message_st {
  union {
    agt::epoch_ptr<agt_message_st>        next;
    agt::tagged_value<size_t, agt_u32_t>  nextOffset;
    agt::atomic_epoch_ptr<agt_message_st> epochNext;
    agt::tagged_atomic<size_t, agt_u32_t> epochNextOffset;
    agt::shared_handle                    selfSharedHandle;
  };
  union {
    agt_agent_t               sender;
    agt::shared_handle        senderHandle;
  };
  union {
    agt_agent_t               receiver;
    agt::shared_handle        receiverHandle;
    struct {
      agt_u32_t               refCount;
    };
  };
  agt::async_data_t           asyncData;
  agt::async_key_t            asyncDataKey;
  agt::message_flags          flags;
  /*agt_u32_t                   poolChunkOffset;
  union {
    agt::context_id           senderContextId;
  };*/
  union {
    agt_u64_t          id;
    agt_message_t      indirectMsg;
    agt::shared_handle indirectMsgHandle;
  };
  agt::agent_cmd_t            messageType;
  agt_u32_t                   payloadSize;
  agt::inline_buffer          inlineBuffer[];
};

static_assert(sizeof(agt_message_st) == AGT_CACHE_LINE);


#endif//JEMSYS_AGATE2_MESSAGE_HPP
