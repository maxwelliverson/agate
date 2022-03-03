//
// Created by maxwe on 2021-11-03.
//

// #include "internal.hpp"
#include "async/async.hpp"
#include "messages/message.hpp"
#include "support/align.hpp"
#include "channel.hpp"

#include <cstring>

using namespace Agt;


/*namespace Agt {

  struct AGT_cache_aligned PrivateChannelMessage {
    PrivateChannelMessage* next;
    PrivateChannel*        owner;
    AgtHandle              returnHandle;
    AgtAsyncData           asyncData;
    AgtUInt32              asyncDataKey;
    MessageFlags           flags;
    MessageState           state;
    AgtUInt32              refCount;
    AgtMessageId           id;
    AgtSize                payloadSize;
    InlineBuffer           inlineBuffer[];
  };

  struct AGT_cache_aligned LocalChannelMessage {
    LocalChannelMessage*      next;
    LocalChannel*             owner;
    AgtHandle                 returnHandle;
    AgtAsyncData              asyncData;
    AgtUInt32                 asyncDataKey;
    MessageFlags              flags;
    AtomicFlags<MessageState> state;
    ReferenceCount            refCount;
    AgtMessageId              id;
    AgtSize                   payloadSize;
    InlineBuffer              inlineBuffer[];
  };

  struct AGT_cache_aligned SharedChannelMessage {
    AgtSize                   nextIndex;
    AgtSize                   thisIndex;
    AgtObjectId               returnHandleId;
    AgtSize                   asyncDataOffset;
    AgtUInt32                 asyncDataKey;
    MessageFlags              flags;
    AtomicFlags<MessageState> state;
    ReferenceCount            refCount;
    AgtMessageId              id;
    AgtSize                   payloadSize;
    InlineBuffer              inlineBuffer[];
  };

}*/


#define AGT_acquire_semaphore(sem, timeout, err) do { \
  switch (timeout) {                                          \
    case AGT_WAIT:                                    \
      (sem).acquire();                                  \
      break;                                          \
    case AGT_DO_NOT_WAIT:                             \
      if (!(sem).try_acquire())                         \
        return err;                                   \
      break;                                          \
    default:                                          \
      if (!(sem).try_acquire_for(timeout))              \
        return err;\
  }                                                    \
  } while(false)

namespace {


  inline AtomicFlags<MessageState>& state(AgtMessage message) noexcept {
    return reinterpret_cast<AtomicFlags<MessageState>&>(message->state);
  }


  /*
  inline void* getPayload(void* message_) noexcept {
    auto message = static_cast<PrivateChannelMessage*>(message_);
    if ((message->flags & MessageFlags::isOutOfLine) == FlagsNotSet) [[likely]]
      return message->inlineBuffer;
    else {
      if ((message->flags & MessageFlags::isMultiFrame) == FlagsNotSet) {
        void* resultData;
        std::memcpy(&resultData, message->inlineBuffer, sizeof(void*));
        return resultData;
      }
      return nullptr;
    }
  }

  void zeroMessageData(void* message_) noexcept {
    auto message = static_cast<PrivateChannelMessage*>(message_);
    if (void* payload = getPayload(message)) {
      std::memset(payload, 0, message->payloadSize);
    } else {
      AgtMultiFrameMessageInfo mfi;
      auto status = getMultiFrameMessage(message->inlineBuffer, mfi);
      if (status == AGT_SUCCESS) {
        AgtMessageFrame frame;
        while(getNextFrame(frame, mfi))
          std::memset(frame.data, 0, frame.size);
      }
    }
  }

  AGT_noinline void doSlowCleanup(void* message_) noexcept {
    auto message = static_cast<PrivateChannelMessage*>(message_);
    zeroMessageData(message);
    message->returnHandle = nullptr;
    message->id           = 0;
    message->payloadSize  = 0;
  }

  inline void* getPayload(SharedChannelMessage* message) noexcept {
    // TODO: Implement Shared
    if ((message->flags & MessageFlags::isOutOfLine) == FlagsNotSet) [[likely]]
      return message->inlineBuffer;
    else {
      if ((message->flags & MessageFlags::isMultiFrame) == FlagsNotSet) {
        void* resultData;
        std::memcpy(&resultData, message->inlineBuffer, sizeof(void*));
        return resultData;
      }
      return nullptr;
    }
  }

  void zeroMessageData(SharedChannelMessage* message) noexcept {
    // TODO: Implement Shared
    if (void* payload = getPayload(message)) {
      std::memset(payload, 0, message->payloadSize);
    } else {
      AgtMultiFrameMessageInfo mfi;
      auto status = getMultiFrameMessage(message->inlineBuffer, mfi);
      if (status == AGT_SUCCESS) {
        AgtMessageFrame frame;
        while(getNextFrame(frame, mfi))
          std::memset(frame.data, 0, frame.size);
      }
    }
  }

  AGT_noinline void doSlowCleanup(SharedChannelMessage* message) noexcept {
    // TODO: Implement Shared
    zeroMessageData(message);
    // message->returnHandle = nullptr;
    message->id           = 0;
    message->payloadSize  = 0;
  }*/

  void zeroMessageData(AgtMessage message) noexcept {
    std::memset(message->inlineBuffer, 0, message->payloadSize);
  }

  AGT_noinline void doSlowCleanup(AgtMessage message) noexcept {
    zeroMessageData(message);
    message->returnHandle = nullptr;
    message->id           = 0;
    message->payloadSize  = 0;
  }


  inline void privateCleanupMessage(AgtContext ctx, AgtMessage message) noexcept {
    message->state    = DefaultMessageState;
    message->refCount = 1;
    message->flags    = FlagsNotSet;
    if (message->asyncData != nullptr)
      asyncDataDrop(message->asyncData, ctx, message->asyncDataKey);
    if ((message->flags & MessageFlags::shouldDoFastCleanup) == FlagsNotSet)
      doSlowCleanup(message);
    //TODO: Release message resources if message is not inline
  }
  inline void cleanupMessage(AgtContext ctx, AgtMessage message) noexcept {
    message->state    = DefaultMessageState;
    Impl::atomicStore(message->refCount, 1);
    message->flags    = FlagsNotSet;
    if (message->asyncData != nullptr)
      asyncDataDrop(message->asyncData, ctx, message->asyncDataKey);
    if ((message->flags & MessageFlags::shouldDoFastCleanup) == FlagsNotSet)
      doSlowCleanup(message);
    //TODO: Release message resources if message is not inline
  }

  inline void privateDestroyMessage(PrivateChannel* channel, AgtMessage message) noexcept {
    privateCleanupMessage(channel->context, message);
    ObjectInfo<PrivateChannel>::releaseMessage(channel, message);
  }
  template <typename Channel>
  inline void destroyMessage(Channel* channel, AgtMessage message) noexcept {
    cleanupMessage(channel->context, message);
    ObjectInfo<Channel>::releaseMessage(channel, message);
  }

  inline void privatePlaceHold(AgtMessage message) noexcept {
    message->state = message->state | MessageState::isOnHold;
  }
  inline void privateReleaseHold(PrivateChannel* channel, AgtMessage message) noexcept {
    MessageState oldState = message->state;
    message->state = message->state & ~MessageState::isOnHold;
    if ((oldState & MessageState::isCondemned) != FlagsNotSet)
      destroyMessage(channel, message);
  }

  inline void placeHold(AgtMessage message) noexcept {
    state(message).set(MessageState::isOnHold);
  }
  template <typename Channel>
  inline void releaseHold(Channel* channel, AgtMessage message) noexcept {
    if ( (state(message).fetchAndReset(MessageState::isOnHold) & MessageState::isCondemned) != FlagsNotSet )
      destroyMessage(channel, message);
  }

  inline void condemn(PrivateChannel* channel, AgtMessage message) noexcept {
    MessageState oldState = message->state;
    message->state = message->state | MessageState::isCondemned;
    if ((oldState & MessageState::isOnHold) == FlagsNotSet)
      destroyMessage(channel, message);
  }
  template <typename Channel>
  inline void condemn(Channel* channel, AgtMessage message) noexcept {
    if ( (state(message).fetchAndSet(MessageState::isCondemned) & MessageState::isOnHold) == FlagsNotSet )
      destroyMessage(channel, message);
  }


  inline void destroy(PrivateChannel* channel) noexcept {

  }


  inline AgtSize getDefaultPrivateChannelMessageSize(AgtContext ctx) noexcept {

    // Default to be used if default lookup fails
    // Each slot is 256 bytes, or 4 cachelines long.
    constexpr static AgtSize SuperDefault = AGT_CACHE_LINE * 3;


    AgtSize messageSize;
    AgtSize dataSize = sizeof(AgtSize);

    if (!ctxGetBuiltinValue(ctx, BuiltinValue::defaultPrivateChannelMessageSize, &messageSize, dataSize)) [[unlikely]]
      return SuperDefault; // If this path is taken, there's likely something very wrong

    return messageSize;
  }
  inline AgtSize getDefaultPrivateChannelSlotCount(AgtContext ctx) noexcept {
    // Default to be used if default lookup fails
    // Using both SuperDefaults, each channel would be 64 KiB in size
    constexpr static AgtSize SuperDefault = 256;


    AgtSize slotCount;
    AgtSize dataSize = sizeof(AgtSize);

    if (!ctxGetBuiltinValue(ctx, BuiltinValue::defaultPrivateChannelSlotCount, &slotCount, dataSize)) [[unlikely]]
      return SuperDefault; // If this path is taken, there's likely something very wrong

    return slotCount;
  }
  inline AgtSize getDefaultLocalChannelMessageSize(AgtContext ctx) noexcept {

    // Default to be used if default lookup fails
    // Each slot is 256 bytes, or 4 cachelines long.
    constexpr static AgtSize SuperDefault = AGT_CACHE_LINE * 3;


    AgtSize messageSize;
    AgtSize dataSize = sizeof(AgtSize);

    if (!ctxGetBuiltinValue(ctx, BuiltinValue::defaultLocalChannelMessageSize, &messageSize, dataSize)) [[unlikely]]
      return SuperDefault; // If this path is taken, there's likely something very wrong

    return messageSize;
  }
  inline AgtSize getDefaultLocalChannelSlotCount(AgtContext ctx) noexcept {
    // Default to be used if default lookup fails
    // Using both SuperDefaults, each channel would be 64 KiB in size
    constexpr static AgtSize SuperDefault = 256;


    AgtSize slotCount;
    AgtSize dataSize = sizeof(AgtSize);

    if (!ctxGetBuiltinValue(ctx, BuiltinValue::defaultLocalChannelSlotCount, &slotCount, dataSize)) [[unlikely]]
      return SuperDefault; // If this path is taken, there's likely something very wrong

    return slotCount;
  }
  inline AgtSize getDefaultSharedChannelMessageSize(AgtContext ctx) noexcept {

    // Default to be used if default lookup fails
    // Each slot is 1024 bytes, or 16 cachelines long.
    constexpr static AgtSize SuperDefault = AGT_CACHE_LINE * 15;

    AgtSize messageSize;
    AgtSize dataSize = sizeof(AgtSize);

    if (!ctxGetBuiltinValue(ctx, BuiltinValue::defaultSharedChannelMessageSize, &messageSize, dataSize)) [[unlikely]]
      return SuperDefault; // If this path is taken, there's likely something very wrong

    return messageSize;
  }
  inline AgtSize getDefaultSharedChannelSlotCount(AgtContext ctx) noexcept {
    // Default to be used if default lookup fails
    // Using both SuperDefaults, each channel would be 64 KiB in size
    constexpr static AgtSize SuperDefault = 256;


    AgtSize slotCount;
    AgtSize dataSize = sizeof(AgtSize);

    if (!ctxGetBuiltinValue(ctx, BuiltinValue::defaultSharedChannelSlotCount, &slotCount, dataSize)) [[unlikely]]
      return SuperDefault; // If this path is taken, there's likely something very wrong

    return slotCount;
  }


}










/** =================================[ PrivateChannel ]===================================================== */

/*PrivateChannel::PrivateChannel(AgtStatus& status, AgtContext ctx, AgtObjectId id, AgtSize slotCount, AgtSize slotSize) noexcept
    : LocalChannel(status, ObjectType::privateChannel, ctx, id, slotCount, slotSize){
  
}*/

template <>
AgtStatus ObjectInfo<PrivateChannel>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  auto channel = static_cast<PrivateChannel*>(object);
  auto& stagedMessage = reinterpret_cast<StagedMessage&>(*pStagedMessage);
  if ( channel->availableSlotCount == 0 ) [[unlikely]]
    return AGT_ERROR_MAILBOX_IS_FULL;
  if ( stagedMessage.messageSize > channel->inlineBufferSize) [[unlikely]]
    return AGT_ERROR_MESSAGE_TOO_LARGE;

  --channel->availableSlotCount;
  auto acquiredMsg = channel->nextFreeSlot;
  channel->nextFreeSlot = acquiredMsg->next;

  stagedMessage.receiver = channel;
  stagedMessage.message  = acquiredMsg;
  stagedMessage.payload  = acquiredMsg->inlineBuffer;
  if (stagedMessage.messageSize == 0)
    stagedMessage.messageSize = channel->inlineBufferSize;

  return AGT_SUCCESS;
}
template <>
void      ObjectInfo<PrivateChannel>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  auto channel = static_cast<PrivateChannel*>(object);
  privatePlaceHold(message);
  channel->prevQueuedMessage->next = message;
  channel->prevQueuedMessage = message;
  ++channel->queuedMessageCount;
}
template <>
AgtStatus ObjectInfo<PrivateChannel>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  auto channel = static_cast<PrivateChannel*>(object);
  if ( channel->queuedMessageCount == 0 )
    return AGT_ERROR_MAILBOX_IS_EMPTY;
  auto next = channel->prevReceivedMessage->next;
  privateReleaseHold(channel, channel->prevReceivedMessage);
  channel->prevReceivedMessage = next;
  pMessageInfo->message = next;
  pMessageInfo->size    = next->payloadSize;
  pMessageInfo->id      = next->id;
  pMessageInfo->payload = next->inlineBuffer;
  return AGT_SUCCESS;
}
template <>
void      ObjectInfo<PrivateChannel>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  auto channel = static_cast<PrivateChannel*>(object);
  ++channel->availableSlotCount;
  message->next = channel->nextFreeSlot;
  channel->nextFreeSlot = message;
}

template <>
AgtStatus ObjectInfo<PrivateChannel>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
AgtStatus ObjectInfo<PrivateChannel>::acquireRef(HandleHeader* object) noexcept {
  ++static_cast<PrivateChannel*>(object)->refCount;
}
template <>
void      ObjectInfo<PrivateChannel>::releaseRef(HandleHeader* object) noexcept {
  auto channel = static_cast<PrivateChannel*>(object);
  if (!--channel->refCount)
    destroy(channel);
}


AgtStatus Agt::createInstance(PrivateChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, PrivateChannelSender* sender, PrivateChannelReceiver* receiver) noexcept {

  const AgtSize messageSize = createInfo.maxMessageSize
                                ? alignSize(createInfo.maxMessageSize, AGT_CACHE_LINE)
                                : getDefaultPrivateChannelMessageSize(ctx);
  const AgtSize slotCount   = createInfo.minCapacity
                                ? createInfo.minCapacity
                                : getDefaultPrivateChannelSlotCount(ctx);

  auto channel = (PrivateChannel*)ctxAllocHandle(ctx, sizeof(PrivateChannel), AGT_CACHE_LINE);

  if (!channel) [[unlikely]] {
    outHandle = nullptr;
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->type = ObjectType::privateChannel;
  channel->flags = FlagsNotSet;
  channel->context = ctx;

  channel->slotCount = slotCount;
  channel->inlineBufferSize = messageSize;

  if (!initMessageArray(channel)) {
    ctxFreeHandle(ctx, channel, sizeof(PrivateChannel), alignof(PrivateChannel));
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->consumer            = receiver;
  channel->producer            = sender;
  channel->availableSlotCount  = slotCount - 1;
  channel->queuedMessageCount  = 0;
  channel->nextFreeSlot        = (AgtMessage)channel->messageSlots;
  channel->prevReceivedMessage = (AgtMessage)(channel->messageSlots + ((slotCount - 1) * (messageSize + AGT_CACHE_LINE)));
  channel->prevQueuedMessage   = channel->prevReceivedMessage;
  channel->refCount            = static_cast<AgtUInt32>(static_cast<bool>(receiver)) + static_cast<bool>(sender);

  if (createInfo.name) {
    if (AgtStatus status = ctxRegisterNamedObject(ctx, channel, createInfo.name)) {
      ctxLocalFree(ctx, channel->messageSlots, slotCount * (messageSize + AGT_CACHE_LINE), AGT_CACHE_LINE);
      ctxFreeHandle(ctx, channel, sizeof(PrivateChannel), alignof(PrivateChannel));
      return status;
    }
  }

  if (sender) {
    sender->channel = channel;
  }
  if (receiver) {
    receiver->channel = channel;
  }

  return AGT_SUCCESS;
}


/** =================================[ LocalSpScChannel ]===================================================== */


template <>
AgtStatus ObjectInfo<LocalSpScChannel>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  auto channel = static_cast<LocalSpScChannel*>(object);
  auto& stagedMessage = reinterpret_cast<StagedMessage&>(*pStagedMessage);
  if ( stagedMessage.messageSize > channel->inlineBufferSize) [[unlikely]]
    return AGT_ERROR_MESSAGE_TOO_LARGE;

  AGT_acquire_semaphore(channel->slotSemaphore, timeout, AGT_ERROR_MAILBOX_IS_FULL);

  auto message = channel->producerNextFreeSlot;
  channel->producerNextFreeSlot = message->next;
  stagedMessage.message  = message;
  stagedMessage.payload  = message->inlineBuffer;
  stagedMessage.receiver = channel;
  if (stagedMessage.messageSize == 0)
    stagedMessage.messageSize = channel->inlineBufferSize;

  return AGT_SUCCESS;
}
template <>
void      ObjectInfo<LocalSpScChannel>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  auto channel = static_cast<LocalSpScChannel*>(object);
  placeHold(message);
  channel->producerPreviousQueuedMsg->next = message;
  channel->producerPreviousQueuedMsg = message;
  channel->queuedMessages.release();
}
template <>
AgtStatus ObjectInfo<LocalSpScChannel>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  auto channel = static_cast<LocalSpScChannel*>(object);
  AGT_acquire_semaphore(channel->queuedMessages, timeout, AGT_ERROR_MAILBOX_IS_EMPTY);
  auto previousMsg = channel->consumerPreviousMsg;
  auto message = previousMsg->next;
  channel->consumerPreviousMsg = message;
  releaseHold(channel, previousMsg);
  pMessageInfo->message = (AgtMessage)message;
  pMessageInfo->size    = message->payloadSize;
  pMessageInfo->id      = message->id;
  pMessageInfo->payload = message->inlineBuffer;
  return AGT_SUCCESS;
}
template <>
void      ObjectInfo<LocalSpScChannel>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  auto channel = static_cast<LocalSpScChannel*>(object);
  auto lastFreeSlot = channel->lastFreeSlot;
  channel->lastFreeSlot = message;
  lastFreeSlot->next = message;
  channel->slotSemaphore.release();
}

template <>
AgtStatus ObjectInfo<LocalSpScChannel>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {}

template <>
AgtStatus ObjectInfo<LocalSpScChannel>::acquireRef(HandleHeader* object) noexcept {}
template <>
void      ObjectInfo<LocalSpScChannel>::releaseRef(HandleHeader* object) noexcept {}


AgtStatus Agt::createInstance(LocalSpScChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, LocalSpScChannelSender* sender, LocalSpScChannelReceiver* receiver) noexcept {
  const AgtSize messageSize = createInfo.maxMessageSize
                                ? alignSize(createInfo.maxMessageSize, AGT_CACHE_LINE)
                                : getDefaultLocalChannelMessageSize(ctx);
  const AgtSize slotCount   = createInfo.minCapacity
                              ? createInfo.minCapacity
                              : getDefaultLocalChannelSlotCount(ctx);
  
  auto channel = (LocalSpScChannel*)ctxAllocHandle(ctx, sizeof(LocalSpScChannel), AGT_CACHE_LINE);

  if (!channel) [[unlikely]] {
    outHandle = nullptr;
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->type = ObjectType::spscChannel;
  channel->flags = FlagsNotSet;
  channel->context = ctx;

  channel->slotCount = slotCount;
  channel->inlineBufferSize = messageSize;

  if (!initMessageArray(channel)) {
    ctxFreeHandle(ctx, channel, sizeof(LocalSpScChannel), alignof(LocalSpScChannel));
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->consumer            = receiver;
  channel->producer            = sender;
  new (&channel->slotSemaphore) semaphore_t((ptrdiff_t)(slotCount - 1));
  new (&channel->queuedMessages) semaphore_t(0);
  channel->producerNextFreeSlot      = (AgtMessage)channel->messageSlots;
  channel->consumerPreviousMsg       = (AgtMessage)(channel->messageSlots + ((slotCount - 1) * (messageSize + AGT_CACHE_LINE)));
  channel->producerPreviousQueuedMsg = channel->consumerPreviousMsg;
  new (&channel->refCount) ReferenceCount(static_cast<AgtUInt32>(static_cast<bool>(receiver)) + static_cast<bool>(sender));

  if (createInfo.name) {
    if (AgtStatus status = ctxRegisterNamedObject(ctx, channel, createInfo.name)) {
      ctxLocalFree(ctx, channel->messageSlots, slotCount * (messageSize + AGT_CACHE_LINE), AGT_CACHE_LINE);
      ctxFreeHandle(ctx, channel, sizeof(PrivateChannel), alignof(PrivateChannel));
      return status;
    }
  }

  if (sender) {
    sender->channel = channel;
  }
  if (receiver) {
    receiver->channel = channel;
  }

  outHandle = channel;

  return AGT_SUCCESS;
}


/** =================================[ LocalSpMcChannel ]===================================================== */

template <>
AgtStatus ObjectInfo<LocalSpMcChannel>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  auto channel = static_cast<LocalSpMcChannel*>(object);
  auto& stagedMessage = reinterpret_cast<StagedMessage&>(*pStagedMessage);
  if ( stagedMessage.messageSize > channel->inlineBufferSize) [[unlikely]]
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  
  AGT_acquire_semaphore(channel->slotSemaphore, timeout, AGT_ERROR_MAILBOX_IS_FULL);

  auto message = channel->nextFreeSlot;
  channel->nextFreeSlot = message->next;
  stagedMessage.message  = message;
  stagedMessage.payload  = message->inlineBuffer;
  stagedMessage.receiver = channel;
  if (stagedMessage.messageSize == 0)
    stagedMessage.messageSize = channel->inlineBufferSize;

  return AGT_SUCCESS;
}
template <>
void      ObjectInfo<LocalSpMcChannel>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  auto channel = static_cast<LocalSpMcChannel*>(object);
  placeHold(message);
  channel->lastQueuedSlot->next = message;
  channel->lastQueuedSlot = message;
  channel->queuedMessages.release();
}
template <>
AgtStatus ObjectInfo<LocalSpMcChannel>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  auto channel = static_cast<LocalSpMcChannel*>(object);
  AGT_acquire_semaphore(channel->queuedMessages, timeout, AGT_ERROR_MAILBOX_IS_EMPTY);

  auto prevReceivedMsg = channel->previousReceivedMessage.load(std::memory_order_acquire);
  AgtMessage message;
  do {
    message = prevReceivedMsg->next;
  } while ( !channel->previousReceivedMessage.compare_exchange_weak(prevReceivedMsg, message) );
  releaseHold(channel, prevReceivedMsg);

  pMessageInfo->message = (AgtMessage)message;
  pMessageInfo->size    = message->payloadSize;
  pMessageInfo->id      = message->id;
  pMessageInfo->payload = message->inlineBuffer;

  return AGT_SUCCESS;
}
template <>
void      ObjectInfo<LocalSpMcChannel>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  auto channel = static_cast<LocalSpMcChannel*>(object);
  auto lastFreeSlot = channel->lastFreeSlot.exchange(message);
  lastFreeSlot->next = message;
  channel->slotSemaphore.release();
}

template <>
AgtStatus ObjectInfo<LocalSpMcChannel>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {}

template <>
AgtStatus ObjectInfo<LocalSpMcChannel>::acquireRef(HandleHeader* object) noexcept {}
template <>
void      ObjectInfo<LocalSpMcChannel>::releaseRef(HandleHeader* object) noexcept {}


AgtStatus Agt::createInstance(LocalSpMcChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, LocalSpMcChannelSender* sender, LocalSpMcChannelReceiver* receiver) noexcept {
  const AgtSize messageSize = createInfo.maxMessageSize
                                ? alignSize(createInfo.maxMessageSize, AGT_CACHE_LINE)
                                : getDefaultLocalChannelMessageSize(ctx);
  const AgtSize slotCount   = createInfo.minCapacity
                              ? createInfo.minCapacity
                              : getDefaultLocalChannelSlotCount(ctx);
  
  auto channel = (LocalSpMcChannel*)ctxAllocHandle(ctx, sizeof(LocalSpMcChannel), AGT_CACHE_LINE);

  if (!channel) [[unlikely]] {
    outHandle = nullptr;
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->type = ObjectType::spmcChannel;
  channel->flags = FlagsNotSet;
  channel->context = ctx;

  channel->slotCount = slotCount;
  channel->inlineBufferSize = messageSize;

  if (!initMessageArray(channel)) {
    ctxFreeHandle(ctx, channel, sizeof(LocalSpMcChannel), alignof(LocalSpMcChannel));
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->maxConsumers        = createInfo.maxReceivers;
  new (&channel->consumerSemaphore) semaphore_t(createInfo.maxReceivers);
  channel->producer            = sender;
  new (&channel->slotSemaphore) semaphore_t((ptrdiff_t)(slotCount - 1));
  new (&channel->queuedMessages) semaphore_t(0);
  channel->nextFreeSlot = (AgtMessage)channel->messageSlots;
  channel->lastQueuedSlot = (AgtMessage)(channel->messageSlots + ((slotCount - 1) * (messageSize + AGT_CACHE_LINE)));
  new (&channel->previousReceivedMessage) std::atomic<AgtMessage>(channel->lastQueuedSlot);
  new (&channel->refCount) ReferenceCount(static_cast<AgtUInt32>(static_cast<bool>(receiver)) + static_cast<bool>(sender));

  if (createInfo.name) {
    if (AgtStatus status = ctxRegisterNamedObject(ctx, channel, createInfo.name)) {
      ctxLocalFree(ctx, channel->messageSlots, slotCount * (messageSize + AGT_CACHE_LINE), AGT_CACHE_LINE);
      ctxFreeHandle(ctx, channel, sizeof(PrivateChannel), alignof(PrivateChannel));
      return status;
    }
  }

  if (sender) {
    sender->channel = channel;
  }
  if (receiver) {
    receiver->channel = channel;
  }

  outHandle = channel;

  return AGT_SUCCESS;
}


/** =================================[ LocalMpMcChannel ]===================================================== */

template <>
AgtStatus ObjectInfo<LocalMpMcChannel>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  auto channel = static_cast<HandleType*>(object);
  auto& stagedMessage = reinterpret_cast<StagedMessage&>(*pStagedMessage);
  AgtMessage message;
  if ( stagedMessage.messageSize > channel->inlineBufferSize) [[unlikely]]
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  
  AGT_acquire_semaphore(channel->slotSemaphore, timeout, AGT_ERROR_MAILBOX_IS_FULL);

  message = channel->nextFreeSlot.load(std::memory_order_acquire);
  while ( !channel->nextFreeSlot.compare_exchange_weak(message, message->next) );

  stagedMessage.message  = message;
  stagedMessage.payload  = message->inlineBuffer;
  stagedMessage.receiver = channel;
  if (stagedMessage.messageSize == 0)
    stagedMessage.messageSize = channel->inlineBufferSize;

  return AGT_SUCCESS;
}
template <>
void      ObjectInfo<LocalMpMcChannel>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  auto channel = static_cast<HandleType*>(object);
  placeHold(message);
  AgtMessage lastQueuedSlot = channel->lastQueuedSlot.load(std::memory_order_acquire);
  do {
    lastQueuedSlot->next = message;
  } while ( !channel->lastQueuedSlot.compare_exchange_weak(lastQueuedSlot, message) );
  channel->queuedMessages.release();
}
template <>
AgtStatus ObjectInfo<LocalMpMcChannel>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  auto channel = static_cast<HandleType*>(object);
  AGT_acquire_semaphore(channel->queuedMessages, timeout, AGT_ERROR_MAILBOX_IS_EMPTY);

  auto prevReceivedMsg = channel->previousReceivedMessage.load(std::memory_order_acquire);
  AgtMessage message;
  do {
    message = prevReceivedMsg->next;
  } while ( !channel->previousReceivedMessage.compare_exchange_weak(prevReceivedMsg, message) );
  releaseHold(channel, prevReceivedMsg);

  pMessageInfo->message = (AgtMessage)message;
  pMessageInfo->size    = message->payloadSize;
  pMessageInfo->id      = message->id;
  pMessageInfo->payload = message->inlineBuffer;

  return AGT_SUCCESS;
}
template <>
void      ObjectInfo<LocalMpMcChannel>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  auto channel = static_cast<HandleType*>(object);
  auto nextFreeSlot = channel->nextFreeSlot.load();
  do {
    message->next = nextFreeSlot;
  } while( !channel->nextFreeSlot.compare_exchange_weak(nextFreeSlot, message) );
  nextFreeSlot->next = message;
  channel->slotSemaphore.release();
}

template <>
AgtStatus ObjectInfo<LocalMpMcChannel>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {}

template <>
AgtStatus ObjectInfo<LocalMpMcChannel>::acquireRef(HandleHeader* object) noexcept {}
template <>
void      ObjectInfo<LocalMpMcChannel>::releaseRef(HandleHeader* object) noexcept {}

AgtStatus Agt::createInstance(LocalMpMcChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, LocalMpMcChannelSender* sender, LocalMpMcChannelReceiver* receiver) noexcept {
  const AgtSize messageSize = createInfo.maxMessageSize
                                ? alignSize(createInfo.maxMessageSize, AGT_CACHE_LINE)
                                : getDefaultLocalChannelMessageSize(ctx);
  const AgtSize slotCount   = createInfo.minCapacity
                              ? createInfo.minCapacity
                              : getDefaultLocalChannelSlotCount(ctx);

  auto channel = (LocalMpMcChannel*)ctxAllocHandle(ctx, sizeof(LocalMpMcChannel), AGT_CACHE_LINE);

  if (!channel) [[unlikely]] {
    outHandle = nullptr;
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->type = ObjectType::mpscChannel;
  channel->flags = FlagsNotSet;
  channel->context = ctx;

  channel->slotCount = slotCount;
  channel->inlineBufferSize = messageSize;

  if (!initMessageArray(channel)) {
    ctxFreeHandle(ctx, channel, sizeof(LocalMpMcChannel), alignof(LocalMpMcChannel));
    return AGT_ERROR_BAD_ALLOC;
  }

  AgtMessage lastMessageInArray = (AgtMessage)(channel->messageSlots + ((slotCount - 1) * (messageSize + AGT_CACHE_LINE)));

  channel->maxConsumers = createInfo.maxReceivers;
  new (&channel->consumerSemaphore) semaphore_t((ptrdiff_t)createInfo.maxReceivers);
  channel->maxProducers = createInfo.maxSenders;
  new (&channel->producerSemaphore) semaphore_t((ptrdiff_t)createInfo.maxSenders);
  new (&channel->slotSemaphore) semaphore_t((ptrdiff_t)(slotCount - 1));
  new (&channel->queuedMessages) semaphore_t(0);
  new (&channel->nextFreeSlot) std::atomic<AgtMessage>((AgtMessage)channel->messageSlots);
  new (&channel->previousReceivedMessage) std::atomic<AgtMessage>(lastMessageInArray);
  new (&channel->lastQueuedSlot) std::atomic<AgtMessage>(lastMessageInArray);
  new (&channel->refCount) ReferenceCount(static_cast<AgtUInt32>(static_cast<bool>(receiver)) + static_cast<bool>(sender));

  if (createInfo.name) {
    if (AgtStatus status = ctxRegisterNamedObject(ctx, channel, createInfo.name)) {
      ctxLocalFree(ctx, channel->messageSlots, slotCount * (messageSize + AGT_CACHE_LINE), AGT_CACHE_LINE);
      ctxFreeHandle(ctx, channel, sizeof(PrivateChannel), alignof(PrivateChannel));
      return status;
    }
  }

  if (sender) {
    sender->channel = channel;
  }
  if (receiver) {
    receiver->channel = channel;
  }

  outHandle = channel;

  return AGT_SUCCESS;
}

/** =================================[ LocalMpScChannel ]===================================================== */

template <>
AgtStatus ObjectInfo<LocalMpScChannel>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  auto channel = static_cast<LocalMpScChannel*>(object);
  auto& stagedMessage = reinterpret_cast<StagedMessage&>(*pStagedMessage);
  if ( stagedMessage.messageSize > channel->inlineBufferSize) [[unlikely]]
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  
  AGT_acquire_semaphore(channel->slotSemaphore, timeout, AGT_ERROR_MAILBOX_IS_FULL);


  auto message = channel->nextFreeSlot.load(std::memory_order_acquire);
  while ( !channel->nextFreeSlot.compare_exchange_weak(message, message->next) );

  stagedMessage.message  = message;
  stagedMessage.payload  = message->inlineBuffer;
  stagedMessage.receiver = channel;
  if (stagedMessage.messageSize == 0)
    stagedMessage.messageSize = channel->inlineBufferSize;

  return AGT_SUCCESS;
}
template <>
void      ObjectInfo<LocalMpScChannel>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  auto channel = static_cast<LocalMpScChannel*>(object);
  auto lastQueuedMessage = channel->lastQueuedSlot.load(std::memory_order_acquire);
  placeHold(message);
  do {
    lastQueuedMessage->next = message;
  } while ( !channel->lastQueuedSlot.compare_exchange_weak(lastQueuedMessage, message) );
  channel->queuedMessageCount.increase(1);
}
template <>
AgtStatus ObjectInfo<LocalMpScChannel>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  auto channel = static_cast<LocalMpScChannel*>(object);
  switch ( timeout ) {
    case AGT_WAIT:
      channel->queuedMessageCount.decrease(1);
      break;
    case AGT_DO_NOT_WAIT:
      if ( !channel->queuedMessageCount.try_decrease(1) )
        return AGT_ERROR_MAILBOX_IS_EMPTY;
      break;
    default:
      if ( !channel->queuedMessageCount.try_decrease_for(1, timeout) )
        return AGT_ERROR_MAILBOX_IS_EMPTY;
  }
  auto previousMsg = channel->previousReceivedMessage;
  auto message = previousMsg->next;
  channel->previousReceivedMessage = message;
  releaseHold(channel, previousMsg);
  pMessageInfo->message = (AgtMessage)message;
  pMessageInfo->size    = message->payloadSize;
  pMessageInfo->id      = message->id;
  pMessageInfo->payload = message->inlineBuffer;
  return AGT_SUCCESS;
}
template <>
void      ObjectInfo<LocalMpScChannel>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  auto channel = static_cast<LocalMpScChannel*>(object);
  auto lastFreeSlot = channel->lastFreeSlot.load();
  do {
    lastFreeSlot->next = message;
  } while( !channel->lastFreeSlot.compare_exchange_weak(lastFreeSlot, message) );
  // FIXME: I think this is still a potential bug, though it might be one thats hard to detect....
  lastFreeSlot->next = message;
}

template <>
AgtStatus ObjectInfo<LocalMpScChannel>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {}

template <>
AgtStatus ObjectInfo<LocalMpScChannel>::acquireRef(HandleHeader* object) noexcept {}
template <>
void      ObjectInfo<LocalMpScChannel>::releaseRef(HandleHeader* object) noexcept {}


AgtStatus Agt::createInstance(LocalMpScChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, LocalMpScChannelSender* sender, LocalMpScChannelReceiver* receiver) noexcept {
  const AgtSize messageSize = createInfo.maxMessageSize
                                ? alignSize(createInfo.maxMessageSize, AGT_CACHE_LINE)
                                : getDefaultLocalChannelMessageSize(ctx);
  const AgtSize slotCount   = createInfo.minCapacity
                              ? createInfo.minCapacity
                              : getDefaultLocalChannelSlotCount(ctx);
  
  auto channel = (LocalMpScChannel*)ctxAllocHandle(ctx, sizeof(LocalMpScChannel), AGT_CACHE_LINE);

  if (!channel) [[unlikely]] {
    outHandle = nullptr;
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->type = ObjectType::mpscChannel;
  channel->flags = FlagsNotSet;
  channel->context = ctx;

  channel->slotCount = slotCount;
  channel->inlineBufferSize = messageSize;

  if (!initMessageArray(channel)) {
    ctxFreeHandle(ctx, channel, sizeof(LocalMpScChannel), alignof(LocalMpScChannel));
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->consumer            = receiver;
  channel->maxProducers = createInfo.maxSenders;
  new (&channel->producerSemaphore) semaphore_t((ptrdiff_t)createInfo.maxSenders);
  new (&channel->slotSemaphore) semaphore_t((ptrdiff_t)(slotCount - 1));
  new (&channel->queuedMessageCount) semaphore_t(0);
  new (&channel->nextFreeSlot) std::atomic<AgtMessage>((AgtMessage)channel->messageSlots);
  channel->previousReceivedMessage = (AgtMessage)(channel->messageSlots + ((slotCount - 1) * (messageSize + AGT_CACHE_LINE)));
  new (&channel->lastQueuedSlot) std::atomic<AgtMessage>(channel->previousReceivedMessage);
  new (&channel->refCount) ReferenceCount(static_cast<AgtUInt32>(static_cast<bool>(receiver)) + static_cast<bool>(sender));

  if (createInfo.name) {
    if (AgtStatus status = ctxRegisterNamedObject(ctx, channel, createInfo.name)) {
      ctxLocalFree(ctx, channel->messageSlots, slotCount * (messageSize + AGT_CACHE_LINE), AGT_CACHE_LINE);
      ctxFreeHandle(ctx, channel, sizeof(PrivateChannel), alignof(PrivateChannel));
      return status;
    }
  }

  if (sender) {
    sender->channel = channel;
  }
  if (receiver) {
    receiver->channel = channel;
  }

  outHandle = channel;

  return AGT_SUCCESS;
}







/** =================================[ PrivateChannelSender ]===================================================== */

template <>
AgtStatus ObjectInfo<PrivateChannelSender>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  auto sender = static_cast<HandleType*>(object);
  if (!sender->channel)
    return AGT_ERROR_NOT_BOUND;
  return ObjectInfo<ObjectType>::acquireMessage(sender->channel, pStagedMessage, timeout);
}

template <>
void      ObjectInfo<PrivateChannelSender>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  auto sender = static_cast<HandleType*>(object);
  ObjectInfo<ObjectType>::pushQueue(sender->channel, message, flags);
}

template <>
AgtStatus ObjectInfo<PrivateChannelSender>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
  // auto sender = static_cast<HandleType*>(object);
  // if (!sender->channel)
  //   return AGT_ERROR_NOT_BOUND;
  // return ObjectInfo<ObjectType>::popQueue(sender->channel, pMessageInfo, timeout);
}

template <>
void      ObjectInfo<PrivateChannelSender>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  // ObjectInfo<HandleType>::releaseMessage(static_cast<HandleType*>(object)->channel, message);
}

template <>
AgtStatus ObjectInfo<PrivateChannelSender>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {
  auto sender = static_cast<HandleType*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
AgtStatus ObjectInfo<PrivateChannelSender>::acquireRef(HandleHeader* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      ObjectInfo<PrivateChannelSender>::releaseRef(HandleHeader* object) noexcept { }

/** =================================[ PrivateChannelReceiver ]=================================================== */

template <>
AgtStatus ObjectInfo<PrivateChannelReceiver>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
}

template <>
void      ObjectInfo<PrivateChannelReceiver>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  // auto sender = static_cast<HandleType*>(object);
  // ObjectInfo<PrivateChannel>::pushQueue(sender->channel, message, flags);
}

template <>
AgtStatus ObjectInfo<PrivateChannelReceiver>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  auto receiver = static_cast<HandleType*>(object);
  if (!receiver->channel)
    return AGT_ERROR_NOT_BOUND;
  return ObjectInfo<ObjectType>::popQueue(receiver->channel, pMessageInfo, timeout);
}

template <>
void      ObjectInfo<PrivateChannelReceiver>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  ObjectInfo<ObjectType>::releaseMessage(static_cast<HandleType*>(object)->channel, message);
}

template <>
AgtStatus ObjectInfo<PrivateChannelReceiver>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {
  auto receiver = static_cast<HandleType*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
AgtStatus ObjectInfo<PrivateChannelReceiver>::acquireRef(HandleHeader* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      ObjectInfo<PrivateChannelReceiver>::releaseRef(HandleHeader* object) noexcept { }

/** =================================[ LocalSpScChannelSender ]=================================================== */

template <>
AgtStatus ObjectInfo<LocalSpScChannelSender>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  auto sender = static_cast<HandleType*>(object);
  if (!sender->channel)
    return AGT_ERROR_NOT_BOUND;
  return ObjectInfo<ObjectType>::acquireMessage(sender->channel, pStagedMessage, timeout);
}

template <>
void      ObjectInfo<LocalSpScChannelSender>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  auto sender = static_cast<HandleType*>(object);
  ObjectInfo<ObjectType>::pushQueue(sender->channel, message, flags);
}

template <>
AgtStatus ObjectInfo<LocalSpScChannelSender>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
  // auto sender = static_cast<HandleType*>(object);
  // if (!sender->channel)
  //   return AGT_ERROR_NOT_BOUND;
  // return ObjectInfo<ObjectType>::popQueue(sender->channel, pMessageInfo, timeout);
}

template <>
void      ObjectInfo<LocalSpScChannelSender>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  // ObjectInfo<ObjectType>::releaseMessage(static_cast<HandleType*>(object)->channel, message);
}

template <>
AgtStatus ObjectInfo<LocalSpScChannelSender>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {
  auto sender = static_cast<HandleType*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
AgtStatus ObjectInfo<LocalSpScChannelSender>::acquireRef(HandleHeader* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      ObjectInfo<LocalSpScChannelSender>::releaseRef(HandleHeader* object) noexcept { }


AgtStatus Agt::createInstance(LocalSpScChannelSender*& objectRef, AgtContext ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/** =================================[ LocalSpScChannelReceiver ]================================================= */

template <>
AgtStatus ObjectInfo<LocalSpScChannelReceiver>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
}

template <>
void      ObjectInfo<LocalSpScChannelReceiver>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  // auto sender = static_cast<HandleType*>(object);
  // ObjectInfo<PrivateChannel>::pushQueue(sender->channel, message, flags);
}

template <>
AgtStatus ObjectInfo<LocalSpScChannelReceiver>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  auto receiver = static_cast<HandleType*>(object);
  if (!receiver->channel)
    return AGT_ERROR_NOT_BOUND;
  return ObjectInfo<ObjectType>::popQueue(receiver->channel, pMessageInfo, timeout);
}

template <>
void      ObjectInfo<LocalSpScChannelReceiver>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  ObjectInfo<ObjectType>::releaseMessage(static_cast<HandleType*>(object)->channel, message);
}

template <>
AgtStatus ObjectInfo<LocalSpScChannelReceiver>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {
  auto receiver = static_cast<HandleType*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
AgtStatus ObjectInfo<LocalSpScChannelReceiver>::acquireRef(HandleHeader* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      ObjectInfo<LocalSpScChannelReceiver>::releaseRef(HandleHeader* object) noexcept { }

AgtStatus Agt::createInstance(LocalSpScChannelReceiver*& objectRef, AgtContext ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/** =================================[ LocalSpMcChannelSender ]=================================================== */

template <>
AgtStatus ObjectInfo<LocalSpMcChannelSender>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  auto sender = static_cast<HandleType*>(object);
  if (!sender->channel)
    return AGT_ERROR_NOT_BOUND;
  return ObjectInfo<ObjectType>::acquireMessage(sender->channel, pStagedMessage, timeout);
}

template <>
void      ObjectInfo<LocalSpMcChannelSender>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  auto sender = static_cast<HandleType*>(object);
  ObjectInfo<ObjectType>::pushQueue(sender->channel, message, flags);
}

template <>
AgtStatus ObjectInfo<LocalSpMcChannelSender>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
  // auto sender = static_cast<HandleType*>(object);
  // if (!sender->channel)
  //   return AGT_ERROR_NOT_BOUND;
  // return ObjectInfo<ObjectType>::popQueue(sender->channel, pMessageInfo, timeout);
}

template <>
void      ObjectInfo<LocalSpMcChannelSender>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  // ObjectInfo<HandleType>::releaseMessage(static_cast<HandleType*>(object)->channel, message);
}

template <>
AgtStatus ObjectInfo<LocalSpMcChannelSender>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {
  auto sender = static_cast<HandleType*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
AgtStatus ObjectInfo<LocalSpMcChannelSender>::acquireRef(HandleHeader* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      ObjectInfo<LocalSpMcChannelSender>::releaseRef(HandleHeader* object) noexcept { }

/** =================================[ LocalSpMcChannelReceiver ]================================================= */

template <>
AgtStatus ObjectInfo<LocalSpMcChannelReceiver>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
}

template <>
void      ObjectInfo<LocalSpMcChannelReceiver>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  // auto sender = static_cast<HandleType*>(object);
  // ObjectInfo<PrivateChannel>::pushQueue(sender->channel, message, flags);
}

template <>
AgtStatus ObjectInfo<LocalSpMcChannelReceiver>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  auto receiver = static_cast<HandleType*>(object);
  if (!receiver->channel)
    return AGT_ERROR_NOT_BOUND;
  return ObjectInfo<ObjectType>::popQueue(receiver->channel, pMessageInfo, timeout);
}

template <>
void      ObjectInfo<LocalSpMcChannelReceiver>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  ObjectInfo<ObjectType>::releaseMessage(static_cast<HandleType*>(object)->channel, message);
}

template <>
AgtStatus ObjectInfo<LocalSpMcChannelReceiver>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {
  auto receiver = static_cast<HandleType*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
AgtStatus ObjectInfo<LocalSpMcChannelReceiver>::acquireRef(HandleHeader* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      ObjectInfo<LocalSpMcChannelReceiver>::releaseRef(HandleHeader* object) noexcept { }

AgtStatus Agt::createInstance(LocalSpMcChannelReceiver*& objectRef, AgtContext ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/** =================================[ LocalMpScChannelSender ]=================================================== */

template <>
AgtStatus ObjectInfo<LocalMpScChannelSender>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  auto sender = static_cast<HandleType*>(object);
  if (!sender->channel)
    return AGT_ERROR_NOT_BOUND;
  return ObjectInfo<ObjectType>::acquireMessage(sender->channel, pStagedMessage, timeout);
}

template <>
void      ObjectInfo<LocalMpScChannelSender>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  auto sender = static_cast<HandleType*>(object);
  ObjectInfo<ObjectType>::pushQueue(sender->channel, message, flags);
}

template <>
AgtStatus ObjectInfo<LocalMpScChannelSender>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
  // auto sender = static_cast<HandleType*>(object);
  // if (!sender->channel)
  //   return AGT_ERROR_NOT_BOUND;
  // return ObjectInfo<ObjectType>::popQueue(sender->channel, pMessageInfo, timeout);
}

template <>
void      ObjectInfo<LocalMpScChannelSender>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  // ObjectInfo<HandleType>::releaseMessage(static_cast<HandleType*>(object)->channel, message);
}

template <>
AgtStatus ObjectInfo<LocalMpScChannelSender>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {
  auto sender = static_cast<HandleType*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
AgtStatus ObjectInfo<LocalMpScChannelSender>::acquireRef(HandleHeader* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      ObjectInfo<LocalMpScChannelSender>::releaseRef(HandleHeader* object) noexcept { }


AgtStatus Agt::createInstance(LocalMpScChannelSender*& objectRef, AgtContext ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/** =================================[ LocalMpScChannelReceiver ]================================================= */

template <>
AgtStatus ObjectInfo<LocalMpScChannelReceiver>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
}

template <>
void      ObjectInfo<LocalMpScChannelReceiver>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  // auto sender = static_cast<HandleType*>(object);
  // ObjectInfo<PrivateChannel>::pushQueue(sender->channel, message, flags);
}

template <>
AgtStatus ObjectInfo<LocalMpScChannelReceiver>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  auto receiver = static_cast<HandleType*>(object);
  if (!receiver->channel)
    return AGT_ERROR_NOT_BOUND;
  return ObjectInfo<ObjectType>::popQueue(receiver->channel, pMessageInfo, timeout);
}

template <>
void      ObjectInfo<LocalMpScChannelReceiver>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  ObjectInfo<ObjectType>::releaseMessage(static_cast<HandleType*>(object)->channel, message);
}

template <>
AgtStatus ObjectInfo<LocalMpScChannelReceiver>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {
  auto receiver = static_cast<HandleType*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
AgtStatus ObjectInfo<LocalMpScChannelReceiver>::acquireRef(HandleHeader* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      ObjectInfo<LocalMpScChannelReceiver>::releaseRef(HandleHeader* object) noexcept { }

AgtStatus Agt::createInstance(LocalMpScChannelReceiver*& objectRef, AgtContext ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/** =================================[ LocalMpMcChannelSender ]=================================================== */

template <>
AgtStatus ObjectInfo<LocalMpMcChannelSender>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  auto sender = static_cast<HandleType*>(object);
  if (!sender->channel)
    return AGT_ERROR_NOT_BOUND;
  return ObjectInfo<ObjectType>::acquireMessage(sender->channel, pStagedMessage, timeout);
}

template <>
void      ObjectInfo<LocalMpMcChannelSender>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  auto sender = static_cast<HandleType*>(object);
  ObjectInfo<ObjectType>::pushQueue(sender->channel, message, flags);
}

template <>
AgtStatus ObjectInfo<LocalMpMcChannelSender>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
  // auto sender = static_cast<HandleType*>(object);
  // if (!sender->channel)
  //   return AGT_ERROR_NOT_BOUND;
  // return ObjectInfo<ObjectType>::popQueue(sender->channel, pMessageInfo, timeout);
}

template <>
void      ObjectInfo<LocalMpMcChannelSender>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  // ObjectInfo<HandleType>::releaseMessage(static_cast<HandleType*>(object)->channel, message);
}

template <>
AgtStatus ObjectInfo<LocalMpMcChannelSender>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {
  auto sender = static_cast<HandleType*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
AgtStatus ObjectInfo<LocalMpMcChannelSender>::acquireRef(HandleHeader* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      ObjectInfo<LocalMpMcChannelSender>::releaseRef(HandleHeader* object) noexcept { }


AgtStatus Agt::createInstance(LocalMpMcChannelSender*& objectRef, AgtContext ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/** =================================[ LocalMpMcChannelReceiver ]================================================= */

template <>
AgtStatus ObjectInfo<LocalMpMcChannelReceiver>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
}

template <>
void      ObjectInfo<LocalMpMcChannelReceiver>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept {
  // auto sender = static_cast<HandleType*>(object);
  // ObjectInfo<PrivateChannel>::pushQueue(sender->channel, message, flags);
}

template <>
AgtStatus ObjectInfo<LocalMpMcChannelReceiver>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
  auto receiver = static_cast<HandleType*>(object);
  if (!receiver->channel)
    return AGT_ERROR_NOT_BOUND;
  return ObjectInfo<ObjectType>::popQueue(receiver->channel, pMessageInfo, timeout);
}

template <>
void      ObjectInfo<LocalMpMcChannelReceiver>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept {
  ObjectInfo<ObjectType>::releaseMessage(static_cast<HandleType*>(object)->channel, message);
}

template <>
AgtStatus ObjectInfo<LocalMpMcChannelReceiver>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept {
  auto receiver = static_cast<HandleType*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
AgtStatus ObjectInfo<LocalMpMcChannelReceiver>::acquireRef(HandleHeader* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      ObjectInfo<LocalMpMcChannelReceiver>::releaseRef(HandleHeader* object) noexcept { }

AgtStatus Agt::createInstance(LocalMpMcChannelReceiver*& objectRef, AgtContext ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}




/** =================================[ SharedSpScChannelHandle ]================================================== */
/** =================================[ SharedSpScChannelSender ]================================================== */
/** =================================[ SharedSpScChannelReceiver ]================================================ */
/** =================================[ SharedSpMcChannelHandle ]================================================== */
/** =================================[ SharedSpMcChannelSender ]================================================== */
/** =================================[ SharedSpMcChannelReceiver ]================================================ */
/** =================================[ SharedMpScChannelHandle ]================================================== */
/** =================================[ SharedMpScChannelSender ]================================================== */
/** =================================[ SharedMpScChannelReceiver ]================================================ */
/** =================================[ SharedMpMcChannelHandle ]================================================== */
/** =================================[ SharedMpMcChannelSender ]================================================== */
/** =================================[ SharedMpMcChannelReceiver ]================================================ */



