//
// Created by maxwe on 2021-11-03.
//

// #include "internal.hpp"
#include "async/async.hpp"
#include "messages/message.hpp"
#include "support/align.hpp"
#include "channel.hpp"

#include <cstring>

using namespace agt;


/*namespace Agt {

  struct AGT_cache_aligned PrivateChannelMessage {
    PrivateChannelMessage* next;
    private_channel*        owner;
    agt_handle_t              returnHandle;
    agt_async_data_t           asyncData;
    agt_u32_t              asyncDataKey;
    message_flags           flags;
    message_state           state;
    agt_u32_t              refCount;
    agt_message_id_t           id;
    size_t                payloadSize;
    InlineBuffer           inlineBuffer[];
  };

  struct AGT_cache_aligned LocalChannelMessage {
    LocalChannelMessage*      next;
    local_channel*             owner;
    agt_handle_t                 returnHandle;
    agt_async_data_t              asyncData;
    agt_u32_t                 asyncDataKey;
    message_flags              flags;
    atomic_flags<message_state> state;
    ReferenceCount            refCount;
    agt_message_id_t              id;
    size_t                   payloadSize;
    InlineBuffer              inlineBuffer[];
  };

  struct AGT_cache_aligned SharedChannelMessage {
    size_t                   nextIndex;
    size_t                   thisIndex;
    agt_object_id_t               returnHandleId;
    size_t                   asyncDataOffset;
    agt_u32_t                 asyncDataKey;
    message_flags              flags;
    atomic_flags<message_state> state;
    ReferenceCount            refCount;
    agt_message_id_t              id;
    size_t                   payloadSize;
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


  inline atomic_flags<message_state>& state(agt_message_t message) noexcept {
    return reinterpret_cast<atomic_flags<message_state>&>(message->state);
  }


  /*
  inline void* getPayload(void* message_) noexcept {
    auto message = static_cast<PrivateChannelMessage*>(message_);
    if ((message->flags & message_flags::isOutOfLine) == flags_not_set) [[likely]]
      return message->inlineBuffer;
    else {
      if ((message->flags & message_flags::isMultiFrame) == flags_not_set) {
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
      agt_multi_frame_message_info_t mfi;
      auto status = getMultiFrameMessage(message->inlineBuffer, mfi);
      if (status == AGT_SUCCESS) {
        agt_message_frame_t frame;
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
    if ((message->flags & message_flags::isOutOfLine) == flags_not_set) [[likely]]
      return message->inlineBuffer;
    else {
      if ((message->flags & message_flags::isMultiFrame) == flags_not_set) {
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
      agt_multi_frame_message_info_t mfi;
      auto status = getMultiFrameMessage(message->inlineBuffer, mfi);
      if (status == AGT_SUCCESS) {
        agt_message_frame_t frame;
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

  void zeroMessageData(agt_message_t message) noexcept {
    std::memset(message->inlineBuffer, 0, message->payloadSize);
  }

  AGT_noinline void doSlowCleanup(agt_message_t message) noexcept {
    zeroMessageData(message);
    message->returnHandle = nullptr;
    message->id           = 0;
    message->payloadSize  = 0;
  }


  inline void privateCleanupMessage(agt_ctx_t ctx, agt_message_t message) noexcept {
    message->state    = default_message_state;
    message->refCount = 1;
    message->flags    = flags_not_set;
    if (message->asyncData != nullptr)
      asyncDataDrop(message->asyncData, ctx, message->asyncDataKey);
    if ((message->flags & message_flags::shouldDoFastCleanup) == flags_not_set)
      doSlowCleanup(message);
    //TODO: Release message resources if message is not inline
  }
  inline void cleanupMessage(agt_ctx_t ctx, agt_message_t message) noexcept {
    message->state    = default_message_state;
    impl::atomicStore(message->refCount, 1);
    message->flags    = flags_not_set;
    if (message->asyncData != nullptr)
      asyncDataDrop(message->asyncData, ctx, message->asyncDataKey);
    if ((message->flags & message_flags::shouldDoFastCleanup) == flags_not_set)
      doSlowCleanup(message);
    //TODO: Release message resources if message is not inline
  }

  inline void privateDestroyMessage(private_channel* channel, agt_message_t message) noexcept {
    privateCleanupMessage(channel->context, message);
    object_info<private_channel>::releaseMessage(channel, message);
  }
  template <typename Channel>
  inline void destroyMessage(Channel* channel, agt_message_t message) noexcept {
    cleanupMessage(channel->context, message);
    object_info<Channel>::releaseMessage(channel, message);
  }

  inline void privatePlaceHold(agt_message_t message) noexcept {
    message->state = message->state | message_state::isOnHold;
  }
  inline void privateReleaseHold(private_channel* channel, agt_message_t message) noexcept {
    message_state oldState = message->state;
    message->state = message->state & ~message_state::isOnHold;
    if ((oldState & message_state::isCondemned) != flags_not_set)
      destroyMessage(channel, message);
  }

  inline void placeHold(agt_message_t message) noexcept {
    state(message).set(message_state::isOnHold);
  }
  template <typename Channel>
  inline void releaseHold(Channel* channel, agt_message_t message) noexcept {
    if ( (state(message).fetchAndReset(message_state::isOnHold) & message_state::isCondemned) != flags_not_set )
      destroyMessage(channel, message);
  }

  inline void condemn(private_channel* channel, agt_message_t message) noexcept {
    message_state oldState = message->state;
    message->state = message->state | message_state::isCondemned;
    if ((oldState & message_state::isOnHold) == flags_not_set)
      destroyMessage(channel, message);
  }
  template <typename Channel>
  inline void condemn(Channel* channel, agt_message_t message) noexcept {
    if ( (state(message).fetchAndSet(message_state::isCondemned) & message_state::isOnHold) == flags_not_set )
      destroyMessage(channel, message);
  }


  inline void destroy(private_channel* channel) noexcept {

  }


  inline size_t getDefaultPrivateChannelMessageSize(agt_ctx_t ctx) noexcept {

    // Default to be used if default lookup fails
    // Each slot is 256 bytes, or 4 cachelines long.
    constexpr static size_t SuperDefault = AGT_CACHE_LINE * 3;


    size_t messageSize;
    size_t dataSize = sizeof(size_t);

    if (!ctxGetBuiltinValue(ctx, builtin_value::defaultPrivateChannelMessageSize, &messageSize, dataSize)) [[unlikely]]
      return SuperDefault; // If this path is taken, there's likely something very wrong

    return messageSize;
  }
  inline size_t getDefaultPrivateChannelSlotCount(agt_ctx_t ctx) noexcept {
    // Default to be used if default lookup fails
    // Using both SuperDefaults, each channel would be 64 KiB in size
    constexpr static size_t SuperDefault = 256;


    size_t slotCount;
    size_t dataSize = sizeof(size_t);

    if (!ctxGetBuiltinValue(ctx, builtin_value::defaultPrivateChannelSlotCount, &slotCount, dataSize)) [[unlikely]]
      return SuperDefault; // If this path is taken, there's likely something very wrong

    return slotCount;
  }
  inline size_t getDefaultLocalChannelMessageSize(agt_ctx_t ctx) noexcept {

    // Default to be used if default lookup fails
    // Each slot is 256 bytes, or 4 cachelines long.
    constexpr static size_t SuperDefault = AGT_CACHE_LINE * 3;


    size_t messageSize;
    size_t dataSize = sizeof(size_t);

    if (!ctxGetBuiltinValue(ctx, builtin_value::defaultLocalChannelMessageSize, &messageSize, dataSize)) [[unlikely]]
      return SuperDefault; // If this path is taken, there's likely something very wrong

    return messageSize;
  }
  inline size_t getDefaultLocalChannelSlotCount(agt_ctx_t ctx) noexcept {
    // Default to be used if default lookup fails
    // Using both SuperDefaults, each channel would be 64 KiB in size
    constexpr static size_t SuperDefault = 256;


    size_t slotCount;
    size_t dataSize = sizeof(size_t);

    if (!ctxGetBuiltinValue(ctx, builtin_value::defaultLocalChannelSlotCount, &slotCount, dataSize)) [[unlikely]]
      return SuperDefault; // If this path is taken, there's likely something very wrong

    return slotCount;
  }
  inline size_t getDefaultSharedChannelMessageSize(agt_ctx_t ctx) noexcept {

    // Default to be used if default lookup fails
    // Each slot is 1024 bytes, or 16 cachelines long.
    constexpr static size_t SuperDefault = AGT_CACHE_LINE * 15;

    size_t messageSize;
    size_t dataSize = sizeof(size_t);

    if (!ctxGetBuiltinValue(ctx, builtin_value::defaultSharedChannelMessageSize, &messageSize, dataSize)) [[unlikely]]
      return SuperDefault; // If this path is taken, there's likely something very wrong

    return messageSize;
  }
  inline size_t getDefaultSharedChannelSlotCount(agt_ctx_t ctx) noexcept {
    // Default to be used if default lookup fails
    // Using both SuperDefaults, each channel would be 64 KiB in size
    constexpr static size_t SuperDefault = 256;


    size_t slotCount;
    size_t dataSize = sizeof(size_t);

    if (!ctxGetBuiltinValue(ctx, builtin_value::defaultSharedChannelSlotCount, &slotCount, dataSize)) [[unlikely]]
      return SuperDefault; // If this path is taken, there's likely something very wrong

    return slotCount;
  }


}










/** =================================[ private_channel ]===================================================== */

/*private_channel::private_channel(agt_status_t& status, agt_ctx_t ctx, agt_object_id_t id, size_t slotCount, size_t slotSize) noexcept
    : local_channel(status, ObjectType::privateChannel, ctx, id, slotCount, slotSize){
  
}*/

template <>
agt_status_t object_info<private_channel>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  auto channel = static_cast<private_channel*>(object);
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
void      object_info<private_channel>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  auto channel = static_cast<private_channel*>(object);
  privatePlaceHold(message);
  channel->prevQueuedMessage->next = message;
  channel->prevQueuedMessage = message;
  ++channel->queuedMessageCount;
}
template <>
agt_status_t object_info<private_channel>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  auto channel = static_cast<private_channel*>(object);
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
void      object_info<private_channel>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  auto channel = static_cast<private_channel*>(object);
  ++channel->availableSlotCount;
  message->next = channel->nextFreeSlot;
  channel->nextFreeSlot = message;
}

template <>
agt_status_t object_info<private_channel>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
agt_status_t object_info<private_channel>::acquireRef(handle_header* object) noexcept {
  ++static_cast<private_channel*>(object)->refCount;
}
template <>
void      object_info<private_channel>::releaseRef(handle_header* object) noexcept {
  auto channel = static_cast<private_channel*>(object);
  if (!--channel->refCount)
    destroy(channel);
}


agt_status_t Agt::createInstance(private_channel*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, private_channel_sender* sender, private_channel_receiver* receiver) noexcept {

  const size_t messageSize = createInfo.maxMessageSize
                                ? alignSize(createInfo.maxMessageSize, AGT_CACHE_LINE)
                                : getDefaultPrivateChannelMessageSize(ctx);
  const size_t slotCount   = createInfo.minCapacity
                                ? createInfo.minCapacity
                                : getDefaultPrivateChannelSlotCount(ctx);

  auto channel = (private_channel*)ctxAllocHandle(ctx, sizeof(private_channel), AGT_CACHE_LINE);

  if (!channel) [[unlikely]] {
    outHandle = nullptr;
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->type = ObjectType::privateChannel;
  channel->flags = flags_not_set;
  channel->context = ctx;

  channel->slotCount = slotCount;
  channel->inlineBufferSize = messageSize;

  if (!initMessageArray(channel)) {
    ctxFreeHandle(ctx, channel, sizeof(private_channel), alignof(private_channel));
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->consumer            = receiver;
  channel->producer            = sender;
  channel->availableSlotCount  = slotCount - 1;
  channel->queuedMessageCount  = 0;
  channel->nextFreeSlot        = (agt_message_t)channel->messageSlots;
  channel->prevReceivedMessage = (agt_message_t)(channel->messageSlots + ((slotCount - 1) * (messageSize + AGT_CACHE_LINE)));
  channel->prevQueuedMessage   = channel->prevReceivedMessage;
  channel->refCount            = static_cast<agt_u32_t>(static_cast<bool>(receiver)) + static_cast<bool>(sender);

  if (createInfo.name) {
    if (agt_status_t status = ctxRegisterNamedObject(ctx, channel, createInfo.name)) {
      ctxLocalFree(ctx, channel->messageSlots, slotCount * (messageSize + AGT_CACHE_LINE), AGT_CACHE_LINE);
      ctxFreeHandle(ctx, channel, sizeof(private_channel), alignof(private_channel));
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


/** =================================[ local_spsc_channel ]===================================================== */


template <>
agt_status_t object_info<local_spsc_channel>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  auto channel = static_cast<local_spsc_channel*>(object);
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
void      object_info<local_spsc_channel>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  auto channel = static_cast<local_spsc_channel*>(object);
  placeHold(message);
  channel->producerPreviousQueuedMsg->next = message;
  channel->producerPreviousQueuedMsg = message;
  channel->queuedMessages.release();
}
template <>
agt_status_t object_info<local_spsc_channel>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  auto channel = static_cast<local_spsc_channel*>(object);
  AGT_acquire_semaphore(channel->queuedMessages, timeout, AGT_ERROR_MAILBOX_IS_EMPTY);
  auto previousMsg = channel->consumerPreviousMsg;
  auto message = previousMsg->next;
  channel->consumerPreviousMsg = message;
  releaseHold(channel, previousMsg);
  pMessageInfo->message = (agt_message_t)message;
  pMessageInfo->size    = message->payloadSize;
  pMessageInfo->id      = message->id;
  pMessageInfo->payload = message->inlineBuffer;
  return AGT_SUCCESS;
}
template <>
void      object_info<local_spsc_channel>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  auto channel = static_cast<local_spsc_channel*>(object);
  auto lastFreeSlot = channel->lastFreeSlot;
  channel->lastFreeSlot = message;
  lastFreeSlot->next = message;
  channel->slotSemaphore.release();
}

template <>
agt_status_t object_info<local_spsc_channel>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {}

template <>
agt_status_t object_info<local_spsc_channel>::acquireRef(handle_header* object) noexcept {}
template <>
void      object_info<local_spsc_channel>::releaseRef(handle_header* object) noexcept {}


agt_status_t Agt::createInstance(local_spsc_channel*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, local_spsc_channel_sender* sender, local_spsc_channel_receiver* receiver) noexcept {
  const size_t messageSize = createInfo.maxMessageSize
                                ? alignSize(createInfo.maxMessageSize, AGT_CACHE_LINE)
                                : getDefaultLocalChannelMessageSize(ctx);
  const size_t slotCount   = createInfo.minCapacity
                              ? createInfo.minCapacity
                              : getDefaultLocalChannelSlotCount(ctx);
  
  auto channel = (local_spsc_channel*)ctxAllocHandle(ctx, sizeof(local_spsc_channel), AGT_CACHE_LINE);

  if (!channel) [[unlikely]] {
    outHandle = nullptr;
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->type = ObjectType::spscChannel;
  channel->flags = flags_not_set;
  channel->context = ctx;

  channel->slotCount = slotCount;
  channel->inlineBufferSize = messageSize;

  if (!initMessageArray(channel)) {
    ctxFreeHandle(ctx, channel, sizeof(local_spsc_channel), alignof(local_spsc_channel));
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->consumer            = receiver;
  channel->producer            = sender;
  new (&channel->slotSemaphore) semaphore((ptrdiff_t)(slotCount - 1));
  new (&channel->queuedMessages) semaphore(0);
  channel->producerNextFreeSlot      = (agt_message_t)channel->messageSlots;
  channel->consumerPreviousMsg       = (agt_message_t)(channel->messageSlots + ((slotCount - 1) * (messageSize + AGT_CACHE_LINE)));
  channel->producerPreviousQueuedMsg = channel->consumerPreviousMsg;
  new (&channel->refCount) ref_count(static_cast<agt_u32_t>(static_cast<bool>(receiver)) + static_cast<bool>(sender));

  if (createInfo.name) {
    if (agt_status_t status = ctxRegisterNamedObject(ctx, channel, createInfo.name)) {
      ctxLocalFree(ctx, channel->messageSlots, slotCount * (messageSize + AGT_CACHE_LINE), AGT_CACHE_LINE);
      ctxFreeHandle(ctx, channel, sizeof(private_channel), alignof(private_channel));
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


/** =================================[ local_spmc_channel ]===================================================== */

template <>
agt_status_t object_info<local_spmc_channel>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  auto channel = static_cast<local_spmc_channel*>(object);
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
void      object_info<local_spmc_channel>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  auto channel = static_cast<local_spmc_channel*>(object);
  placeHold(message);
  channel->lastQueuedSlot->next = message;
  channel->lastQueuedSlot = message;
  channel->queuedMessages.release();
}
template <>
agt_status_t object_info<local_spmc_channel>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  auto channel = static_cast<local_spmc_channel*>(object);
  AGT_acquire_semaphore(channel->queuedMessages, timeout, AGT_ERROR_MAILBOX_IS_EMPTY);

  auto prevReceivedMsg = channel->previousReceivedMessage.load(std::memory_order_acquire);
  agt_message_t message;
  do {
    message = prevReceivedMsg->next;
  } while ( !channel->previousReceivedMessage.compare_exchange_weak(prevReceivedMsg, message) );
  releaseHold(channel, prevReceivedMsg);

  pMessageInfo->message = (agt_message_t)message;
  pMessageInfo->size    = message->payloadSize;
  pMessageInfo->id      = message->id;
  pMessageInfo->payload = message->inlineBuffer;

  return AGT_SUCCESS;
}
template <>
void      object_info<local_spmc_channel>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  auto channel = static_cast<local_spmc_channel*>(object);
  auto lastFreeSlot = channel->lastFreeSlot.exchange(message);
  lastFreeSlot->next = message;
  channel->slotSemaphore.release();
}

template <>
agt_status_t object_info<local_spmc_channel>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {}

template <>
agt_status_t object_info<local_spmc_channel>::acquireRef(handle_header* object) noexcept {}
template <>
void      object_info<local_spmc_channel>::releaseRef(handle_header* object) noexcept {}


agt_status_t Agt::createInstance(local_spmc_channel*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, local_spmc_channel_sender* sender, local_spmc_channel_receiver* receiver) noexcept {
  const size_t messageSize = createInfo.maxMessageSize
                                ? alignSize(createInfo.maxMessageSize, AGT_CACHE_LINE)
                                : getDefaultLocalChannelMessageSize(ctx);
  const size_t slotCount   = createInfo.minCapacity
                              ? createInfo.minCapacity
                              : getDefaultLocalChannelSlotCount(ctx);
  
  auto channel = (local_spmc_channel*)ctxAllocHandle(ctx, sizeof(local_spmc_channel), AGT_CACHE_LINE);

  if (!channel) [[unlikely]] {
    outHandle = nullptr;
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->type = ObjectType::spmcChannel;
  channel->flags = flags_not_set;
  channel->context = ctx;

  channel->slotCount = slotCount;
  channel->inlineBufferSize = messageSize;

  if (!initMessageArray(channel)) {
    ctxFreeHandle(ctx, channel, sizeof(local_spmc_channel), alignof(local_spmc_channel));
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->maxConsumers        = createInfo.maxReceivers;
  new (&channel->consumerSemaphore) semaphore(createInfo.maxReceivers);
  channel->producer            = sender;
  new (&channel->slotSemaphore) semaphore((ptrdiff_t)(slotCount - 1));
  new (&channel->queuedMessages) semaphore(0);
  channel->nextFreeSlot = (agt_message_t)channel->messageSlots;
  channel->lastQueuedSlot = (agt_message_t)(channel->messageSlots + ((slotCount - 1) * (messageSize + AGT_CACHE_LINE)));
  new (&channel->previousReceivedMessage) std::atomic<agt_message_t>(channel->lastQueuedSlot);
  new (&channel->refCount) ref_count(static_cast<agt_u32_t>(static_cast<bool>(receiver)) + static_cast<bool>(sender));

  if (createInfo.name) {
    if (agt_status_t status = ctxRegisterNamedObject(ctx, channel, createInfo.name)) {
      ctxLocalFree(ctx, channel->messageSlots, slotCount * (messageSize + AGT_CACHE_LINE), AGT_CACHE_LINE);
      ctxFreeHandle(ctx, channel, sizeof(private_channel), alignof(private_channel));
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


/** =================================[ local_mpmc_channel ]===================================================== */

template <>
agt_status_t object_info<local_mpmc_channel>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  auto channel = static_cast<handle_type*>(object);
  auto& stagedMessage = reinterpret_cast<StagedMessage&>(*pStagedMessage);
  agt_message_t message;
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
void      object_info<local_mpmc_channel>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  auto channel = static_cast<handle_type*>(object);
  placeHold(message);
  agt_message_t lastQueuedSlot = channel->lastQueuedSlot.load(std::memory_order_acquire);
  do {
    lastQueuedSlot->next = message;
  } while ( !channel->lastQueuedSlot.compare_exchange_weak(lastQueuedSlot, message) );
  channel->queuedMessages.release();
}
template <>
agt_status_t object_info<local_mpmc_channel>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  auto channel = static_cast<handle_type*>(object);
  AGT_acquire_semaphore(channel->queuedMessages, timeout, AGT_ERROR_MAILBOX_IS_EMPTY);

  auto prevReceivedMsg = channel->previousReceivedMessage.load(std::memory_order_acquire);
  agt_message_t message;
  do {
    message = prevReceivedMsg->next;
  } while ( !channel->previousReceivedMessage.compare_exchange_weak(prevReceivedMsg, message) );
  releaseHold(channel, prevReceivedMsg);

  pMessageInfo->message = (agt_message_t)message;
  pMessageInfo->size    = message->payloadSize;
  pMessageInfo->id      = message->id;
  pMessageInfo->payload = message->inlineBuffer;

  return AGT_SUCCESS;
}
template <>
void      object_info<local_mpmc_channel>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  auto channel = static_cast<handle_type*>(object);
  auto nextFreeSlot = channel->nextFreeSlot.load();
  do {
    message->next = nextFreeSlot;
  } while( !channel->nextFreeSlot.compare_exchange_weak(nextFreeSlot, message) );
  nextFreeSlot->next = message;
  channel->slotSemaphore.release();
}

template <>
agt_status_t object_info<local_mpmc_channel>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {}

template <>
agt_status_t object_info<local_mpmc_channel>::acquireRef(handle_header* object) noexcept {}
template <>
void      object_info<local_mpmc_channel>::releaseRef(handle_header* object) noexcept {}

agt_status_t Agt::createInstance(local_mpmc_channel*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, local_mpmc_channel_sender* sender, local_mpmc_channel_receiver* receiver) noexcept {
  const size_t messageSize = createInfo.maxMessageSize
                                ? alignSize(createInfo.maxMessageSize, AGT_CACHE_LINE)
                                : getDefaultLocalChannelMessageSize(ctx);
  const size_t slotCount   = createInfo.minCapacity
                              ? createInfo.minCapacity
                              : getDefaultLocalChannelSlotCount(ctx);

  auto channel = (local_mpmc_channel*)ctxAllocHandle(ctx, sizeof(local_mpmc_channel), AGT_CACHE_LINE);

  if (!channel) [[unlikely]] {
    outHandle = nullptr;
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->type = ObjectType::mpscChannel;
  channel->flags = flags_not_set;
  channel->context = ctx;

  channel->slotCount = slotCount;
  channel->inlineBufferSize = messageSize;

  if (!initMessageArray(channel)) {
    ctxFreeHandle(ctx, channel, sizeof(local_mpmc_channel), alignof(local_mpmc_channel));
    return AGT_ERROR_BAD_ALLOC;
  }

  agt_message_t lastMessageInArray = (agt_message_t)(channel->messageSlots + ((slotCount - 1) * (messageSize + AGT_CACHE_LINE)));

  channel->maxConsumers = createInfo.maxReceivers;
  new (&channel->consumerSemaphore) semaphore((ptrdiff_t)createInfo.maxReceivers);
  channel->maxProducers = createInfo.maxSenders;
  new (&channel->producerSemaphore) semaphore((ptrdiff_t)createInfo.maxSenders);
  new (&channel->slotSemaphore) semaphore((ptrdiff_t)(slotCount - 1));
  new (&channel->queuedMessages) semaphore(0);
  new (&channel->nextFreeSlot) std::atomic<agt_message_t>((agt_message_t)channel->messageSlots);
  new (&channel->previousReceivedMessage) std::atomic<agt_message_t>(lastMessageInArray);
  new (&channel->lastQueuedSlot) std::atomic<agt_message_t>(lastMessageInArray);
  new (&channel->refCount) ref_count(static_cast<agt_u32_t>(static_cast<bool>(receiver)) + static_cast<bool>(sender));

  if (createInfo.name) {
    if (agt_status_t status = ctxRegisterNamedObject(ctx, channel, createInfo.name)) {
      ctxLocalFree(ctx, channel->messageSlots, slotCount * (messageSize + AGT_CACHE_LINE), AGT_CACHE_LINE);
      ctxFreeHandle(ctx, channel, sizeof(private_channel), alignof(private_channel));
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

/** =================================[ local_mpsc_channel ]===================================================== */

template <>
agt_status_t object_info<local_mpsc_channel>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  auto channel = static_cast<local_mpsc_channel*>(object);
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
void      object_info<local_mpsc_channel>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  auto channel = static_cast<local_mpsc_channel*>(object);
  auto lastQueuedMessage = channel->lastQueuedSlot.load(std::memory_order_acquire);
  placeHold(message);
  do {
    lastQueuedMessage->next = message;
  } while ( !channel->lastQueuedSlot.compare_exchange_weak(lastQueuedMessage, message) );
  channel->queuedMessageCount.increase(1);
}
template <>
agt_status_t object_info<local_mpsc_channel>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  auto channel = static_cast<local_mpsc_channel*>(object);
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
  pMessageInfo->message = (agt_message_t)message;
  pMessageInfo->size    = message->payloadSize;
  pMessageInfo->id      = message->id;
  pMessageInfo->payload = message->inlineBuffer;
  return AGT_SUCCESS;
}
template <>
void      object_info<local_mpsc_channel>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  auto channel = static_cast<local_mpsc_channel*>(object);
  auto lastFreeSlot = channel->lastFreeSlot.load();
  do {
    lastFreeSlot->next = message;
  } while( !channel->lastFreeSlot.compare_exchange_weak(lastFreeSlot, message) );
  // FIXME: I think this is still a potential bug, though it might be one thats hard to detect....
  lastFreeSlot->next = message;
}

template <>
agt_status_t object_info<local_mpsc_channel>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {}

template <>
agt_status_t object_info<local_mpsc_channel>::acquireRef(handle_header* object) noexcept {}
template <>
void      object_info<local_mpsc_channel>::releaseRef(handle_header* object) noexcept {}


agt_status_t Agt::createInstance(local_mpsc_channel*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, local_mpsc_channel_sender* sender, local_mpsc_channel_receiver* receiver) noexcept {
  const size_t messageSize = createInfo.maxMessageSize
                                ? alignSize(createInfo.maxMessageSize, AGT_CACHE_LINE)
                                : getDefaultLocalChannelMessageSize(ctx);
  const size_t slotCount   = createInfo.minCapacity
                              ? createInfo.minCapacity
                              : getDefaultLocalChannelSlotCount(ctx);
  
  auto channel = (local_mpsc_channel*)ctxAllocHandle(ctx, sizeof(local_mpsc_channel), AGT_CACHE_LINE);

  if (!channel) [[unlikely]] {
    outHandle = nullptr;
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->type = ObjectType::mpscChannel;
  channel->flags = flags_not_set;
  channel->context = ctx;

  channel->slotCount = slotCount;
  channel->inlineBufferSize = messageSize;

  if (!initMessageArray(channel)) {
    ctxFreeHandle(ctx, channel, sizeof(local_mpsc_channel), alignof(local_mpsc_channel));
    return AGT_ERROR_BAD_ALLOC;
  }

  channel->consumer            = receiver;
  channel->maxProducers = createInfo.maxSenders;
  new (&channel->producerSemaphore) semaphore((ptrdiff_t)createInfo.maxSenders);
  new (&channel->slotSemaphore) semaphore((ptrdiff_t)(slotCount - 1));
  new (&channel->queuedMessageCount) semaphore(0);
  new (&channel->nextFreeSlot) std::atomic<agt_message_t>((agt_message_t)channel->messageSlots);
  channel->previousReceivedMessage = (agt_message_t)(channel->messageSlots + ((slotCount - 1) * (messageSize + AGT_CACHE_LINE)));
  new (&channel->lastQueuedSlot) std::atomic<agt_message_t>(channel->previousReceivedMessage);
  new (&channel->refCount) ref_count(static_cast<agt_u32_t>(static_cast<bool>(receiver)) + static_cast<bool>(sender));

  if (createInfo.name) {
    if (agt_status_t status = ctxRegisterNamedObject(ctx, channel, createInfo.name)) {
      ctxLocalFree(ctx, channel->messageSlots, slotCount * (messageSize + AGT_CACHE_LINE), AGT_CACHE_LINE);
      ctxFreeHandle(ctx, channel, sizeof(private_channel), alignof(private_channel));
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







/** =================================[ private_channel_sender ]===================================================== */

template <>
agt_status_t object_info<private_channel_sender>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  auto sender = static_cast<handle_type*>(object);
  if (!sender->channel)
    return AGT_ERROR_NOT_BOUND;
  return object_info<ObjectType>::acquireMessage(sender->channel, pStagedMessage, timeout);
}

template <>
void      object_info<private_channel_sender>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  auto sender = static_cast<handle_type*>(object);
  object_info<ObjectType>::pushQueue(sender->channel, message, flags);
}

template <>
agt_status_t object_info<private_channel_sender>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
  // auto sender = static_cast<handle_type*>(object);
  // if (!sender->channel)
  //   return AGT_ERROR_NOT_BOUND;
  // return object_info<ObjectType>::popQueue(sender->channel, pMessageInfo, timeout);
}

template <>
void      object_info<private_channel_sender>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  // object_info<handle_type>::releaseMessage(static_cast<handle_type*>(object)->channel, message);
}

template <>
agt_status_t object_info<private_channel_sender>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {
  auto sender = static_cast<handle_type*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
agt_status_t object_info<private_channel_sender>::acquireRef(handle_header* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      object_info<private_channel_sender>::releaseRef(handle_header* object) noexcept { }

/** =================================[ private_channel_receiver ]=================================================== */

template <>
agt_status_t object_info<private_channel_receiver>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
}

template <>
void      object_info<private_channel_receiver>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  // auto sender = static_cast<handle_type*>(object);
  // object_info<private_channel>::pushQueue(sender->channel, message, flags);
}

template <>
agt_status_t object_info<private_channel_receiver>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  auto receiver = static_cast<handle_type*>(object);
  if (!receiver->channel)
    return AGT_ERROR_NOT_BOUND;
  return object_info<ObjectType>::popQueue(receiver->channel, pMessageInfo, timeout);
}

template <>
void      object_info<private_channel_receiver>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  object_info<ObjectType>::releaseMessage(static_cast<handle_type*>(object)->channel, message);
}

template <>
agt_status_t object_info<private_channel_receiver>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {
  auto receiver = static_cast<handle_type*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
agt_status_t object_info<private_channel_receiver>::acquireRef(handle_header* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      object_info<private_channel_receiver>::releaseRef(handle_header* object) noexcept { }

/** =================================[ local_spsc_channel_sender ]=================================================== */

template <>
agt_status_t object_info<local_spsc_channel_sender>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  auto sender = static_cast<handle_type*>(object);
  if (!sender->channel)
    return AGT_ERROR_NOT_BOUND;
  return object_info<ObjectType>::acquireMessage(sender->channel, pStagedMessage, timeout);
}

template <>
void      object_info<local_spsc_channel_sender>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  auto sender = static_cast<handle_type*>(object);
  object_info<ObjectType>::pushQueue(sender->channel, message, flags);
}

template <>
agt_status_t object_info<local_spsc_channel_sender>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
  // auto sender = static_cast<handle_type*>(object);
  // if (!sender->channel)
  //   return AGT_ERROR_NOT_BOUND;
  // return object_info<ObjectType>::popQueue(sender->channel, pMessageInfo, timeout);
}

template <>
void      object_info<local_spsc_channel_sender>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  // object_info<ObjectType>::releaseMessage(static_cast<handle_type*>(object)->channel, message);
}

template <>
agt_status_t object_info<local_spsc_channel_sender>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {
  auto sender = static_cast<handle_type*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
agt_status_t object_info<local_spsc_channel_sender>::acquireRef(handle_header* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      object_info<local_spsc_channel_sender>::releaseRef(handle_header* object) noexcept { }


agt_status_t Agt::createInstance(local_spsc_channel_sender*& objectRef, agt_ctx_t ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/** =================================[ local_spsc_channel_receiver ]================================================= */

template <>
agt_status_t object_info<local_spsc_channel_receiver>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
}

template <>
void      object_info<local_spsc_channel_receiver>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  // auto sender = static_cast<handle_type*>(object);
  // object_info<private_channel>::pushQueue(sender->channel, message, flags);
}

template <>
agt_status_t object_info<local_spsc_channel_receiver>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  auto receiver = static_cast<handle_type*>(object);
  if (!receiver->channel)
    return AGT_ERROR_NOT_BOUND;
  return object_info<ObjectType>::popQueue(receiver->channel, pMessageInfo, timeout);
}

template <>
void      object_info<local_spsc_channel_receiver>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  object_info<ObjectType>::releaseMessage(static_cast<handle_type*>(object)->channel, message);
}

template <>
agt_status_t object_info<local_spsc_channel_receiver>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {
  auto receiver = static_cast<handle_type*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
agt_status_t object_info<local_spsc_channel_receiver>::acquireRef(handle_header* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      object_info<local_spsc_channel_receiver>::releaseRef(handle_header* object) noexcept { }

agt_status_t Agt::createInstance(local_spsc_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/** =================================[ local_spmc_channel_sender ]=================================================== */

template <>
agt_status_t object_info<local_spmc_channel_sender>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  auto sender = static_cast<handle_type*>(object);
  if (!sender->channel)
    return AGT_ERROR_NOT_BOUND;
  return object_info<ObjectType>::acquireMessage(sender->channel, pStagedMessage, timeout);
}

template <>
void      object_info<local_spmc_channel_sender>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  auto sender = static_cast<handle_type*>(object);
  object_info<ObjectType>::pushQueue(sender->channel, message, flags);
}

template <>
agt_status_t object_info<local_spmc_channel_sender>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
  // auto sender = static_cast<handle_type*>(object);
  // if (!sender->channel)
  //   return AGT_ERROR_NOT_BOUND;
  // return object_info<ObjectType>::popQueue(sender->channel, pMessageInfo, timeout);
}

template <>
void      object_info<local_spmc_channel_sender>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  // object_info<handle_type>::releaseMessage(static_cast<handle_type*>(object)->channel, message);
}

template <>
agt_status_t object_info<local_spmc_channel_sender>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {
  auto sender = static_cast<handle_type*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
agt_status_t object_info<local_spmc_channel_sender>::acquireRef(handle_header* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      object_info<local_spmc_channel_sender>::releaseRef(handle_header* object) noexcept { }

/** =================================[ local_spmc_channel_receiver ]================================================= */

template <>
agt_status_t object_info<local_spmc_channel_receiver>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
}

template <>
void      object_info<local_spmc_channel_receiver>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  // auto sender = static_cast<handle_type*>(object);
  // object_info<private_channel>::pushQueue(sender->channel, message, flags);
}

template <>
agt_status_t object_info<local_spmc_channel_receiver>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  auto receiver = static_cast<handle_type*>(object);
  if (!receiver->channel)
    return AGT_ERROR_NOT_BOUND;
  return object_info<ObjectType>::popQueue(receiver->channel, pMessageInfo, timeout);
}

template <>
void      object_info<local_spmc_channel_receiver>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  object_info<ObjectType>::releaseMessage(static_cast<handle_type*>(object)->channel, message);
}

template <>
agt_status_t object_info<local_spmc_channel_receiver>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {
  auto receiver = static_cast<handle_type*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
agt_status_t object_info<local_spmc_channel_receiver>::acquireRef(handle_header* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      object_info<local_spmc_channel_receiver>::releaseRef(handle_header* object) noexcept { }

agt_status_t Agt::createInstance(local_spmc_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/** =================================[ local_mpsc_channel_sender ]=================================================== */

template <>
agt_status_t object_info<local_mpsc_channel_sender>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  auto sender = static_cast<handle_type*>(object);
  if (!sender->channel)
    return AGT_ERROR_NOT_BOUND;
  return object_info<ObjectType>::acquireMessage(sender->channel, pStagedMessage, timeout);
}

template <>
void      object_info<local_mpsc_channel_sender>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  auto sender = static_cast<handle_type*>(object);
  object_info<ObjectType>::pushQueue(sender->channel, message, flags);
}

template <>
agt_status_t object_info<local_mpsc_channel_sender>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
  // auto sender = static_cast<handle_type*>(object);
  // if (!sender->channel)
  //   return AGT_ERROR_NOT_BOUND;
  // return object_info<ObjectType>::popQueue(sender->channel, pMessageInfo, timeout);
}

template <>
void      object_info<local_mpsc_channel_sender>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  // object_info<handle_type>::releaseMessage(static_cast<handle_type*>(object)->channel, message);
}

template <>
agt_status_t object_info<local_mpsc_channel_sender>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {
  auto sender = static_cast<handle_type*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
agt_status_t object_info<local_mpsc_channel_sender>::acquireRef(handle_header* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      object_info<local_mpsc_channel_sender>::releaseRef(handle_header* object) noexcept { }


agt_status_t Agt::createInstance(local_mpsc_channel_sender*& objectRef, agt_ctx_t ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/** =================================[ local_mpsc_channel_receiver ]================================================= */

template <>
agt_status_t object_info<local_mpsc_channel_receiver>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
}

template <>
void      object_info<local_mpsc_channel_receiver>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  // auto sender = static_cast<handle_type*>(object);
  // object_info<private_channel>::pushQueue(sender->channel, message, flags);
}

template <>
agt_status_t object_info<local_mpsc_channel_receiver>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  auto receiver = static_cast<handle_type*>(object);
  if (!receiver->channel)
    return AGT_ERROR_NOT_BOUND;
  return object_info<ObjectType>::popQueue(receiver->channel, pMessageInfo, timeout);
}

template <>
void      object_info<local_mpsc_channel_receiver>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  object_info<ObjectType>::releaseMessage(static_cast<handle_type*>(object)->channel, message);
}

template <>
agt_status_t object_info<local_mpsc_channel_receiver>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {
  auto receiver = static_cast<handle_type*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
agt_status_t object_info<local_mpsc_channel_receiver>::acquireRef(handle_header* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      object_info<local_mpsc_channel_receiver>::releaseRef(handle_header* object) noexcept { }

agt_status_t Agt::createInstance(local_mpsc_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/** =================================[ local_mpmc_channel_sender ]=================================================== */

template <>
agt_status_t object_info<local_mpmc_channel_sender>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  auto sender = static_cast<handle_type*>(object);
  if (!sender->channel)
    return AGT_ERROR_NOT_BOUND;
  return object_info<ObjectType>::acquireMessage(sender->channel, pStagedMessage, timeout);
}

template <>
void      object_info<local_mpmc_channel_sender>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  auto sender = static_cast<handle_type*>(object);
  object_info<ObjectType>::pushQueue(sender->channel, message, flags);
}

template <>
agt_status_t object_info<local_mpmc_channel_sender>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
  // auto sender = static_cast<handle_type*>(object);
  // if (!sender->channel)
  //   return AGT_ERROR_NOT_BOUND;
  // return object_info<ObjectType>::popQueue(sender->channel, pMessageInfo, timeout);
}

template <>
void      object_info<local_mpmc_channel_sender>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  // object_info<handle_type>::releaseMessage(static_cast<handle_type*>(object)->channel, message);
}

template <>
agt_status_t object_info<local_mpmc_channel_sender>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {
  auto sender = static_cast<handle_type*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
agt_status_t object_info<local_mpmc_channel_sender>::acquireRef(handle_header* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      object_info<local_mpmc_channel_sender>::releaseRef(handle_header* object) noexcept { }


agt_status_t Agt::createInstance(local_mpmc_channel_sender*& objectRef, agt_ctx_t ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/** =================================[ local_mpmc_channel_receiver ]================================================= */

template <>
agt_status_t object_info<local_mpmc_channel_receiver>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
  return AGT_ERROR_INVALID_OPERATION;
}

template <>
void      object_info<local_mpmc_channel_receiver>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept {
  // auto sender = static_cast<handle_type*>(object);
  // object_info<private_channel>::pushQueue(sender->channel, message, flags);
}

template <>
agt_status_t object_info<local_mpmc_channel_receiver>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
  auto receiver = static_cast<handle_type*>(object);
  if (!receiver->channel)
    return AGT_ERROR_NOT_BOUND;
  return object_info<ObjectType>::popQueue(receiver->channel, pMessageInfo, timeout);
}

template <>
void      object_info<local_mpmc_channel_receiver>::releaseMessage(handle_header* object, agt_message_t message) noexcept {
  object_info<ObjectType>::releaseMessage(static_cast<handle_type*>(object)->channel, message);
}

template <>
agt_status_t object_info<local_mpmc_channel_receiver>::connect(handle_header* object, handle_header* handle, ConnectAction action) noexcept {
  auto receiver = static_cast<handle_type*>(object);
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
agt_status_t object_info<local_mpmc_channel_receiver>::acquireRef(handle_header* object) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

template <>
void      object_info<local_mpmc_channel_receiver>::releaseRef(handle_header* object) noexcept { }

agt_status_t Agt::createInstance(local_mpmc_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}




/** =================================[ shared_spsc_channel_handle ]================================================== */
/** =================================[ shared_spsc_channel_sender ]================================================== */
/** =================================[ shared_spsc_channel_receiver ]================================================ */
/** =================================[ shared_spmc_channel_handle ]================================================== */
/** =================================[ shared_spmc_channel_sender ]================================================== */
/** =================================[ shared_spmc_channel_receiver ]================================================ */
/** =================================[ shared_mpsc_channel_handle ]================================================== */
/** =================================[ shared_mpsc_channel_sender ]================================================== */
/** =================================[ shared_mpsc_channel_receiver ]================================================ */
/** =================================[ shared_mpmc_channel_handle ]================================================== */
/** =================================[ shared_mpmc_channel_sender ]================================================== */
/** =================================[ shared_mpmc_channel_receiver ]================================================ */



