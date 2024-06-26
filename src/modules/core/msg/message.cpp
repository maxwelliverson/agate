//
// Created by maxwe on 2021-11-28.
//

#include "message.hpp"

#include "core/agents.hpp"
#include "core/async.hpp"

#include "agate/atomic.hpp"
#include "agate/flags.hpp"
#include "modules/core/instance.hpp"

#include <tuple>
#include <utility>




void* agt::try_message_get_async_local(agt_message_t message, bool& refCountIsZero) noexcept {
  if (test(message->flags, message_flags::asyncIsBound)) {
    return try_acquire_local_async(message->asyncData, message->asyncKey, refCountIsZero);
  }

  // In the case that there's no bound async, we use the extraData field as a reference count instead.
  refCountIsZero = atomic_decrement(message->extraData) == 0;

  return nullptr;
}

void* agt::try_message_get_async_local(agt_message_t message) noexcept {
  if (test(message->flags, message_flags::asyncIsBound))
    return try_acquire_local_async(message->asyncData, message->asyncKey);
  return nullptr;
}

void  agt::message_release_async_local(agt_ctx_t ctx, agt_message_t message, bool shouldWakeWaiters) noexcept {
  AGT_invariant( test(message->flags, message_flags::asyncIsBound) );
  if (shouldWakeWaiters) {
    wake_all_local_async(ctx, message->asyncData);
  }
  release_local_async(message->asyncData, false);
  reset(message->flags, message_flags::asyncIsBound);
}

void  agt::free_message(agt_message_t message) noexcept {

  AGT_invariant( !test(message->flags, message_flags::asyncIsBound) );

  // TODO: Implement message freeing
}


void agt::complete_agent_message(agt_ctx_t ctx, agt_message_t message, agt_status_t status, agt_u64_t value) noexcept {
  AGT_invariant( message->layout == AGT_MSG_LAYOUT_AGENT_CMD );
  AGT_invariant( !test(message->flags, message_flags::isComplete) );
  // TODO: Implement indirect message stuff, but for now, just assert that it's direct.

  if (auto asyncData = try_message_get_async_local(message)) {
    const auto agentData = static_cast<local_agent_message_async_data*>(asyncData);
    agentData->responseValue = value;
    agentData->status = status;
    atomic_store(agentData->isComplete, AGT_TRUE);
    message_release_async_local(ctx, message, true);
  }

  set_flags(message->flags, message_flags::isComplete);
  reset(message->flags, message_flags::isPinned);
}

void agt::finalize_agent_message(agt_ctx_t ctx, agt_message_t message, agt_u64_t value) noexcept {

  AGT_invariant( message->layout == AGT_MSG_LAYOUT_AGENT_CMD );

  if ( !test(message->flags, message_flags::isPinned)) [[likely]] {
    if ( !test(message->flags, message_flags::isComplete) )
      complete_agent_message(ctx, message, AGT_SUCCESS, value);
    // TODO: free message here
    free_message(message);
  }
}


/*
namespace {
  bool messageIsShared(agt_message_t message) noexcept {
    return test(message->flags & agt::message_flags::isShared);
  }
}

extern "C" {


struct AGT_cache_aligned agt_shared_message_st {
  size_t             nextIndex;
  agt_object_id_t         returnHandleId;
  size_t             asyncDataOffset;
  agt_u32_t           asyncDataKey;
  size_t             payloadSize;
  agt_message_id_t        id;
AGT_cache_aligned
  char                inlineBuffer[];
};
struct AGT_cache_aligned agt_local_message_st {
  agt_local_message_st* next;
  agt_handle_t           returnHandle;
  agt_async_data_t        asyncData;
  agt_u32_t           asyncDataKey;
  size_t             payloadSize;
  agt_message_id_t        id;
AGT_cache_aligned
  char                inlineBuffer[];
};

}


bool  agt::initMessageArray(local_channel_header* owner) noexcept {
  const size_t slotCount = owner->slotCount;
  const size_t messageSize = owner->inlineBufferSize + sizeof(agt_message_st);
  auto messageSlots = (std::byte*)ctxLocalAlloc(owner->context, slotCount * messageSize, AGT_CACHE_LINE);
  if (messageSlots) [[likely]] {
    for ( size_t i = 0; i < slotCount; ++i ) {
      auto address = messageSlots + (messageSize * i);
      auto message = new(address) agt_message_st;
      message->next = (agt_message_t)(address + messageSize);
      message->owner = owner;
      message->refCount = 1;
    }

    owner->messageSlots = messageSlots;

    return true;
  }

  return false;
}

void* agt::initMessageArray(shared_handle_header* owner, shared_channel_header* channel) noexcept {
  size_t slotCount   = channel->slotCount;
  size_t messageSize = channel->inlineBufferSize + sizeof(agt_message_st);
  size_t arraySize = slotCount * messageSize;
  auto messageSlots = (std::byte*)ctxSharedAlloc(owner->context,
                                                 arraySize,
                                                 AGT_CACHE_LINE,
                                                 channel->messageSlotsId);

  if (messageSlots) [[likely]] {

    for ( size_t offset = 0; offset < arraySize;) {
      auto message = new(messageSlots + offset) agt_message_st;
      message->nextOffset = (offset += messageSize);
      message->owner = nullptr; // TODO: Fix, this obviously doesn't work for shared channels
      message->refCount = 1;
      message->flags = message_flags::isShared;
    }

  }

  return messageSlots;

}


agt_agent_t agt::getRecipientFromRaw(agt_raw_msg_t msg) noexcept {
  return reinterpret_cast<agt_message_t>(msg)->receiver;
}

std::pair<agt_agent_t, shared_handle> agt::getMessageSender(agt_message_t message) noexcept {
  if (messageIsShared(message)) [[unlikely]]
    return { nullptr, message->senderHandle };
  return { message->sender, {} };
}


void agt::prepareLocalMessage(agt_message_t message, agt_agent_t receiver, agt_agent_t sender, agt_async_t* async, agent_cmd_t msgCmd) noexcept {
  assert(!messageIsShared(message));
  message->receiver = receiver;
  message->sender   = sender;
  std::tie(message->asyncData, message->asyncDataKey) = async != nullptr ? agt::asyncAttachLocal(*async, 1, 1) : std::pair<async_data_t, async_key_t>{};
  message->messageType = msgCmd;
}

void agt::prepareSharedMessage(agt_message_t message, agt_ctx_t ctx, agt_agent_t receiver, agt_agent_t sender, agt_async_t* async, agent_cmd_t msgCmd) noexcept {
  assert(messageIsShared(message));
  message->receiver       = agt::agentGetReceiver(receiver);
  message->senderHandle   = agt::agentGetSharedHandle(sender);
  std::tie(message->asyncData, message->asyncDataKey) = async != nullptr ? agt::asyncAttachShared(*async, 1, 1) : std::pair<async_data_t, async_key_t>{};
  message->messageType = msgCmd;
  message->senderContextId = ctxId(ctx);
}

void agt::prepareLocalIndirectMessage(agt_message_t message, agt_agent_t sender, agt_async_t *async, agt_u32_t expectedCount, agt::agent_cmd_t msgCmd) noexcept {
  assert(!messageIsShared(message));
  message->refCount = expectedCount;
  message->sender   = sender;
  std::tie(message->asyncData, message->asyncDataKey) = async != nullptr ? agt::asyncAttachLocal(*async, expectedCount, 1) : std::pair<async_data_t, async_key_t>{};
  message->messageType = msgCmd;
}

void agt::prepareSharedIndirectMessage(agt_message_t message, agt_ctx_t ctx, agt_agent_t sender, agt_async_t *async, agt_u32_t expectedCount, agt::agent_cmd_t msgCmd) noexcept {
  assert(messageIsShared(message));
  message->refCount     = expectedCount;
  message->senderHandle = agt::agentGetSharedHandle(sender);
  std::tie(message->asyncData, message->asyncDataKey) = async != nullptr ? agt::asyncAttachShared(*async, expectedCount, 1) : std::pair<async_data_t, async_key_t>{};
  message->messageType = msgCmd;
  message->senderContextId = ctxId(ctx);
}


void agt::writeUserMessage(agt_message_t message, const agt_send_info_t& sendInfo) noexcept {
  assert(sendInfo.size <= UINT32_MAX);
  assert(sendInfo.flags == 0);
  message->payloadSize = static_cast<agt_u32_t>(sendInfo.size);
  message->id          = sendInfo.id;
  std::memcpy(message->inlineBuffer, sendInfo.buffer, sendInfo.size);
}

void agt::writeIndirectUserMessage(agt_message_t message, agt_message_t indirectMsg) noexcept {
  if (messageIsShared(message)) {
    assert(messageIsShared(indirectMsg));
    message->flags             = message->flags | message_flags::indirectMsgIsShared;
    message->indirectMsgHandle = indirectMsg->selfSharedHandle;
  }
  else {
    message->flags       = message->flags & ~message_flags::indirectMsgIsShared;
    message->indirectMsg = indirectMsg;
  }
}
*/

