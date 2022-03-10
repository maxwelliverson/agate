//
// Created by maxwe on 2021-11-28.
//

#include "message.hpp"
#include "context/context.hpp"
#include "channels/channel.hpp"
#include "support/atomic.hpp"
#include "support/flags.hpp"


using namespace agt;

namespace {

}

extern "C" {



/*struct AGT_cache_aligned AgtQueuedMessage_st {
  union {
    size_t       index;
    agt_queued_message_t address;
  } next;
};*/

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