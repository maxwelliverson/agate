//
// Created by maxwe on 2022-05-27.
//

#include "agents.hpp"
#include "executor.hpp"

#include "context/error.hpp"
#include "async/async.hpp"
#include "channels/message_queue.hpp"

#include "state.hpp"


#include "support/vector.hpp"


#include <span>



/*
namespace {
  void releaseMessage(agt::agent_instance* receiver, agt_message_t message) noexcept;

  void releaseMessageIndirect(agt::agent_instance* receiver, agt_message_t message) noexcept;
}


bool agt::processMessage(agt::agent_instance* receiver, agt_agent_t sender, agt_message_t message) noexcept {
  auto lastAgent = std::exchange(agt::tl_state.boundAgent, receiver);
  auto lastMessage = std::exchange(agt::tl_state.currentMessage, message);

  if (receiver->kind == agt::free_agent_kind)
    (*receiver->)(message->extraData, message->inlineBuffer, message->payloadSize);
  else
    (*receiver->standard.proc)(receiver->standard.state, message->extraData, message->inlineBuffer, message->payloadSize);

#if defined(NDEBUG)
  agt::tl_state.boundAgent = lastAgent;
  return std::exchange(agt::tl_state.currentMessage, lastMessage);
#else
  auto poppedAgent = std::exchange(agt::tl_state.boundAgent, lastAgent);
  assert(poppedAgent == receiver);
  if (auto poppedMsg = std::exchange(agt::tl_state.currentMessage, lastMessage)) {
    assert(poppedMsg == message);
    return true;
  }
  return false;
#endif
}

bool agt::dequeueAndProcessMessage(message_queue_t queue, agt_timeout_t timeout) noexcept {

  using enum agt::agent_cmd_t;

  auto message = agt::dequeueMessage(queue, timeout);

  if (!message)
    return false;

  auto receiver = message->receiver;
  auto sender   = message->sender;

  switch (message->messageType) {
    case AGT_CMD_SEND:
      if (processMessage(receiver, sender, sender, message))
        releaseMessage(receiver, message);
      break;
    case AGT_CMD_SEND_AS:
      if (processMessage(receiver, sender, reinterpret_cast<agt_agent_t>(message->extraData), message))
        releaseMessage(receiver, message);
      break;
    case AGT_CMD_SEND_MANY:
      if (processMessage(receiver, sender, sender, reinterpret_cast<agt_message_t>(message->extraData)))
        releaseMessageIndirect(receiver, message);
      break;
    case AGT_CMD_SEND_MANY_AS: {
      auto indirectMessage = reinterpret_cast<agt_message_t>(message->extraData);
      if (processMessage(receiver, sender, reinterpret_cast<agt_agent_t>(indirectMessage->extraData), indirectMessage))
        releaseMessageIndirect(receiver, message);
    }
    break;
    default:
      agt::raiseError(AGT_ERROR_CORRUPTED_MESSAGE, message);
  }

  return true;
}


agt_status_t agt::wait(blocked_queue& queue, agt_async_t* async, agt_timeout_t timeout) noexcept {
  agt_status_t status = asyncStatus(*async);
  if (timeout == AGT_DO_NOT_WAIT || status != AGT_NOT_READY)
    return status;

  queue.blockKind = timeout == AGT_WAIT ? block_kind::eSingleBlock : block_kind::eSingleBlockWithTimeout;

  // if (setjmp(queue.contextStorage))
}

agt_status_t agt::waitAny(blocked_queue& queue, agt_async_t* const * ppAsyncs, size_t asyncCount, agt_timeout_t timeout) noexcept {}

agt_status_t agt::waitMany(blocked_queue& queue, agt_async_t* const * ppAsyncs, size_t asyncCount, size_t waitForCount, size_t& index, agt_timeout_t timeout) noexcept {}
*/

typedef void(* agt_send_callback_t)(void* userData, agt_agent_t receiver, agt_agent_t sender, const agt_send_info_t* sendInfo);
typedef void(* agt_receive_callback_t)(void* userData, agt_agent_t receiver, agt_agent_t sender, agt_message_id_t id, const void* message, agt_size_t msgSize);

struct send_hook {
  agt_send_callback_t callback;
  void*               userData;
};
struct receive_hook {
  agt_receive_callback_t callback;
  void*                  userData;
};

extern "C" {
typedef struct agt_local_agent_st*        agt_local_agent_t;
typedef struct agt_shared_agent_st*       agt_shared_agent_t;
typedef struct agt_imported_agent_st*     agt_imported_agent_t;
typedef struct agt_proxy_agent_st*        agt_proxy_agent_t;
typedef struct agt_shared_agent_state_st* agt_shared_agent_state_t;


struct agt_agent_st {
  agt::agent_flags     flags;
  agt_executor_t       executor;
  agt_type_id_t        type;
  agt_agent_t          receiver;
  /*agt_type_id_t        type;
  agt_agent_dtor_t     destructor;
  agt_agent_proc_t     proc;
  void*                state;
  agt::blocked_queue   blockedQueue;*/
};

struct agt_local_agent_st : agt_agent_st {
  agt_agent_dtor_t     destructor;
  agt_agent_proc_t     proc;
  void*                state;
};

struct agt_proxy_agent_st : agt_agent_st {
  agt_agent_t                  realAgent;
  agt::vector<send_hook, 1>    preSendHooks;
  agt::vector<receive_hook, 1> preReceiveHooks;
};

struct agt_shared_agent_st : agt_agent_st {
  agt_agent_dtor_t         destructor;
  agt_agent_proc_t         proc;
  void*                    state;
  agt_shared_agent_state_t sharedState;
  agt::shared_handle       sharedHandle;
};

struct agt_imported_agent_st : agt_agent_st {
  agt_shared_agent_state_t sharedState;
  agt::shared_handle       sharedHandle;
};





struct agt_shared_agent_state_st {
  agt::agent_flags   flags;
  agt_type_id_t      type;
  agt_shared_agent_t sharedAgent;    // These must only be accessed if current context id == ownerCtx
  agt_executor_t     sharedExecutor; // "
  agt::shared_handle executorHandle;
  agt::context_id    ownerCtx;
};







}


namespace {
  bool messageSentAcrossContexts(agt_ctx_t ctx, agt_message_t message) noexcept {
    if (test(message->flags & agt::message_flags::isShared)) [[unlikely]]
      return agt::ctxId(ctx) != message->senderContextId;
    return false;
  }

  enum agent_type {
    LOCAL_AGENT  = 0x0,
    PROXY_AGENT  = 0x1,
    SHARED_AGENT = 0x2,
    IMPORT_AGENT = 0x3
  };

  __forceinline agent_type getAgentType(agt_agent_t agent) noexcept {
    return static_cast<agent_type>(static_cast<std::underlying_type_t<agt::agent_flags>>(agent->flags & (agt::agent_flags::isShared | agt::agent_flags::isProxy)));
  }

  __forceinline bool isProxyAgent(agt_agent_t agent) noexcept {
    return getAgentType(agent) == PROXY_AGENT;
  }

  __forceinline bool isLocalAgent(agt_agent_t agent) noexcept {
    switch (getAgentType(agent)) {
      case LOCAL_AGENT: return true;
      case PROXY_AGENT: return isLocalAgent(static_cast<agt_proxy_agent_t>(agent)->realAgent);
      default: return false;
    }
  }

  __forceinline bool isSharedAgent(agt_agent_t agent) noexcept {
    switch (getAgentType(agent)) {
      case SHARED_AGENT: return true;
      case PROXY_AGENT: return isSharedAgent(static_cast<agt_proxy_agent_t>(agent)->realAgent);
      default: return false;
    }
  }

  __forceinline bool isImportedAgent(agt_agent_t agent) noexcept {
    switch (getAgentType(agent)) {
      case IMPORT_AGENT: return true;
      case PROXY_AGENT: return isImportedAgent(static_cast<agt_proxy_agent_t>(agent)->realAgent);
      default: return false;
    }
  }


  agt_status_t doReplyAs(agt_agent_t spoofSender, const agt_send_info_t& sendInfo) noexcept {
    auto ctx     = agt::tl_state.context;
    auto message = agt::tl_state.currentMessage;

    auto [sender, senderHandle]  = agt::getMessageSender(message);

    agt_executor_t executor;

    if (!sender) {
      if (senderHandle == agt::shared_handle{})
        return AGT_ERROR_FOREIGN_SENDER;
      auto sharedState = (agt_shared_agent_state_t)agt::ctxImportSharedHandle(ctx, senderHandle);
      sender   = sharedState->sharedAgent;
      if (sharedState->ownerCtx != agt::ctxId(ctx))
        executor = agt::importExecutor(ctx, sharedState->executorHandle);
      else
        executor = sharedState->sharedExecutor;
    }

    return agt::sendToExecutor(executor, sender, spoofSender, sendInfo);
  }

  agt_status_t doSendManyAs(agt_agent_t spoofSender, std::span<const agt_agent_t> recipients, const agt_send_info_t& sendInfo) noexcept {
    if (recipients.empty()) [[unlikely]]
      return AGT_SUCCESS;
    bool hasSharedAgent = false;
    for (auto recipient : recipients) {
      if (getAgentType(recipient) == IMPORT_AGENT) {
        hasSharedAgent = true;
        break;
      }
    }

    auto self = agt::tl_state.boundAgent;
    assert(self != nullptr);
    agt_status_t status;
    agt_message_t message;
    auto msgExec = agt::tl_state.executor;
    std::tie(status, message) = agt::getMessageForSendManyFromExecutor(msgExec, spoofSender, sendInfo, hasSharedAgent);
    if (status != AGT_SUCCESS) [[unlikely]] {
      if (message)
        agt::releaseMessageToExecutor(msgExec, message);
      return status;
    }

    for (auto recipient : recipients)
      ((agt_u32_t&)status) |= agt::sendOneOfManyToExecutor(recipient->executor, recipient, spoofSender, message);

    if (status != AGT_SUCCESS)
      return AGT_ERROR_COULD_NOT_REACH_ALL_TARGETS;
    return AGT_SUCCESS;
  }

  agt_status_t doSendRawAs(agt_agent_t spoofSender, agt_agent_t recipient, const agt_raw_send_info_t& sendInfo) noexcept {
#if !defined(NDEBUG)
    auto msgRecipient = agt::getRecipientFromRaw(sendInfo.msg);
    assert( (!msgRecipient || recipient == msgRecipient) && "recipient parameter of agt_raw_send does not match the recipient parameter of the matching call to agt_raw_acquire");
#endif
    return agt::sendRawToExecutor(recipient->executor, recipient, spoofSender, sendInfo);
  }

  agt_status_t doRawSendManyAs(agt_agent_t spoofSender, std::span<const agt_agent_t> recipients, const agt_raw_send_info_t& sendInfo) noexcept {

  }
}


agt_agent_t        agt::agentGetReceiver(agt_agent_t agent) noexcept {
  return agent->receiver;
}

agt::shared_handle agt::agentGetSharedHandle(agt_agent_t agent) noexcept {
  switch (getAgentType(agent)) {
    case LOCAL_AGENT:  return {};
    case PROXY_AGENT:  return agentGetSharedHandle(static_cast<agt_proxy_agent_t>(agent)->realAgent);
    case SHARED_AGENT: return static_cast<agt_shared_agent_t>(agent)->sharedHandle;
    case IMPORT_AGENT: return static_cast<agt_imported_agent_t>(agent)->sharedHandle;
    default:
      assert(false && "Invalid agent type!");
  }
}



extern "C" {


AGT_agent_api agt_ctx_t    AGT_stdcall agt_current_context() AGT_noexcept {
  AGT_assert_valid_aec("agt_current_context called outside of a valid agent execution context.");
  return agt::tl_state.context;
}

AGT_agent_api agt_agent_t  AGT_stdcall agt_self() AGT_noexcept {
  AGT_assert_valid_aec("agt_self called outside of a valid agent execution context.");
  return agt::tl_state.boundAgent;
}

AGT_agent_api agt_agent_t  AGT_stdcall agt_retain_sender() AGT_noexcept {
  AGT_assert_valid_aec("agt_retain_sender called outside of a valid agent execution context.");
  // TODO: Implement agt_retain_sender
}

AGT_agent_api agt_status_t AGT_stdcall agt_retain(agt_agent_t* pNewAgent, agt_agent_t agent) AGT_noexcept {
  AGT_assert_valid_aec("agt_retain called outside of a valid agent execution context.");
  // TODO: Implement agt_retain
}



AGT_agent_api agt_status_t AGT_stdcall agt_send(agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept {
  AGT_assert_valid_aec("agt_send called outside of a valid agent execution context.");
  if (!recipient || !pSendInfo) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  return agt::sendToExecutor(recipient->executor, recipient, agt::tl_state.boundAgent, *pSendInfo);
}

AGT_agent_api agt_status_t AGT_stdcall agt_send_as(agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept {
  AGT_assert_valid_aec("agt_send_as called outside of a valid agent execution context.");
  if (!recipient || !pSendInfo) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  return agt::sendToExecutor(recipient->executor, recipient, spoofSender, *pSendInfo);
}

AGT_agent_api agt_status_t AGT_stdcall agt_send_many(const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept {
  AGT_assert_valid_aec("agt_send_many called outside of a valid agent execution context.");
  assert(recipients || !agentCount);
  if (!pSendInfo) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  return doSendManyAs(agt::tl_state.boundAgent, { recipients, agentCount }, *pSendInfo);
}

AGT_agent_api agt_status_t AGT_stdcall agt_send_many_as(agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept {
  AGT_assert_valid_aec("agt_send_many_as called outside of a valid agent execution context.");
  assert(recipients || !agentCount);
  if (!pSendInfo)
    return AGT_ERROR_INVALID_ARGUMENT;
  return doSendManyAs(spoofSender, {recipients, agentCount}, *pSendInfo);
}

AGT_agent_api agt_status_t AGT_stdcall agt_reply(const agt_send_info_t* pSendInfo) AGT_noexcept {
  AGT_assert_valid_aec("agt_reply called outside of a valid agent execution context.");
  if (!pSendInfo) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;

  return doReplyAs(agt::tl_state.boundAgent, *pSendInfo);
}

AGT_agent_api agt_status_t AGT_stdcall agt_reply_as(agt_agent_t spoofReplier, const agt_send_info_t* pSendInfo) AGT_noexcept {
  AGT_assert_valid_aec("agt_reply_as called outside of a valid agent execution context.");
  if (!pSendInfo) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;

  return doReplyAs(spoofReplier, *pSendInfo);
}




AGT_agent_api agt_status_t AGT_stdcall agt_raw_acquire(agt_agent_t recipient, size_t desiredMessageSize, agt_raw_msg_t* pRawMsg, void** ppRawBuffer) AGT_noexcept {
  AGT_assert_valid_aec("agt_raw_acquire called outside of a valid agent execution context.");
  if (!recipient || !desiredMessageSize || !pRawMsg || !ppRawBuffer) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  agt::acquire_raw_info rawInfo{};
  agt_status_t status = agt::acquireRawFromExecutor(recipient->executor, recipient, desiredMessageSize, rawInfo);
  *pRawMsg     = rawInfo.rawMsg;
  *ppRawBuffer = rawInfo.buffer;
  return status;
}

AGT_agent_api agt_status_t AGT_stdcall agt_raw_send(agt_agent_t recipient, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept {
  AGT_assert_valid_aec("agt_raw_send called outside of a valid agent execution context.");
  if (!recipient || !pRawSendInfo) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  return doSendRawAs(agt::tl_state.boundAgent, recipient, *pRawSendInfo);
}

AGT_agent_api agt_status_t AGT_stdcall agt_raw_send_as(agt_agent_t spoofSender, agt_agent_t recipient, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept {
  AGT_assert_valid_aec("agt_raw_send_as called outside of a valid agent execution context.");
  if (!recipient || !pRawSendInfo) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  return doSendRawAs(spoofSender, recipient, *pRawSendInfo);
}

AGT_agent_api agt_status_t AGT_stdcall agt_raw_send_many(const agt_agent_t* recipients, agt_size_t agentCount, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept {
  // TODO: Implement agt_raw_send_many
}

AGT_agent_api agt_status_t AGT_stdcall agt_raw_send_many_as(agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept {
  // TODO: Implement agt_raw_send_many_as
}

AGT_agent_api agt_status_t AGT_stdcall agt_raw_reply(const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept {
  // TODO: Implement agt_raw_reply
}

AGT_agent_api agt_status_t AGT_stdcall agt_raw_reply_as(agt_agent_t spoofSender, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept {
  // TODO: Implement agt_raw_reply_as
}


AGT_agent_api void         AGT_stdcall agt_delegate(agt_agent_t recipient) AGT_noexcept {
  assert( recipient != nullptr );
  agt::delegateToExecutor(recipient->executor, recipient, agt::tl_state.currentMessage);
}






}