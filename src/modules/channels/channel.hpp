//
// Created by maxwe on 2021-12-28.
//

#ifndef JEMSYS_AGATE2_CHANNEL_HPP
#define JEMSYS_AGATE2_CHANNEL_HPP

#include "config.hpp"
#include "message.hpp"
#include "message_pool.hpp"
#include "modules/core/object.hpp"

// #include <ipc/offset_ptr.hpp>


namespace agt {


  /*
  enum channel_kind_t {
    local_spsc_channel_kind,
    local_mpsc_channel_kind,
    local_spmc_channel_kind,
    local_mpmc_channel_kind,
    shared_spsc_channel_kind,
    shared_mpsc_channel_kind,
    shared_spmc_channel_kind,
    shared_mpmc_channel_kind,
    private_channel_kind,
    local_spsc_fixed_size_channel_kind,
    local_mpsc_fixed_size_channel_kind,
    local_spmc_fixed_size_channel_kind,
    local_mpmc_fixed_size_channel_kind,
    shared_spsc_fixed_size_channel_kind,
    shared_mpsc_fixed_size_channel_kind,
    shared_spmc_fixed_size_channel_kind,
    shared_mpmc_fixed_size_channel_kind,
    private_fixed_size_channel_kind,
  };

  struct channel {
    channel_kind_t kind;
    agt_u32_t      refCount;
    agt_ctx_t      ctx;
    // message_pool_t msgPool;
  };

  struct dynamic_channel : channel {

  };

  struct fixed_channel   : channel {

  };



  // sizeof == 64 bytes, or 1 cache line
  struct local_channel_header : handle_header {
    size_t        slotCount;
    size_t        inlineBufferSize;
    std::byte*     messageSlots;
  };

  // sizeof == 64 bytes, or 1 cache line
  struct shared_channel_header : shared_object_header {
    ref_count refCount;
    size_t            slotCount;
    size_t            inlineBufferSize;
    SharedAllocationId messageSlotsId;
  };


  // sizeof == 128 bytes, or 2 cache lines
  struct AGT_cache_aligned private_channel    : local_channel_header {
  AGT_cache_aligned
    agt_handle_t  consumer;
    agt_handle_t  producer;
    size_t    availableSlotCount;
    size_t    queuedMessageCount;
    agt_message_t nextFreeSlot;
    agt_message_t prevReceivedMessage;
    agt_message_t prevQueuedMessage;
    size_t    refCount;
  };

  struct AGT_cache_aligned local_spsc_channel : local_channel_header {
  AGT_cache_aligned
    semaphore    slotSemaphore;
    agt_message_t     producerPreviousQueuedMsg;
    agt_message_t     producerNextFreeSlot;

  AGT_cache_aligned
    semaphore    queuedMessages;
    agt_message_t     consumerPreviousMsg;

  AGT_cache_aligned
        ref_count refCount;
    agt_message_t     lastFreeSlot;
    handle_header*  consumer;
    handle_header*  producer;
  };
  struct AGT_cache_aligned local_mpsc_channel : local_channel_header {
  AGT_cache_aligned
    std::atomic<agt_message_t> nextFreeSlot;
    semaphore             slotSemaphore;
  AGT_cache_aligned
    std::atomic<agt_message_t> lastQueuedSlot;
    std::atomic<agt_message_t> lastFreeSlot;
    agt_message_t              previousReceivedMessage;
  AGT_cache_aligned
    mpsc_counter_t          queuedMessageCount;
  ref_count refCount;
    handle_header*           consumer;
    agt_u32_t               maxProducers;
    semaphore             producerSemaphore;
  };
  struct AGT_cache_aligned local_spmc_channel : local_channel_header {
  AGT_cache_aligned
    semaphore                       slotSemaphore;
    std::atomic<agt_message_t> lastFreeSlot;
    agt_message_t              nextFreeSlot;
    agt_message_t              lastQueuedSlot;
  AGT_cache_aligned
    std::atomic<agt_message_t> previousReceivedMessage;
  AGT_cache_aligned
    semaphore                       queuedMessages;
  ref_count refCount;
    handle_header*                     producer;
    agt_u32_t                         maxConsumers;
    semaphore                       consumerSemaphore;
  };
  struct AGT_cache_aligned local_mpmc_channel : local_channel_header {
  AGT_cache_aligned
    std::atomic<agt_message_t> nextFreeSlot;
    std::atomic<agt_message_t> lastQueuedSlot;
  AGT_cache_aligned
    semaphore             slotSemaphore;
    std::atomic<agt_message_t> previousReceivedMessage;
  AGT_cache_aligned
    semaphore                       queuedMessages;
    agt_u32_t                         maxProducers;
    agt_u32_t                         maxConsumers;
    ref_count refCount;
    semaphore                       producerSemaphore;
    semaphore                       consumerSemaphore;
  };


  struct AGT_cache_aligned shared_spsc_channel : shared_channel_header {
  AGT_cache_aligned
    semaphore        availableSlotSema;
    binary_semaphore producerSemaphore;
    size_t            producerPreviousMsgOffset;
  AGT_cache_aligned
    semaphore        queuedMessageSema;
    binary_semaphore consumerSemaphore;
    size_t            consumerPreviousMsgOffset;
    size_t            consumerLastFreeSlotOffset;
  AGT_cache_aligned
    atomic_size_t      nextFreeSlotIndex;
  
  
    using handle_type = shared_spsc_channel_handle;
  };
  struct AGT_cache_aligned shared_mpsc_channel : shared_channel_header {
  AGT_cache_aligned
    semaphore    slotSemaphore;
    std::atomic<agt_message_t> nextFreeSlot;
    size_t              payloadOffset;
  AGT_cache_aligned
    std::atomic<agt_message_t> lastQueuedSlot;
    agt_message_t              previousReceivedMessage;
  AGT_cache_aligned
    mpsc_counter_t     queuedMessageCount;
    agt_u32_t          maxProducers;
    binary_semaphore consumerSemaphore;
    semaphore        producerSemaphore;
    
    using handle_type = shared_mpsc_channel_handle;
  };
  struct AGT_cache_aligned shared_spmc_channel : shared_channel_header {
    ref_count refCount;
    size_t            slotCount;
    size_t            slotSize;
    agt_object_id_t        messageSlotsId;
  AGT_cache_aligned
    atomic_size_t      nextFreeSlot;
    semaphore        slotSemaphore;
  AGT_cache_aligned
    atomic_size_t      lastQueuedSlot;
  AGT_cache_aligned
    atomic_u32_t       queuedSinceLastCheck;
    agt_u32_t          minQueuedMessages;
    agt_u32_t          maxConsumers;
    binary_semaphore producerSemaphore;
    semaphore        consumerSemaphore;
    
    using handle_type = shared_spmc_channel_handle;
  };
  struct AGT_cache_aligned shared_mpmc_channel : shared_channel_header {
    size_t          slotCount;
    size_t          slotSize;
    agt_object_id_t      messageSlotsId;
    semaphore      slotSemaphore;
  AGT_cache_aligned
    atomic_size_t    nextFreeSlot;
  AGT_cache_aligned
    atomic_size_t    lastQueuedSlot;
  AGT_cache_aligned
    atomic_u32_t     queuedSinceLastCheck;
  ref_count refCount;
    agt_u32_t        minQueuedMessages;
    agt_u32_t        maxProducers;         // 220
    agt_u32_t        maxConsumers;         // 224
    semaphore      producerSemaphore;    // 240
    semaphore      consumerSemaphore;    // 256
    
    using handle_type = shared_mpmc_channel_handle;
  };


  struct private_channel_sender : handle_header {
    private_channel* channel;

    using object_type = private_channel;
  };
  struct private_channel_receiver : handle_header {
    private_channel* channel;

    using object_type = private_channel;
  };
  struct local_spsc_channel_sender : handle_header {
    local_spsc_channel* channel;

    using object_type = local_spsc_channel;
  };
  struct local_spsc_channel_receiver : handle_header {
    local_spsc_channel* channel;

    using object_type = local_spsc_channel;
  };
  struct local_mpsc_channel_sender : handle_header {
    local_mpsc_channel* channel;

    using object_type = local_mpsc_channel;
  };
  struct local_mpsc_channel_receiver : handle_header {
    local_mpsc_channel* channel;

    using object_type = local_mpsc_channel;
  };
  struct local_spmc_channel_sender : handle_header {
    local_spmc_channel* channel;

    using object_type = local_spmc_channel;
  };
  struct local_spmc_channel_receiver : handle_header {
    local_spmc_channel* channel;

    using object_type = local_spmc_channel;
  };
  struct local_mpmc_channel_sender : handle_header {
    local_mpmc_channel* channel;

    using object_type = local_mpmc_channel;
  };
  struct local_mpmc_channel_receiver : handle_header {
    local_mpmc_channel* channel;

    using object_type = local_mpmc_channel;
  };


  struct shared_spsc_channel_handle   : shared_handle_header {
    size_t    slotCount;
    size_t    inlineBufferSize;
    std::byte* messageSlots;

    using object_type = shared_spsc_channel;
  };
  struct shared_spsc_channel_sender   : shared_handle_header {
    size_t    slotCount;
    size_t    inlineBufferSize;
    std::byte* messageSlots;
    size_t    previousMessageOffset;

    using object_type = shared_spsc_channel;
  };
  struct shared_spsc_channel_receiver : shared_handle_header {
    size_t    slotCount;
    size_t    inlineBufferSize;
    std::byte* messageSlots;
    size_t    previousMessageOffset;
    size_t    lastFreeSlotOffset;

    using object_type = shared_spsc_channel;
  };

  struct shared_mpsc_channel_handle   : shared_handle_header {

    using object_type = shared_mpsc_channel;
  };
  struct shared_mpsc_channel_sender   : shared_handle_header {

    using object_type = shared_mpsc_channel;
  };
  struct shared_mpsc_channel_receiver : shared_handle_header {

    using object_type = shared_mpsc_channel;
  };

  struct shared_spmc_channel_handle   : shared_handle_header {

    using object_type = shared_spmc_channel;
  };
  struct shared_spmc_channel_sender   : shared_handle_header {

    using object_type = shared_spmc_channel;
  };
  struct shared_spmc_channel_receiver : shared_handle_header {

    using object_type = shared_spmc_channel;
  };

  struct shared_mpmc_channel_handle   : shared_handle_header {

    using object_type = shared_mpmc_channel;
  };
  struct shared_mpmc_channel_sender   : shared_handle_header {

    using object_type = shared_mpmc_channel;
  };
  struct shared_mpmc_channel_receiver : shared_handle_header {

    using object_type = shared_mpmc_channel;
  };



  agt_status_t createInstance(private_channel*& outHandle,   agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, private_channel_sender* sender, private_channel_receiver* receiver) noexcept;
  agt_status_t createInstance(local_spsc_channel*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, local_spsc_channel_sender* sender, local_spsc_channel_receiver* receiver) noexcept;
  agt_status_t createInstance(local_spmc_channel*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, local_spmc_channel_sender* sender, local_spmc_channel_receiver* receiver) noexcept;
  agt_status_t createInstance(local_mpsc_channel*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, local_mpsc_channel_sender* sender, local_mpsc_channel_receiver* receiver) noexcept;
  agt_status_t createInstance(local_mpmc_channel*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, local_mpmc_channel_sender* sender, local_mpmc_channel_receiver* receiver) noexcept;

  agt_status_t createInstance(private_channel_sender*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(private_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(local_spsc_channel_sender*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(local_spsc_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(local_mpsc_channel_sender*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(local_mpsc_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(local_spmc_channel_sender*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(local_spmc_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(local_mpmc_channel_sender*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(local_mpmc_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept;


  agt_status_t createInstance(shared_spsc_channel_handle*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, shared_spsc_channel_sender* sender, shared_spsc_channel_receiver* receiver) noexcept;
  agt_status_t createInstance(shared_spmc_channel_handle*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, shared_spmc_channel_sender* sender, shared_spmc_channel_receiver* receiver) noexcept;
  agt_status_t createInstance(shared_mpsc_channel_handle*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, shared_mpsc_channel_sender* sender, shared_mpsc_channel_receiver* receiver) noexcept;
  agt_status_t createInstance(shared_mpmc_channel_handle*& outHandle, agt_ctx_t ctx, const agt_channel_create_info_t& createInfo, shared_mpmc_channel_sender* sender, shared_mpmc_channel_receiver* receiver) noexcept;


  agt_status_t createInstance(shared_spsc_channel_sender*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(shared_spsc_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(shared_mpsc_channel_sender*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(shared_mpsc_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(shared_spmc_channel_sender*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(shared_spmc_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(shared_mpmc_channel_sender*& objectRef, agt_ctx_t ctx) noexcept;
  agt_status_t createInstance(shared_mpmc_channel_receiver*& objectRef, agt_ctx_t ctx) noexcept;




  AGT_declare_vtable(private_channel);
  AGT_declare_vtable(private_channel_sender);
  AGT_declare_vtable(private_channel_receiver);
  
  AGT_declare_vtable(local_spsc_channel);
  AGT_declare_vtable(local_spsc_channel_sender);
  AGT_declare_vtable(local_spsc_channel_receiver);
  
  AGT_declare_vtable(local_mpsc_channel);
  AGT_declare_vtable(local_mpsc_channel_sender);
  AGT_declare_vtable(local_mpsc_channel_receiver);
  
  AGT_declare_vtable(local_spmc_channel);
  AGT_declare_vtable(local_spmc_channel_sender);
  AGT_declare_vtable(local_spmc_channel_receiver);
  
  AGT_declare_vtable(local_mpmc_channel);
  AGT_declare_vtable(local_mpmc_channel_sender);
  AGT_declare_vtable(local_mpmc_channel_receiver);
  
  
  
  AGT_declare_vtable(shared_spsc_channel_handle);
  AGT_declare_vtable(shared_spsc_channel_sender);
  AGT_declare_vtable(shared_spsc_channel_receiver);
  
  AGT_declare_vtable(shared_mpsc_channel_handle);
  AGT_declare_vtable(shared_mpsc_channel_sender);
  AGT_declare_vtable(shared_mpsc_channel_receiver);
  
  AGT_declare_vtable(shared_spmc_channel_handle);
  AGT_declare_vtable(shared_spmc_channel_sender);
  AGT_declare_vtable(shared_spmc_channel_receiver);
  
  AGT_declare_vtable(shared_mpmc_channel_handle);
  AGT_declare_vtable(shared_mpmc_channel_sender);
  AGT_declare_vtable(shared_mpmc_channel_receiver);*/
}

#endif//JEMSYS_AGATE2_CHANNEL_HPP
