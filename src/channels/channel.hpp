//
// Created by maxwe on 2021-12-28.
//

#ifndef JEMSYS_AGATE2_CHANNEL_HPP
#define JEMSYS_AGATE2_CHANNEL_HPP

#include "core/objects.hpp"
#include "fwd.hpp"
#include "messages/message.hpp"

// #include <ipc/offset_ptr.hpp>


namespace Agt {

  struct PrivateChannelMessage;
  struct LocalChannelMessage;
  struct SharedChannelMessage;


  class PrivateChannel;
  class LocalSpScChannel;
  class LocalSpMcChannel;
  class LocalMpScChannel;
  class LocalMpMcChannel;
  class SharedSpScChannel;
  class SharedSpMcChannel;
  class SharedMpScChannel;
  class SharedMpMcChannel;

  class SharedSpScChannelHandle;
  class SharedSpMcChannelHandle;
  class SharedMpScChannelHandle;
  class SharedMpMcChannelHandle;

  class PrivateChannelSender;
  class LocalSpScChannelSender;
  class LocalSpMcChannelSender;
  class LocalMpScChannelSender;
  class LocalMpMcChannelSender;
  class SharedSpScChannelSender;
  class SharedSpMcChannelSender;
  class SharedMpScChannelSender;
  class SharedMpMcChannelSender;

  class PrivateChannelReceiver;
  class LocalSpScChannelReceiver;
  class LocalSpMcChannelReceiver;
  class LocalMpScChannelReceiver;
  class LocalMpMcChannelReceiver;
  class SharedSpScChannelReceiver;
  class SharedSpMcChannelReceiver;
  class SharedMpScChannelReceiver;
  class SharedMpMcChannelReceiver;


  AgtStatus createInstance(PrivateChannel*& outHandle,   AgtContext ctx, const AgtChannelCreateInfo& createInfo, PrivateChannelSender* sender, PrivateChannelReceiver* receiver) noexcept;
  AgtStatus createInstance(LocalSpScChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, LocalSpScChannelSender* sender, LocalSpScChannelReceiver* receiver) noexcept;
  AgtStatus createInstance(LocalSpMcChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, LocalSpMcChannelSender* sender, LocalSpMcChannelReceiver* receiver) noexcept;
  AgtStatus createInstance(LocalMpScChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, LocalMpScChannelSender* sender, LocalMpScChannelReceiver* receiver) noexcept;
  AgtStatus createInstance(LocalMpMcChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, LocalMpMcChannelSender* sender, LocalMpMcChannelReceiver* receiver) noexcept;

  AgtStatus createInstance(PrivateChannelSender*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(PrivateChannelReceiver*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(LocalSpScChannelSender*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(LocalSpScChannelReceiver*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(LocalMpScChannelSender*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(LocalMpScChannelReceiver*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(LocalSpMcChannelSender*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(LocalSpMcChannelReceiver*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(LocalMpMcChannelSender*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(LocalMpMcChannelReceiver*& objectRef, AgtContext ctx) noexcept;


  AgtStatus createInstance(SharedSpScChannelHandle*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, SharedSpScChannelSender* sender, SharedSpScChannelReceiver* receiver) noexcept;
  AgtStatus createInstance(SharedSpMcChannelHandle*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, SharedSpMcChannelSender* sender, SharedSpMcChannelReceiver* receiver) noexcept;
  AgtStatus createInstance(SharedMpScChannelHandle*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, SharedMpScChannelSender* sender, SharedMpScChannelReceiver* receiver) noexcept;
  AgtStatus createInstance(SharedMpMcChannelHandle*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, SharedMpMcChannelSender* sender, SharedMpMcChannelReceiver* receiver) noexcept;


  AgtStatus createInstance(SharedSpScChannelSender*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(SharedSpScChannelReceiver*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(SharedMpScChannelSender*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(SharedMpScChannelReceiver*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(SharedSpMcChannelSender*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(SharedSpMcChannelReceiver*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(SharedMpMcChannelSender*& objectRef, AgtContext ctx) noexcept;
  AgtStatus createInstance(SharedMpMcChannelReceiver*& objectRef, AgtContext ctx) noexcept;


  // sizeof == 64 bytes, or 1 cache line
  struct LocalChannelHeader : HandleHeader {
    AgtSize        slotCount;
    AgtSize        inlineBufferSize;
    std::byte*     messageSlots;
  };

  // sizeof == 64 bytes, or 1 cache line
  struct SharedChannelHeader : SharedObjectHeader {
    ReferenceCount     refCount;
    AgtSize            slotCount;
    AgtSize            inlineBufferSize;
    SharedAllocationId messageSlotsId;
  };


  // sizeof == 128 bytes, or 2 cache lines
  struct AGT_cache_aligned PrivateChannel   : LocalChannelHeader {
  AGT_cache_aligned
    AgtHandle  consumer;
    AgtHandle  producer;
    AgtSize    availableSlotCount;
    AgtSize    queuedMessageCount;
    AgtMessage nextFreeSlot;
    AgtMessage prevReceivedMessage;
    AgtMessage prevQueuedMessage;
    AgtSize    refCount;
  };

  struct AGT_cache_aligned LocalSpScChannel : LocalChannelHeader {
  AGT_cache_aligned
    semaphore_t    slotSemaphore;
    AgtMessage     producerPreviousQueuedMsg;
    AgtMessage     producerNextFreeSlot;

  AGT_cache_aligned
    semaphore_t    queuedMessages;
    AgtMessage     consumerPreviousMsg;

  AGT_cache_aligned
    ReferenceCount refCount;
    AgtMessage     lastFreeSlot;
    HandleHeader*  consumer;
    HandleHeader*  producer;
  };
  struct AGT_cache_aligned LocalMpScChannel : LocalChannelHeader {
  AGT_cache_aligned
    std::atomic<AgtMessage> nextFreeSlot;
    semaphore_t             slotSemaphore;
  AGT_cache_aligned
    std::atomic<AgtMessage> lastQueuedSlot;
    std::atomic<AgtMessage> lastFreeSlot;
    AgtMessage              previousReceivedMessage;
  AGT_cache_aligned
    mpsc_counter_t          queuedMessageCount;
    ReferenceCount          refCount;
    HandleHeader*           consumer;
    jem_u32_t               maxProducers;
    semaphore_t             producerSemaphore;
  };
  struct AGT_cache_aligned LocalSpMcChannel : LocalChannelHeader {
  AGT_cache_aligned
    semaphore_t                       slotSemaphore;
    std::atomic<AgtMessage> lastFreeSlot;
    AgtMessage              nextFreeSlot;
    AgtMessage              lastQueuedSlot;
  AGT_cache_aligned
    std::atomic<AgtMessage> previousReceivedMessage;
  AGT_cache_aligned
    semaphore_t                       queuedMessages;
    ReferenceCount                    refCount;
    HandleHeader*                     producer;
    jem_u32_t                         maxConsumers;
    semaphore_t                       consumerSemaphore;
  };
  struct AGT_cache_aligned LocalMpMcChannel : LocalChannelHeader {
  AGT_cache_aligned
    std::atomic<AgtMessage> nextFreeSlot;
    std::atomic<AgtMessage> lastQueuedSlot;
  AGT_cache_aligned
    semaphore_t             slotSemaphore;
    std::atomic<AgtMessage> previousReceivedMessage;
  AGT_cache_aligned
    semaphore_t                       queuedMessages;
    jem_u32_t                         maxProducers;
    jem_u32_t                         maxConsumers;
    ReferenceCount                    refCount;
    semaphore_t                       producerSemaphore;
    semaphore_t                       consumerSemaphore;
  };


  struct AGT_cache_aligned SharedSpScChannel : SharedChannelHeader {
  AGT_cache_aligned
    semaphore_t        availableSlotSema;
    binary_semaphore_t producerSemaphore;
    AgtSize            producerPreviousMsgOffset;
  AGT_cache_aligned
    semaphore_t        queuedMessageSema;
    binary_semaphore_t consumerSemaphore;
    AgtSize            consumerPreviousMsgOffset;
    AgtSize            consumerLastFreeSlotOffset;
  AGT_cache_aligned
    atomic_size_t      nextFreeSlotIndex;
  
  
    using HandleType = SharedSpScChannelHandle;
  };
  struct AGT_cache_aligned SharedMpScChannel : SharedChannelHeader {
  AGT_cache_aligned
    semaphore_t    slotSemaphore;
    std::atomic<AgtMessage> nextFreeSlot;
    jem_size_t              payloadOffset;
  AGT_cache_aligned
    std::atomic<AgtMessage> lastQueuedSlot;
    AgtMessage              previousReceivedMessage;
  AGT_cache_aligned
    mpsc_counter_t     queuedMessageCount;
    jem_u32_t          maxProducers;
    binary_semaphore_t consumerSemaphore;
    semaphore_t        producerSemaphore;
    
    using HandleType = SharedMpScChannelHandle;
  };
  struct AGT_cache_aligned SharedSpMcChannel : SharedChannelHeader {
    ReferenceCount     refCount;
    AgtSize            slotCount;
    AgtSize            slotSize;
    AgtObjectId        messageSlotsId;
  AGT_cache_aligned
    atomic_size_t      nextFreeSlot;
    semaphore_t        slotSemaphore;
  AGT_cache_aligned
    atomic_size_t      lastQueuedSlot;
  AGT_cache_aligned
    atomic_u32_t       queuedSinceLastCheck;
    jem_u32_t          minQueuedMessages;
    jem_u32_t          maxConsumers;
    binary_semaphore_t producerSemaphore;
    semaphore_t        consumerSemaphore;
    
    using HandleType = SharedSpMcChannelHandle;
  };
  struct AGT_cache_aligned SharedMpMcChannel : SharedChannelHeader {
    AgtSize          slotCount;
    AgtSize          slotSize;
    AgtObjectId      messageSlotsId;
    semaphore_t      slotSemaphore;
  AGT_cache_aligned
    atomic_size_t    nextFreeSlot;
  AGT_cache_aligned
    atomic_size_t    lastQueuedSlot;
  AGT_cache_aligned
    atomic_u32_t     queuedSinceLastCheck;
    ReferenceCount   refCount;
    jem_u32_t        minQueuedMessages;
    jem_u32_t        maxProducers;         // 220
    jem_u32_t        maxConsumers;         // 224
    semaphore_t      producerSemaphore;    // 240
    semaphore_t      consumerSemaphore;    // 256
    
    using HandleType = SharedMpMcChannelHandle;
  };


  struct PrivateChannelSender : HandleHeader {
    PrivateChannel* channel;

    using ObjectType = PrivateChannel;
  };
  struct PrivateChannelReceiver : HandleHeader {
    PrivateChannel* channel;

    using ObjectType = PrivateChannel;
  };
  struct LocalSpScChannelSender : HandleHeader {
    LocalSpScChannel* channel;

    using ObjectType = LocalSpScChannel;
  };
  struct LocalSpScChannelReceiver : HandleHeader {
    LocalSpScChannel* channel;

    using ObjectType = LocalSpScChannel;
  };
  struct LocalMpScChannelSender : HandleHeader {
    LocalMpScChannel* channel;

    using ObjectType = LocalMpScChannel;
  };
  struct LocalMpScChannelReceiver : HandleHeader {
    LocalMpScChannel* channel;

    using ObjectType = LocalMpScChannel;
  };
  struct LocalSpMcChannelSender : HandleHeader {
    LocalSpMcChannel* channel;

    using ObjectType = LocalSpMcChannel;
  };
  struct LocalSpMcChannelReceiver : HandleHeader {
    LocalSpMcChannel* channel;

    using ObjectType = LocalSpMcChannel;
  };
  struct LocalMpMcChannelSender : HandleHeader {
    LocalMpMcChannel* channel;

    using ObjectType = LocalMpMcChannel;
  };
  struct LocalMpMcChannelReceiver : HandleHeader {
    LocalMpMcChannel* channel;

    using ObjectType = LocalMpMcChannel;
  };


  struct SharedSpScChannelHandle   : SharedHandleHeader {
    AgtSize    slotCount;
    AgtSize    inlineBufferSize;
    std::byte* messageSlots;

    using ObjectType = SharedSpScChannel;
  };
  struct SharedSpScChannelSender   : SharedHandleHeader {
    AgtSize    slotCount;
    AgtSize    inlineBufferSize;
    std::byte* messageSlots;
    AgtSize    previousMessageOffset;

    using ObjectType = SharedSpScChannel;
  };
  struct SharedSpScChannelReceiver : SharedHandleHeader {
    AgtSize    slotCount;
    AgtSize    inlineBufferSize;
    std::byte* messageSlots;
    AgtSize    previousMessageOffset;
    AgtSize    lastFreeSlotOffset;

    using ObjectType = SharedSpScChannel;
  };

  struct SharedMpScChannelHandle   : SharedHandleHeader {

    using ObjectType = SharedMpScChannel;
  };
  struct SharedMpScChannelSender   : SharedHandleHeader {

    using ObjectType = SharedMpScChannel;
  };
  struct SharedMpScChannelReceiver : SharedHandleHeader {

    using ObjectType = SharedMpScChannel;
  };

  struct SharedSpMcChannelHandle   : SharedHandleHeader {

    using ObjectType = SharedSpMcChannel;
  };
  struct SharedSpMcChannelSender   : SharedHandleHeader {

    using ObjectType = SharedSpMcChannel;
  };
  struct SharedSpMcChannelReceiver : SharedHandleHeader {

    using ObjectType = SharedSpMcChannel;
  };

  struct SharedMpMcChannelHandle   : SharedHandleHeader {

    using ObjectType = SharedMpMcChannel;
  };
  struct SharedMpMcChannelSender   : SharedHandleHeader {

    using ObjectType = SharedMpMcChannel;
  };
  struct SharedMpMcChannelReceiver : SharedHandleHeader {

    using ObjectType = SharedMpMcChannel;
  };





  AGT_declare_vtable(PrivateChannel);
  AGT_declare_vtable(PrivateChannelSender);
  AGT_declare_vtable(PrivateChannelReceiver);
  
  AGT_declare_vtable(LocalSpScChannel);
  AGT_declare_vtable(LocalSpScChannelSender);
  AGT_declare_vtable(LocalSpScChannelReceiver);
  
  AGT_declare_vtable(LocalMpScChannel);
  AGT_declare_vtable(LocalMpScChannelSender);
  AGT_declare_vtable(LocalMpScChannelReceiver);
  
  AGT_declare_vtable(LocalSpMcChannel);
  AGT_declare_vtable(LocalSpMcChannelSender);
  AGT_declare_vtable(LocalSpMcChannelReceiver);
  
  AGT_declare_vtable(LocalMpMcChannel);
  AGT_declare_vtable(LocalMpMcChannelSender);
  AGT_declare_vtable(LocalMpMcChannelReceiver);
  
  
  
  AGT_declare_vtable(SharedSpScChannelHandle);
  AGT_declare_vtable(SharedSpScChannelSender);
  AGT_declare_vtable(SharedSpScChannelReceiver);
  
  AGT_declare_vtable(SharedMpScChannelHandle);
  AGT_declare_vtable(SharedMpScChannelSender);
  AGT_declare_vtable(SharedMpScChannelReceiver);
  
  AGT_declare_vtable(SharedSpMcChannelHandle);
  AGT_declare_vtable(SharedSpMcChannelSender);
  AGT_declare_vtable(SharedSpMcChannelReceiver);
  
  AGT_declare_vtable(SharedMpMcChannelHandle);
  AGT_declare_vtable(SharedMpMcChannelSender);
  AGT_declare_vtable(SharedMpMcChannelReceiver);
}

#endif//JEMSYS_AGATE2_CHANNEL_HPP
