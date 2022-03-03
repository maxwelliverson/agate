//
// Created by maxwe on 2021-11-28.
//

#include "message.hpp"
#include "context/context.hpp"
#include "channels/channel.hpp"
#include "support/atomic.hpp"
#include "support/flags.hpp"


using namespace Agt;

namespace {

}

extern "C" {



struct AGT_cache_aligned AgtQueuedMessage_st {
  union {
    jem_size_t       index;
    AgtQueuedMessage address;
  } next;
};

struct AGT_cache_aligned AgtSharedMessage_st {
  AgtSize             nextIndex;
  AgtObjectId         returnHandleId;
  AgtSize             asyncDataOffset;
  AgtUInt32           asyncDataKey;
  AgtSize             payloadSize;
  AgtMessageId        id;
AGT_cache_aligned
  char                inlineBuffer[];
};
struct AGT_cache_aligned AgtLocalMessage_st {
  AgtLocalMessage_st* next;
  AgtHandle           returnHandle;
  AgtAsyncData        asyncData;
  AgtUInt32           asyncDataKey;
  AgtSize             payloadSize;
  AgtMessageId        id;
AGT_cache_aligned
  char                inlineBuffer[];
};

}


bool  Agt::initMessageArray(LocalChannelHeader* owner) noexcept {
  const AgtSize slotCount = owner->slotCount;
  const AgtSize messageSize = owner->inlineBufferSize + sizeof(AgtMessage_st);
  auto messageSlots = (std::byte*)ctxLocalAlloc(owner->context, slotCount * messageSize, AGT_CACHE_LINE);
  if (messageSlots) [[likely]] {
    for ( AgtSize i = 0; i < slotCount; ++i ) {
      auto address = messageSlots + (messageSize * i);
      auto message = new(address) AgtMessage_st;
      message->next = (AgtMessage)(address + messageSize);
      message->owner = owner;
      message->refCount = 1;
    }

    owner->messageSlots = messageSlots;

    return true;
  }

  return false;
}

void* Agt::initMessageArray(SharedHandleHeader* owner, SharedChannelHeader* channel) noexcept {
  AgtSize slotCount   = channel->slotCount;
  AgtSize messageSize = channel->inlineBufferSize + sizeof(AgtMessage_st);
  AgtSize arraySize = slotCount * messageSize;
  auto messageSlots = (std::byte*)ctxSharedAlloc(owner->context,
                                                 arraySize,
                                                 AGT_CACHE_LINE,
                                                 channel->messageSlotsId);

  if (messageSlots) [[likely]] {

    for ( AgtSize offset = 0; offset < arraySize;) {
      auto message = new(messageSlots + offset) AgtMessage_st;
      message->nextOffset = (offset += messageSize);
      message->owner = nullptr; // TODO: Fix, this obviously doesn't work for shared channels
      message->refCount = 1;
      message->flags = MessageFlags::isShared;
    }

  }

  return messageSlots;

}