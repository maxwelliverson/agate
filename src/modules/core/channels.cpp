//
// Created by maxwe on 2024-03-18.
//

#include "core/msg/message_queue.hpp"
#include "core/async.hpp"
#include "core/attr.hpp"


agt_status_t     agt::open_private_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept {
  if (!pSender || !pReceiver)
    return AGT_ERROR_INVALID_ARGUMENT;

  if (channelDesc.name != AGT_ANONYMOUS)
    return AGT_ERROR_NOT_YET_IMPLEMENTED;

  auto sender   = alloc<private_sender>(ctx);
  auto receiver = alloc<private_receiver>(ctx);

  receiver->refCount = 1;
  receiver->head    = nullptr;
  receiver->tail     = &sender->tail;
  receiver->sender   = sender;

  sender->refCount = 1;
  sender->tail     = &receiver->head;
  sender->receiver = receiver;
  sender->maxReceivers = channelDesc.maxReceivers;
  sender->maxSenders = channelDesc.maxSenders;

  *pSender   = reinterpret_cast<agt_sender_t>(sender);
  *pReceiver = reinterpret_cast<agt_receiver_t>(receiver);

  return AGT_SUCCESS;
}
agt_status_t  agt::open_local_spsc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t  agt::open_local_mpsc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t  agt::open_local_spmc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t  agt::open_local_mpmc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::open_shared_spsc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::open_shared_mpsc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::open_shared_spmc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t agt::open_shared_mpmc_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& channelDesc) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}



namespace {

  inline constexpr size_t MsgPoolOffset = 16;

  AGT_forceinline bool is_basic_sender(agt_sender_t sender_) noexcept {
    const auto sender = reinterpret_cast<object*>(sender_);
    AGT_assert_is_type( sender, sender );
    return sender->type <= object_type::basic_sender_end;
  }

  AGT_forceinline bool is_basic_receiver(agt_receiver_t receiver_) noexcept {
    const auto receiver = reinterpret_cast<object*>(receiver_);
    AGT_assert_is_type( receiver, receiver );
    return receiver->type <= object_type::basic_receiver_end;
  }

  AGT_forceinline agt_msg_pool_t get_msg_pool(agt_sender_t sender_) noexcept {
    AGT_invariant( is_basic_sender(sender_) );
    return *reinterpret_cast<agt_msg_pool_t*>(reinterpret_cast<uintptr_t>(sender_) + MsgPoolOffset);
  }

  AGT_forceinline agt_msg_pool_t get_msg_pool(agt_receiver_t receiver_) noexcept {
    AGT_invariant( is_basic_receiver(receiver_) );
    return *reinterpret_cast<agt_msg_pool_t*>(reinterpret_cast<uintptr_t>(receiver_) + MsgPoolOffset);
  }

  inline constexpr size_t ChannelMsgSize = 32;

  AGT_forceinline agt_message_t get_message(void* msg) noexcept {
    return reinterpret_cast<agt_message_t>(static_cast<char*>(msg) - ChannelMsgSize);
  }

}


struct local_channel_async_info {
  agt_bool_t   complete;
  agt_bool_t   bleh;
  agt_u64_t    value;
};

static agt_status_t resolve_channel_callback(void* obj, agt_u64_t* pResult, void* callbackData) noexcept {
  (void)callbackData;

  AGT_invariant( obj != nullptr );
  auto& info = *static_cast<local_channel_async_info*>(obj);
  if (atomicLoad(info.complete) == AGT_TRUE) {
    if (pResult)
      *pResult = info.value;
    return AGT_SUCCESS;
  }

  return AGT_NOT_READY;
}



namespace agt {
  agt_status_t AGT_stdcall open_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t* pChannelDesc) AGT_noexcept {
    if (!pChannelDesc)
      return AGT_ERROR_INVALID_ARGUMENT;

    if (ctx == AGT_CURRENT_CTX) {
      ctx = get_ctx();
      if (ctx == nullptr) [[unlikely]]
        return AGT_ERROR_CTX_NOT_ACQUIRED;
    }

    using pfn_open_channel_t = agt_status_t(*)(agt_ctx_t, agt_sender_t*, agt_receiver_t*, const agt_channel_desc_t&) noexcept;

    constexpr static pfn_open_channel_t JmpTable[] = {
      &open_local_spsc_channel,
      &open_local_mpsc_channel,
      &open_local_spmc_channel,
      &open_local_mpmc_channel,
      &open_shared_spsc_channel,
      &open_shared_mpsc_channel,
      &open_shared_spmc_channel,
      &open_shared_mpmc_channel,
    };

    if (pChannelDesc->scope == AGT_CTX_SCOPE)
      return open_private_channel(ctx, pSender, pReceiver, *pChannelDesc); // Do this one seperately


    const bool scopeIsShared = pChannelDesc->scope == AGT_SHARED_SCOPE;

    if (!attr::shared_context(ctx->instance) && scopeIsShared)
      return AGT_ERROR_CANNOT_CREATE_SHARED;

    size_t index = 0;
    index |= (pChannelDesc->maxSenders != 1 ? 0x1 : 0x0);
    index |= (pChannelDesc->maxReceivers != 1 ? 0x2 : 0x0);
    index |= (scopeIsShared ? 0x4 : 0x0);

    assert( (index & ~0x7) == 0 ); // index must be between 0 and 7.

    return (*JmpTable[index])(ctx, pSender, pReceiver, *pChannelDesc);
  }


  agt_status_t AGT_stdcall acquire_msg_local(agt_sender_t sender, size_t desiredMessageSize, void** ppMsgBuffer) AGT_noexcept {
    // Unless sender is a special kind of sender (like a broadcast queue or a ring queue), we should simply allocate a message from the sender ctx, which should be kept at a constant offset.
    // return AGT_ERROR_NOT_YET_IMPLEMENTED;
    if (is_basic_sender(sender)) {
      auto msgPool = get_msg_pool(sender);
      size_t requiredSize = ChannelMsgSize + desiredMessageSize;
      auto msg = acquire_msg(msgPool, requiredSize);
      // TODO: Figure out a way to return error if msgPool does not support the requiredSize.
      msg->extraData = static_cast<uint32_t>(desiredMessageSize);
      *ppMsgBuffer = reinterpret_cast<char*>(msg) + ChannelMsgSize;
      return AGT_SUCCESS;
    }

    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }


  agt_status_t AGT_stdcall send_msg_local(agt_sender_t sender, void* msgBuffer, size_t size, agt_async_t async) AGT_noexcept {
    agt_ctx_t                ctx       = nullptr;
    local_async_data*        asyncData = nullptr;

    auto msg = get_message(msgBuffer);
    if (msg->extraData > size)
      return AGT_ERROR_MESSAGE_TOO_LARGE;

    msg->extraData = static_cast<agt_u32_t>(size);


    if (async != AGT_FORGET) {
      if (async == AGT_SYNCHRONIZE) {
        ctx = get_ctx();
        asyncData = alloc_local_async_data(ctx, 1); // allocate new async data
      }
      else {
        asyncData = local_async_bind(*static_cast<agt::async*>(async), 1);
      }

      msg->asyncData = to_handle(asyncData);
      msg->asyncKey  = get_local_async_key(asyncData);
    }

    auto status = agt::send(sender, reinterpret_cast<agt_message_t>(msg));

    if (async == AGT_SYNCHRONIZE) {
      AGT_invariant( ctx != nullptr );
      AGT_invariant( asyncData != nullptr );
      if (status == AGT_SUCCESS) [[likely]]
        status = wait_for_local_async(ctx, nullptr, to_handle(asyncData), 0, resolve_channel_callback, nullptr);
      drop_local_async_data(to_handle(asyncData), true);
    }

    return status;
  }


  agt_status_t AGT_stdcall receive_msg_local(agt_receiver_t receiver, void** pMsgBuffer, size_t* pMsgSize, agt_timeout_t timeout) AGT_noexcept {
    if (!pMsgBuffer || !pMsgSize) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;
    agt_message_t msg;
    auto status = agt::receive(static_cast<receiver_t>(receiver), msg, timeout);
    if (status == AGT_SUCCESS) {
      *pMsgBuffer = reinterpret_cast<char*>(msg) + ChannelMsgSize;
      *pMsgSize   = msg->extraData;
    }
    return status;
  }


  void         AGT_stdcall retire_msg_local(agt_receiver_t receiver, void* msgBuffer, agt_u64_t response) AGT_noexcept {
    auto msg = get_message(msgBuffer);

    if (async_data_is_attached(msg->asyncData)) {

      if (auto obj = try_acquire_local_async(msg->asyncData, msg->asyncKey)) {
        auto& data = *static_cast<local_channel_async_info*>(obj);
        data.value = response;
        atomicStore(data.complete, AGT_TRUE);
        wake_local_async(msg->asyncData, &data.complete);
        release_local_async(msg->asyncData, false);
      }
    }

    if (is_basic_receiver(receiver)) {
      release_msg(get_msg_pool(receiver), msg);
    }
    else {
      // TODO: Implement retire op for non-basic receivers (ring queues and broadcast queues, etc)
    }
  }
}