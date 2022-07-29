//
// Created by maxwe on 2021-11-23.
//

// #include "internal.hpp"
// #include "handle.hpp"

#include "agate2.h"

#include "async/async.hpp"
#include "async/signal.hpp"
#include "context/context.hpp"
#include "core/objects.hpp"
#include "messages/message.hpp"


using namespace agt;


extern "C" {

/*struct AgtContext_st {

  agt_u32_t                processId;

  SharedPageMap            sharedPageMap;
  PageList                 localFreeList;
  ListNode*                emptyLocalPage;

  size_t                  localPageSize;
  size_t                  localEntryCount;

  ObjectPool<SharedHandle, (0x1 << 14) / sizeof(SharedHandle)> handlePool;

  IpcBlock                 ipcBlock;

};*/





}




AGT_noinline static agt_status_t agtGetSharedObjectInfoById(agt_ctx_t context, agt_object_info_t* pObjectInfo) noexcept;


extern "C" {







AGT_api agt_status_t     AGT_stdcall agtNewContext(agt_ctx_t* pContext) AGT_noexcept {
  if (!pContext)
    return AGT_ERROR_INVALID_ARGUMENT;
  return createCtx(*pContext);
}

AGT_api agt_status_t     AGT_stdcall agtDestroyContext(agt_ctx_t context) AGT_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}


AGT_api agt_status_t     AGT_stdcall agtGetObjectInfo(agt_ctx_t context, agt_object_info_t* pObjectInfo) AGT_noexcept {
  if (!pObjectInfo) [[unlikely]] {
    return AGT_ERROR_INVALID_ARGUMENT;
  }

  if (pObjectInfo->id != 0) {

    if (!context) [[unlikely]] {
      return AGT_ERROR_INVALID_ARGUMENT;
    }

    Agt::Id id = Agt::Id::convert(pObjectInfo->id);

    if (!id.isShared()) [[likely]] {
      if (id.getProcessId() != ctxGetProcessId(context)) {
        return AGT_ERROR_UNKNOWN_FOREIGN_OBJECT;
      }
    }
    else {
      return agtGetSharedObjectInfoById(context, pObjectInfo);
    }

    LocalObject* object;

    if (auto objectHeader = ctxLookupId(context, id))
      object = reinterpret_cast<LocalObject*>(objectHeader);
    else
      return AGT_ERROR_EXPIRED_OBJECT_ID;

    pObjectInfo->handle = object;
    pObjectInfo->exportId = AGT_INVALID_OBJECT_ID;
    pObjectInfo->flags = toHandleFlags(object->flags);
    pObjectInfo->type = toHandleType(object->type);

  } else if (pObjectInfo->handle != nullptr)  {
    Handle handle = Handle::wrap(pObjectInfo->handle);

    pObjectInfo->type     = toHandleType(handle.getObjectType());
    pObjectInfo->flags    = toHandleFlags(handle.getObjectFlags());
    pObjectInfo->id       = handle.getLocalId().toRaw();
    pObjectInfo->exportId = handle.getExportId().toRaw();

  } else [[unlikely]] {
    return AGT_ERROR_INVALID_OBJECT_ID;
  }

  return AGT_SUCCESS;

}

AGT_api agt_status_t     AGT_stdcall agtDuplicateHandle(agt_handle_t inHandle, agt_handle_t* pOutHandle) AGT_noexcept {
  if (!pOutHandle) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  if (inHandle == AGT_NULL_HANDLE) [[unlikely]] {
    *pOutHandle = AGT_NULL_HANDLE;
    return AGT_ERROR_NULL_HANDLE;
  }
  return static_cast<Handle*>(inHandle)->duplicate(reinterpret_cast<Handle*&>(*pOutHandle));
}

AGT_api void          AGT_stdcall agtCloseHandle(agt_handle_t handle) AGT_noexcept {
  if (handle) [[likely]]
    static_cast<Handle*>(handle)->close();
}



AGT_api agt_status_t     AGT_stdcall agtCreateChannel(agt_ctx_t context, const agt_channel_create_info_t* cpCreateInfo, agt_handle_t* pSender, agt_handle_t* pReceiver) AGT_noexcept;
AGT_api agt_status_t     AGT_stdcall agtCreateAgent(agt_ctx_t context, const agt_agent_create_info_t* cpCreateInfo, agt_handle_t* pAgent) AGT_noexcept;
AGT_api agt_status_t     AGT_stdcall agtCreateAgency(agt_ctx_t context, const agt_agency_create_info_t* cpCreateInfo, agt_handle_t* pAgency) AGT_noexcept;
AGT_api agt_status_t     AGT_stdcall agtCreateThread(agt_ctx_t context, const agt_thread_create_info_t* cpCreateInfo, agt_handle_t* pThread) AGT_noexcept;

AGT_api agt_status_t     AGT_stdcall agtStage(agt_handle_t sender, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) AGT_noexcept {
  if (sender == AGT_NULL_HANDLE) [[unlikely]]
    return AGT_ERROR_NULL_HANDLE;
  if (pStagedMessage == nullptr) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  return static_cast<Handle*>(sender)->stage(*pStagedMessage, timeout);
}
AGT_api void          AGT_stdcall agtSend(const agt_staged_message_t* cpStagedMessage, agt_async_t asyncHandle, agt_send_flags_t flags) AGT_noexcept {
  const auto& stagedMsg = reinterpret_cast<const StagedMessage&>(*cpStagedMessage);
  const auto message = stagedMsg.message;
  setMessageId(message, stagedMsg.id);
  setMessageReturnHandle(message, stagedMsg.returnHandle);
  setMessageAsyncHandle(message, asyncHandle);
  stagedMsg.receiver->send(stagedMsg.message, flags);
}
AGT_api agt_status_t     AGT_stdcall agtReceive(agt_handle_t receiver, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) AGT_noexcept {
  if (receiver == AGT_NULL_HANDLE) [[unlikely]]
    return AGT_ERROR_NULL_HANDLE;
  if (pMessageInfo == nullptr) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  return static_cast<Handle*>(receiver)->receive(*pMessageInfo, timeout);
}




AGT_api void          AGT_stdcall agtReturn(agt_message_t message, agt_status_t status) AGT_noexcept {

}


AGT_api void          AGT_stdcall agtYieldExecution() AGT_noexcept;

AGT_api agt_status_t     AGT_stdcall agtDispatchMessage(const agt_actor_t* pActor, const agt_message_info_t* pMessageInfo) AGT_noexcept;
// AGT_api agt_status_t     AGT_stdcall agtExecuteOnThread(agt_thread_t thread, ) AGT_noexcept;





AGT_api agt_status_t     AGT_stdcall agtGetSenderHandle(agt_message_t message, agt_handle_t* pSenderHandle) AGT_noexcept;




/* ========================= [ Async ] ========================= */

AGT_api agt_status_t     AGT_stdcall agtNewAsync(agt_ctx_t ctx, agt_async_t* pAsync) AGT_noexcept {

  if (!pAsync) [[unlikely]] {
    return AGT_ERROR_INVALID_ARGUMENT;
  }

  // TODO: Detect bad context?

  *pAsync = createAsync(ctx);

  return AGT_SUCCESS;
}

AGT_api void          AGT_stdcall agtCopyAsync(agt_async_t from, agt_async_t to) AGT_noexcept {
  asyncCopyTo(from, to);
}

AGT_api void          AGT_stdcall agtClearAsync(agt_async_t async) AGT_noexcept {
  asyncClear(async);
}

AGT_api void          AGT_stdcall agtDestroyAsync(agt_async_t async) AGT_noexcept {
  asyncDestroy(async);
}

AGT_api agt_status_t     AGT_stdcall agtWait(agt_async_t async, agt_timeout_t timeout) AGT_noexcept {
  return asyncWait(async, timeout);
}

AGT_api agt_status_t     AGT_stdcall agtWaitMany(const agt_async_t* pAsyncs, size_t asyncCount, agt_timeout_t timeout) AGT_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}




/* ========================= [ Signal ] ========================= */

AGT_api agt_status_t     AGT_stdcall agtNewSignal(agt_ctx_t ctx, agt_signal_t* pSignal) AGT_noexcept {

}

AGT_api void          AGT_stdcall agtAttachSignal(agt_signal_t signal, agt_async_t async) AGT_noexcept {
  signalAttach(signal, async);
}

AGT_api void          AGT_stdcall agtRaiseSignal(agt_signal_t signal) AGT_noexcept {
  signalRaise(signal);
}

AGT_api void          AGT_stdcall agtRaiseManySignals(agt_signal_t signal) AGT_noexcept {

}

AGT_api void          AGT_stdcall agtDestroySignal(agt_signal_t signal) AGT_noexcept {

}


}