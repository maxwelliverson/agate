//
// Created by maxwe on 2022-05-27.
//

#include "agents.hpp"
// #include "agents.h"

#include "context/error.hpp"

#include "async/async.hpp"

#include "channels/message_queue.hpp"


#include "state.hpp"

namespace {


  void releaseMessage(agt::agent_instance* receiver, agt_message_t message) noexcept;

  void releaseMessageIndirect(agt::agent_instance* receiver, agt_message_t message) noexcept;
}


bool         agt::processMessage(agt::agent_instance* receiver, agt_agent_t sender, agt_message_t message) noexcept {
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