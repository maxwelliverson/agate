//
// Created by maxwe on 2023-01-13.
//

#ifndef AGATE_CHANNELS_MODULE_RING_QUEUE_HPP
#define AGATE_CHANNELS_MODULE_RING_QUEUE_HPP


#include "core/object.hpp"
#include "core/ctx.hpp"


namespace agt {

  struct ring_buffer_msg {
    agt_u32_t       nextOffset;
    agt_u32_t       msgSize;
    agt_timestamp_t sendTime;
    async_data_t    asyncData;
    async_key_t     asyncKey;
    // 4 free bytes here:
  };

  static_assert(sizeof(ring_buffer_msg) == 32);

  struct local_sp_ring_queue_sender;

  AGT_final_object_type(local_sp_ring_queue_receiver) {
    agt_u32_t                   bufferLength;
    std::byte*                  messageBuffer;
    local_sp_ring_queue_sender* sender;
    agt_u32_t                   releasedSize;
    agt_u32_t                   cachedEnqueuedMsgCount;
    agt_u32_t                   headOffset;
    agt_u32_t*                  pEnqueuedMessageCount;
  };

  AGT_final_object_type(local_sp_ring_queue_sender) {
    agt_u32_t                     bufferLength;
    std::byte*                    messageBuffer;
    local_sp_ring_queue_receiver* receiver;
    agt_u32_t                     enqueuedMessageCount;
    agt_u32_t                     tailOffset;
    agt_u32_t                     availableSize;
    agt_u32_t                     maxSendSize;
    agt_u32_t*                    pReleasedSize;
    agt_ctx_t                     ctx;
  };

  AGT_final_object_type(local_mp_ring_queue_receiver) {
    agt_u32_t  bufferLength;
    std::byte* messageBuffer;
    agt_u32_t  cachedQueuedSize;
    agt_u32_t  availableSize;
    agt_u32_t  headOffset;
    agt_u32_t  tailOffset;
    agt_u32_t  maxSenders;
    agt_u32_t  attachedSenders;
  };

  AGT_final_object_type(local_mp_ring_queue_sender) {
    std::byte* messageBuffer;
    agt_u32_t* pAvailableSize;
    agt_u32_t* pTailOffset;
  };


  AGT_forceinline ring_buffer_msg* _get_msg(std::byte* buffer, agt_u32_t offset) noexcept {
    return reinterpret_cast<ring_buffer_msg*>(buffer + offset);
  }

  AGT_forceinline agt_u32_t        _get_aligned_total_size(agt_u32_t size) noexcept {
    return align_to<16u>(size + sizeof(ring_buffer_msg));
  }

  void _wait(local_sp_ring_queue_sender* sender, size_t size, agt_u32_t& availableSize) noexcept;

  bool _wait_for(local_sp_ring_queue_sender* sender, size_t size, agt_u32_t& availableSize, agt_timeout_t timeout) noexcept;

  AGT_forceinline static void* _get_msg_buffer(ring_buffer_msg* msg) noexcept {
    return reinterpret_cast<std::byte*>(msg) + sizeof(ring_buffer_msg);
  }

  agt_status_t _acquire_msg(local_sp_ring_queue_sender* sender, size_t size, void** pMsg, agt_timeout_t timeout) noexcept {
    AGT_invariant( pMsg != nullptr );

    std::atomic_thread_fence(std::memory_order_acquire);

    const agt_u32_t requiredSize = _get_aligned_total_size(size);

    if (!sender->receiver)
      return AGT_ERROR_NO_RECEIVERS;

    if (size >= sender->maxSendSize)
      return AGT_ERROR_MESSAGE_TOO_LARGE;

    agt_u32_t availableSize = sender->availableSize;

    if (availableSize < requiredSize) {
      availableSize += atomicExchange(*sender->pReleasedSize, 0);
      if (availableSize < requiredSize) {
        switch (timeout) {
          case AGT_DO_NOT_WAIT:
            return AGT_ERROR_MAILBOX_IS_FULL;
          case AGT_WAIT:
            _wait(sender, requiredSize, availableSize);
            break;
          default:
            if (!_wait_for(sender, requiredSize, availableSize, timeout))
              return AGT_ERROR_MAILBOX_IS_FULL;
        }
      }
    }

    agt_u32_t msgOffset = sender->tailOffset;
    agt_u32_t nextOffset = msgOffset + requiredSize;
    if (nextOffset >= sender->bufferLength) [[unlikely]]
      nextOffset -= sender->bufferLength;
    auto msg = _get_msg(sender->messageBuffer, msgOffset);
    msg->msgSize = size;
    msg->nextOffset = nextOffset;
    sender->tailOffset = nextOffset;
    sender->availableSize = availableSize - requiredSize;
    atomicRelaxedIncrement(sender->enqueuedMessageCount);

    *pMsg = _get_msg_buffer(msg);

    return AGT_SUCCESS;
  }

  void         _send_msg(local_sp_ring_queue_sender* sender, void* msgBuffer, agt_async_t* async) noexcept {
    AGT_invariant(msgBuffer != nullptr);

    AGT_api(async_attach, sender->ctx)();
  }
}

#endif//AGATE_CHANNELS_MODULE_RING_QUEUE_HPP
