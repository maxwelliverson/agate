//
// Created by maxwe on 2021-11-28.
//

#ifndef AGATE_CORE_MSG_MESSAGE_HPP
#define AGATE_CORE_MSG_MESSAGE_HPP

// #include "agate/epoch_ptr.hpp"

#include "config.hpp"
#include "agate/flags.hpp"
#include "agate/time.hpp"
// #include "core/async.hpp"
#include "core/ctx.hpp"


#include <utility>


namespace agt {
  /**
  * AGT_MSG_LAYOUT_BASIC {
  *   agt_message_t   next;
  *   msg_state_t     state;
  *   msg_layout_t    layout;
  *   message_flags   flags;
  *   agt_ecmd_t      cmd;
  *   async_data_t    asyncData;
  *   async_key_t     asyncKey;
  *   agt_u32_t       extraData;
  *   agt_timestamp_t sendTime;
  * }
  * Size: 40
  *
  * AGT_MSG_LAYOUT_SIGNAL {
  *   agt_message_t next;
  *   msg_state_t   state;
  *   msg_layout_t  layout;
  *   message_flags flags;
  *   agt_ecmd_t    cmd;
  * }
  * Size: 16
  *
  * AGT_MSG_LAYOUT_EXTENDED {
  *   agt_message_t   next;
  *   msg_state_t     state;
  *   msg_layout_t    layout;
  *   message_flags   flags;
  *   agt_ecmd_t      cmd;
  *   async_data_t    asyncData;
  *   async_key_t     asyncKey;
  *   agt_u32_t       extraData;
  *   agt_timestamp_t sendTime;
  *   agt_self_t      agent;
  *   agt_executor_t  targetExecutor;
  * }
  * Size: 56
  *
  * AGT_MSG_LAYOUT_CHANNEL {
  *   agt_message_t   next;
  *   msg_state_t     state;
  *   msg_layout_t    layout;
  *   message_flags   flags;
  *   agt_ecmd_t      cmd;
  *   async_data_t    asyncData;
  *   async_key_t     asyncKey;
  *   agt_u32_t       extraData; // channel message payload size.
  * }
  * Size: 32 + extraData
  *
  * AGT_MSG_LAYOUT_INDIRECT {
  *   agt_message_t   next;
  *   msg_state_t     state;
  *   msg_layout_t    layout;
  *   message_flags   flags;
  *   agt_ecmd_t      cmd;
  *   agt_message_t   indirectMsg;
  * }
  * Size: 24
  *
  * AGT_MSG_LAYOUT_AGENT_CMD {
  *   agt_message_t   next;
  *   msg_state_t     state;
  *   msg_layout_t    layout;
  *   message_flags   flags;
  *   agt_ecmd_t      cmd;
  *   async_data_t    asyncData;
  *   async_key_t     asyncKey;
  *   agt_u32_t       extraData; // Maybe priority??
  *   agt_timestamp_t sendTime;  // ??
  *   agt_self_t      agent;
  *   agt_self_t      sender;
  *   uint32_t        userDataOffset;
  *   uint32_t        payloadSize;
  * }
  * Size: 64 + userDataOffset + payloadSize
  *
  * AGT_MSG_LAYOUT_AGENT_INDIRECT_CMD {
  *   agt_message_t   next;
  *   msg_state_t     state;
  *   msg_layout_t    layout;
  *   message_flags   flags;
  *   agt_ecmd_t      cmd;
  *   agt_message_t   indirectMsg;
  *   agt_self_t      indirectAgent;
  * }
  * Size: 32
  *
  */
  enum msg_layout_t : agt_u8_t {
    AGT_MSG_LAYOUT_BASIC,              ///< Basic layout that contains standard info, but minimal slots for arbitrary data (Eg. PING)
    AGT_MSG_LAYOUT_SIGNAL,             ///< A barebones layout that contains no info or data other than the cmd code (Eg. NOOP, KILL, etc.)
    AGT_MSG_LAYOUT_EXTENDED,           ///< Like the basic layout, but with a couple additional arbitrary data slots. (Eg. ATTACH_AGENT, DETACH_AGENT)
    AGT_MSG_LAYOUT_INDIRECT,           ///< Used when broadcasting a message to multiple receivers; contains little more than a reference to a shared broadcast message
    AGT_MSG_LAYOUT_AGENT_CMD,          ///< Used by the agent invocation commands; allows for arbitrary amounts of extra data to be sent, depending on the specific command (Eg. SEND, SEND_AS)
    AGT_MSG_LAYOUT_AGENT_INDIRECT_CMD, ///< Almost the exact same as LAYOUT_INDIRECT, except has an extra data slot to hold the receiving agent
    AGT_MSG_LAYOUT_RPC_INVOCATION,     ///< Contains all the necessarily data to invoke an RPC. Not yet implemented
    AGT_MSG_LAYOUT_PACKET              ///< Contains an IP packet. Subject to change, not yet implemented.
  };




  enum msg_state_t : agt_u8_t {
    AGT_MSG_STATE_INITIAL,
    AGT_MSG_STATE_QUEUED,
    AGT_MSG_STATE_ACTIVE,
    AGT_MSG_STATE_PINNED,
    AGT_MSG_STATE_COMPLETE
  };

  AGT_DECL_BITFLAG_ENUM(message_flags, agt_u16_t);

}


extern "C" struct agt_message_st {
  agt_message_t          next;
  agt::msg_state_t       state;
  agt::msg_layout_t      layout;
  agt::message_flags     flags;
  agt_ecmd_t             cmd;
  union {
    agt_message_t        indirectMsg;
    async_data_t         asyncData;
  };
  union {
    struct {
      async_key_t        asyncKey;
      agt_u32_t          extraData;
    };
    agt_agent_instance_t indirectAgent;
  };
  agt_timestamp_t        sendTime;
  agt_agent_instance_t   agent;
  union {
    agt_agent_instance_t sender;
    agt_executor_t       targetExecutor;
  };
  uint32_t               userDataOffset;
  uint32_t               payloadSize;
};

namespace agt {

  // enum class agent_handle : agt_u64_t;


  enum cmd_t : agt_u16_t {

    AGT_CMD_NOOP,                    ///< As the name would imply, this is a noop.
    AGT_CMD_PING_STATUS,             ///< Sent every so often by other executors to test connection/latency
    AGT_CMD_KILL,                    ///< Command sent on abnormal termination. Minimal cleanup is performed, typically indicates some unhandled error

    AGT_CMD_SEND,                    ///< Process message sent with agt_send
    AGT_CMD_SEND_AS,                 ///< Process message sent with agt_send_as
    AGT_CMD_SEND_MANY,               ///< Process message sent with agt_send_many
    AGT_CMD_SEND_MANY_AS,            ///< Process message sent with agt_send_many_as

    AGT_CMD_GET_TIMESTAMP,           ///<

    AGT_CMD_MESSAGE_SEQUENCE,        ///< Indirectly contains multiple messages to be invoked one after the other

    AGT_CMD_ATTACH_AGENT,            ///< Attach agent; if agent is eager, also invokes the start routine
    AGT_CMD_DETACH_AGENT,            ///< Detach agent from this executor
    AGT_CMD_REBIND_AGENT,            ///< Rebind agent to another executor

    AGT_CMD_NAKED_MESSAGE,           ///< Message type of messages sent using raw channel API

    AGT_CMD_CTX_TRIM_MEMORY_POOLS,   ///<

    AGT_CMD_BARRIER_ARRIVE,          ///< When this message is dequeued, the arrival count of the provided barrier is incremented. If the post-increment arrival count is equal to the expected arrival count, any agents waiting on the barrier are unblocked, and if the barrier was set with a continuation callback, it is called (in the context of the last agent to arrive).
    AGT_CMD_BARRIER_WAIT,            ///< If the arrival count of the provided barrier is less than the expected arrival count, the agent is blocked until they are equal. Otherwise, this is a noop.
    AGT_CMD_BARRIER_ARRIVE_AND_WAIT, ///< Equivalent to sending AGT_CMD_BARRIER_ARRIVE immediately followed by AGT_CMD_BARRIER_WAIT
    AGT_CMD_BARRIER_ARRIVE_AND_DROP, ///<
    AGT_CMD_ACQUIRE_SEMAPHORE,       ///<
    AGT_CMD_RELEASE_SEMAPHORE,       ///<

    AGT_CMD_QUERY_AGENT_PROPERTY,    ///<
    AGT_CMD_SET_AGENT_PROPERTY,      ///<

    AGT_CMD_QUERY_AGENT_METHOD,      ///<
    AGT_CMD_INVOKE_AGENT_METHOD,     ///<
    AGT_CMD_REGISTER_AGENT_METHOD,   ///<
    AGT_CMD_UNREGISTER_AGENT_METHOD, ///<

    AGT_CMD_WRITE,                   ///<
    AGT_CMD_READ,                    ///<
    AGT_CMD_FLUSH,                   ///<

    AGT_CMD_INVOKE_COROUTINE,        ///<

    AGT_CMD_INVOKE_CALLBACK,         ///<

    AGT_CMD_REGISTER_HOOK,           ///<
    AGT_CMD_UNREGISTER_HOOK,         ///<

    AGT_CMD_CLOSE_QUEUE,             ///< Normal termination command. Any messages sent before this will be processed as normal, but no new messages will be sent. As soon as the queue is empty, the agent is destroyed.
    AGT_CMD_INVALIDATE_QUEUE,        ///< Current message queue is discarded without having been processed, but the queue is not closed, nor is the agent destroyed.
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

  AGT_DEFINE_BITFLAG_ENUM(message_flags, agt_u16_t) {
    // dispatchKindById    = AGT_DISPATCH_BY_ID,
    // dispatchKindByName  = AGT_DISPATCH_BY_NAME,

    isComplete          = 0x01,
    isPinned            = 0x02,

    isOutOfLine         = 0x04,
    isMultiFrame        = 0x08,

    ownedByChannel      = 0x40,
    isAgentInvocation   = 0x80,
    isShared            = 0x100,
    shouldDoFastCleanup = 0x200,
    indirectMsgIsShared = 0x400,
    asyncIsBound        = 0x800,
  };

  AGT_forceinline void* message_payload(agt_message_t msg) noexcept {
    return reinterpret_cast<std::byte*>(msg) + AGT_CACHE_LINE;
  }

  /*struct message {
    message*        next;
    uint32_t        bufferOffset;
    cmd_t           cmd;
    message_flags   flags;
    agent_self*     sender;
    agent_self*     receiver;
    agt_timestamp_t sendTime;
    agt_u64_t       extraData;
    async_data_t    asyncData;
    async_key_t     asyncKey;
    uint32_t        payloadSize;
    inline_buffer   buffer[];
  };*/


  inline static void init_message(agt_message_t message, msg_layout_t layout, agt_ecmd_t cmd, message_flags flags = {}) noexcept {
    message->layout = layout;
    message->state  = AGT_MSG_STATE_INITIAL;
    message->flags  = flags;
    message->cmd    = cmd;
  }

  inline static void message_bind_async(agt_message_t message, async_data_t asyncData, async_key_t key) noexcept {
    message->asyncData = asyncData;
    message->asyncKey  = key;
    message->flags    |= message_flags::asyncIsBound;
  }

  inline static void message_write_send_time(agt_message_t message) noexcept {
    message->sendTime = now();
  }


    /**
   * @note Semantics of this function mirror that of try_acquire_local_async,
   *       which this basically just wraps. This overload does a little extra work
   *       to determine whether or not the ref count dropped to zero, and is intended
   *       for indirect messages.
   *
   *
   * @param message
   * @param refCountIsZero
   * @return
   *
   */
  void* try_message_get_async_local(agt_message_t message, bool& refCountIsZero) noexcept;

  void* try_message_get_async_local(agt_message_t message) noexcept;

  // Should only be called if try_message_get_async_local was called and returned a nonnull value.
  void  message_release_async_local(agt_message_t message, bool shouldWakeWaiters) noexcept;

  // NOTE: This does NOT signal the async object, if bound.
  void free_message(agt_message_t message) noexcept;

  void complete_agent_message(agt_message_t message, agt_status_t status, agt_u64_t value) noexcept;

  void finalize_agent_message(agt_message_t message, agt_u64_t value) noexcept;

  AGT_forceinline static void pin_message(agt_message_t message) noexcept {
    set_flags(message->flags, message_flags::isPinned);
  }

  AGT_forceinline static void unpin_message(agt_message_t message) noexcept {
    reset(message->flags, message_flags::isPinned);
  }



  /*class message {
    message_header* pMsg = nullptr;

    template <typename T>
    static void destroy_message(T* msg) noexcept {}

  public:


    message() = default;
    message(message_header* header) noexcept : pMsg(header) { }
    message(agt_message_t message) noexcept : pMsg(reinterpret_cast<message_header *>(message)) { }
    message(std::nullptr_t) noexcept : pMsg(nullptr) { }

    void init(msg_layout_t layout, agt_ecmd_t cmd, message_flags flags = {}) const noexcept {
      pMsg->layout = layout;
      pMsg->state  = AGT_MSG_STATE_INITIAL;
      pMsg->flags  = flags;
      pMsg->cmd    = cmd;
    }

    void bind_async_local(async& async, agt_u32_t expectedCount = 1) noexcept {
      assert(pMsg != nullptr);
      auto msg = reinterpret_cast<basic_message*>(pMsg);
      auto [data, key] = async_attach_local(async, expectedCount, 1);
      msg->asyncData = data;
      msg->asyncKey  = key;
      msg->flags    |= message_flags::asyncIsBound;
    }

    void write_send_time() noexcept {
      reinterpret_cast<basic_message*>(pMsg)->sendTime = now();
    }

    template <bool SharedIsEnabled>
    void drop(agt_status_t dropReason, agt_ctx_t ctx = agt::get_ctx()) noexcept {
      if (test(pMsg->flags, message_flags::asyncIsBound)) {
        const auto msg = reinterpret_cast<basic_message*>(pMsg);
        async_drop<SharedIsEnabled>(ctx, msg->asyncData, msg->asyncKey, dropReason);
      }
      else if (pMsg->layout == AGT_MSG_LAYOUT_INDIRECT) {
        const auto msg = reinterpret_cast<indirect_message*>(pMsg);
        const auto broadcastMsg = msg->msg;
        bool shouldDestroy;
        if (test(broadcastMsg->flags, message_flags::asyncIsBound))
          shouldDestroy = async_drop<SharedIsEnabled>(ctx, broadcastMsg->asyncData, broadcastMsg->asyncKey, dropReason);
        else
          shouldDestroy = atomic_decrement(broadcastMsg->extraData) == 0;
        if (shouldDestroy)
          destroy_message(broadcastMsg);
      }
      pMsg->state = AGT_MSG_STATE_COMPLETE;
    }

    template <bool SharedIsEnabled>
    void complete(agt_ctx_t ctx = agt::get_ctx()) noexcept {
      if (test(pMsg->flags, message_flags::asyncIsBound)) {
        const auto msg = reinterpret_cast<basic_message*>(pMsg);
        async_arrive<SharedIsEnabled>(ctx, msg->asyncData, msg->asyncKey);
      }
      else if (pMsg->layout == AGT_MSG_LAYOUT_INDIRECT) {
        const auto msg = reinterpret_cast<indirect_message*>(pMsg);
        const auto broadcastMsg = msg->msg;
        bool shouldDestroy;
        if (test(broadcastMsg->flags, message_flags::asyncIsBound))
          shouldDestroy = async_arrive<SharedIsEnabled>(ctx, broadcastMsg->asyncData, broadcastMsg->asyncKey);
        else
          shouldDestroy = atomic_decrement(broadcastMsg->extraData) == 0;
        if (shouldDestroy)
          destroy_message(broadcastMsg);
      }
      pMsg->state = AGT_MSG_STATE_COMPLETE;
    }

    template <bool SharedIsEnabled>
    void complete_with_result(agt_u64_t result, agt_ctx_t ctx = agt::get_ctx()) noexcept {
      if (test(pMsg->flags, message_flags::asyncIsBound)) {
        const auto msg = reinterpret_cast<basic_message*>(pMsg);
        async_arrive_with_result<SharedIsEnabled>(ctx, msg->asyncData, msg->asyncKey, result);
      }
      else if (pMsg->layout == AGT_MSG_LAYOUT_INDIRECT) {
        const auto msg = reinterpret_cast<indirect_message*>(pMsg);
        const auto broadcastMsg = msg->msg;
        bool shouldDestroy;
        if (test(broadcastMsg->flags, message_flags::asyncIsBound))
          shouldDestroy = async_arrive_with_result<SharedIsEnabled>(ctx, broadcastMsg->asyncData, broadcastMsg->asyncKey, result);
        else
          shouldDestroy = atomic_decrement(broadcastMsg->extraData) == 0;
        if (shouldDestroy)
          destroy_message(broadcastMsg);
      }
      pMsg->state = AGT_MSG_STATE_COMPLETE;
    }

    void try_release() noexcept {

    }

    void release(agt_ctx_t ctx) noexcept {

    }

    [[nodiscard]] msg_layout_t layout() const noexcept {
      return pMsg->layout;
    }

    [[nodiscard]] agt_ecmd_t   cmd() const noexcept {
      return pMsg->cmd;
    }


    [[nodiscard]] message&     next() const noexcept {
      return *reinterpret_cast<message *>(&pMsg->next);
    }

    template <typename T>
    [[nodiscard]] T* get_as() const noexcept {
      return static_cast<T*>(pMsg);
    }

    explicit operator bool() const noexcept {
      return pMsg != nullptr;
    }

    [[nodiscard]] bool operator==(std::nullptr_t) const noexcept {
      return pMsg == nullptr;
    }

    [[nodiscard]] inline agt_message_t c_type() const noexcept {
      return reinterpret_cast<agt_message_t>(pMsg);
    }

    [[nodiscard]] inline agt_message_t& c_ref() noexcept {
      return *reinterpret_cast<agt_message_t*>(&pMsg);
    }
  };*/


  // static_assert(sizeof(message) == AGT_CACHE_LINE);



  /**
   * @returns true if successful, false if there was an allocation error
   * */
  /*bool  initMessageArray(local_channel_header* owner) noexcept;*/

  /**
   * @returns the address of the message array if successful, or a null point on allocation error
   * */
  /*void* initMessageArray(shared_handle_header* owner, shared_channel_header* channel) noexcept;

  void initMessage(agt_message_t message) noexcept;

  void setMessageId(agt_message_t message, agt_message_id_t id) noexcept;

  void cleanupMessage(agt_message_t message) noexcept;


  std::pair<agt_agent_t, shared_handle> getMessageSender(agt_message_t message) noexcept;

  agt_agent_t getRecipientFromRaw(agt_raw_msg_t msg) noexcept;


  void try_return_message(agt_self_t self, agt_message_t message) noexcept;


  void returnMessage(agt_message_t message) noexcept;

  void prepareLocalMessage(agt_message_t message, agt_agent_t receiver, agt_agent_t sender, agt_async_t* async, agent_cmd_t msgCmd) noexcept;

  void prepareSharedMessage(agt_message_t message, agt_ctx_t ctx, agt_agent_t receiver, agt_agent_t sender, agt_async_t* async, agent_cmd_t msgCmd) noexcept;

  void prepareLocalIndirectMessage(agt_message_t message, agt_agent_t sender, agt_async_t* async, agt_u32_t expectedCount, agent_cmd_t msgCmd) noexcept;

  void prepareSharedIndirectMessage(agt_message_t message, agt_ctx_t ctx, agt_agent_t sender, agt_async_t* async, agt_u32_t expectedCount, agent_cmd_t msgCmd) noexcept;

  void writeUserMessage(agt_message_t message, const agt_send_info_t& sendInfo) noexcept;

  void writeIndirectUserMessage(agt_message_t message, agt_message_t indirectMsg) noexcept;*/
}

/*struct agt_message_st {
  std::byte          reserved[12];
  agt::cmd_t         cmd;
  agt::message_flags flags;
  agt_agent_handle_t sender;
  agt_agent_handle_t receiver;
  agt_timestamp_t    sendTime;
  agt_u64_t          extraData;
  agt::async_data_t  asyncData;
  agt::async_key_t   asyncKey;
  uint32_t           payloadSize;
  agt::inline_buffer buffer[];
};*/

/*struct agt_message_st {
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
  union {
    agt_u64_t          id;
    agt_message_t      indirectMsg;
    agt::shared_handle indirectMsgHandle;
  };
  agt::agent_cmd_t            messageType;
  agt_u32_t                   payloadSize;
  agt::inline_buffer          inlineBuffer[];
};*/

// static_assert(sizeof(agt_message_st) == AGT_CACHE_LINE);



#endif//AGATE_CORE_MSG_MESSAGE_HPP
