//
// Created by maxwe on 2022-05-27.
//

#include "agents.hpp"
// #include "agents.h"

#include "async/async.hpp"

#include "channels/message_queue.hpp"


#include "state.hpp"

namespace {


  void releaseMessage(agt::agent_instance* receiver, agt_message_t message) noexcept;

  void processMessage(agt::agent_instance* receiver, agt_message_t message) noexcept {
    auto lastAgent = std::exchange(agt::tl_state.boundAgent, receiver);
    auto lastMessage = std::exchange(agt::tl_state.currentMessage, message);

    if (receiver->kind == agt::free_agent_kind)
      (*receiver->free.proc)(message->extraData, message->inlineBuffer, message->payloadSize);
    else
      (*receiver->standard.proc)(receiver->standard.state, message->extraData, message->inlineBuffer, message->payloadSize);

#if defined(NDEBUG)
    agt::tl_state.boundAgent = lastAgent;
#else
    auto poppedAgent = std::exchange(agt::tl_state.boundAgent, lastAgent);
    assert(poppedAgent == receiver);
#endif


    if (auto poppedMsg = std::exchange(agt::tl_state.currentMessage, lastMessage)) {
      assert(poppedMsg == message);
      releaseMessage(receiver, message);
    }

  }


  bool dequeueNextMessage(agt::message_queue_t queue) noexcept {
    auto message = agt::dequeueMessage(queue, AGT_DO_NOT_WAIT);
    if (!message)
      return false;
    auto receiver = message->receiver;
    switch (message->messageType) {
      case AGT_CMD_PROC_MESSAGE:
        processMessage(receiver, message);
        break;
      case AGT_CMD_PROC_INDIRECT_MESSAGE:
        processMessage(receiver, reinterpret_cast<agt_message_t>(message->extraData));
        break;
      default:

    }

    return true;
  }
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