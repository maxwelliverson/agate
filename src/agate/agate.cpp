//
// Created by maxwe on 2021-11-23.
//

// #include "internal.hpp"
// #include "handle.hpp"

#include "agate.h"

#include "async/async.hpp"
#include "async/signal.hpp"
#include "context/context.hpp"
#include "core/objects.hpp"
#include "messages/message.hpp"


using namespace Agt;


extern "C" {

/*struct AgtContext_st {

  AgtUInt32                processId;

  SharedPageMap            sharedPageMap;
  PageList                 localFreeList;
  ListNode*                emptyLocalPage;

  AgtSize                  localPageSize;
  AgtSize                  localEntryCount;

  ObjectPool<SharedHandle, (0x1 << 14) / sizeof(SharedHandle)> handlePool;

  IpcBlock                 ipcBlock;

};*/





}




AGT_noinline static AgtStatus agtGetSharedObjectInfoById(AgtContext context, AgtObjectInfo* pObjectInfo) noexcept;


extern "C" {



AGT_api AgtStatus     AGT_stdcall agtNewContext(AgtContext* pContext) AGT_noexcept {
  if (!pContext)
    return AGT_ERROR_INVALID_ARGUMENT;
  return createCtx(*pContext);
}

AGT_api AgtStatus     AGT_stdcall agtDestroyContext(AgtContext context) AGT_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}


AGT_api AgtStatus     AGT_stdcall agtGetObjectInfo(AgtContext context, AgtObjectInfo* pObjectInfo) AGT_noexcept {
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

AGT_api AgtStatus     AGT_stdcall agtDuplicateHandle(AgtHandle inHandle, AgtHandle* pOutHandle) AGT_noexcept {
  if (!pOutHandle) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  if (inHandle == AGT_NULL_HANDLE) [[unlikely]] {
    *pOutHandle = AGT_NULL_HANDLE;
    return AGT_ERROR_NULL_HANDLE;
  }
  return static_cast<Handle*>(inHandle)->duplicate(reinterpret_cast<Handle*&>(*pOutHandle));
}

AGT_api void          AGT_stdcall agtCloseHandle(AgtHandle handle) AGT_noexcept {
  if (handle) [[likely]]
    static_cast<Handle*>(handle)->close();
}



AGT_api AgtStatus     AGT_stdcall agtCreateChannel(AgtContext context, const AgtChannelCreateInfo* cpCreateInfo, AgtHandle* pSender, AgtHandle* pReceiver) AGT_noexcept;
AGT_api AgtStatus     AGT_stdcall agtCreateAgent(AgtContext context, const AgtAgentCreateInfo* cpCreateInfo, AgtHandle* pAgent) AGT_noexcept;
AGT_api AgtStatus     AGT_stdcall agtCreateAgency(AgtContext context, const AgtAgencyCreateInfo* cpCreateInfo, AgtHandle* pAgency) AGT_noexcept;
AGT_api AgtStatus     AGT_stdcall agtCreateThread(AgtContext context, const AgtThreadCreateInfo* cpCreateInfo, AgtHandle* pThread) AGT_noexcept;

AGT_api AgtStatus     AGT_stdcall agtStage(AgtHandle sender, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) AGT_noexcept {
  if (sender == AGT_NULL_HANDLE) [[unlikely]]
    return AGT_ERROR_NULL_HANDLE;
  if (pStagedMessage == nullptr) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  return static_cast<Handle*>(sender)->stage(*pStagedMessage, timeout);
}
AGT_api void          AGT_stdcall agtSend(const AgtStagedMessage* cpStagedMessage, AgtAsync asyncHandle, AgtSendFlags flags) AGT_noexcept {
  const auto& stagedMsg = reinterpret_cast<const StagedMessage&>(*cpStagedMessage);
  const auto message = stagedMsg.message;
  setMessageId(message, stagedMsg.id);
  setMessageReturnHandle(message, stagedMsg.returnHandle);
  setMessageAsyncHandle(message, asyncHandle);
  stagedMsg.receiver->send(stagedMsg.message, flags);
}
AGT_api AgtStatus     AGT_stdcall agtReceive(AgtHandle receiver, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) AGT_noexcept {
  if (receiver == AGT_NULL_HANDLE) [[unlikely]]
    return AGT_ERROR_NULL_HANDLE;
  if (pMessageInfo == nullptr) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  return static_cast<Handle*>(receiver)->receive(*pMessageInfo, timeout);
}




AGT_api void          AGT_stdcall agtReturn(AgtMessage message, AgtStatus status) AGT_noexcept {

}


AGT_api void          AGT_stdcall agtYieldExecution() AGT_noexcept;

AGT_api AgtStatus     AGT_stdcall agtDispatchMessage(const AgtActor* pActor, const AgtMessageInfo* pMessageInfo) AGT_noexcept;
// AGT_api AgtStatus     AGT_stdcall agtExecuteOnThread(AgtThread thread, ) AGT_noexcept;





AGT_api AgtStatus     AGT_stdcall agtGetSenderHandle(AgtMessage message, AgtHandle* pSenderHandle) AGT_noexcept;




/* ========================= [ Async ] ========================= */

AGT_api AgtStatus     AGT_stdcall agtNewAsync(AgtContext ctx, AgtAsync* pAsync) AGT_noexcept {

  if (!pAsync) [[unlikely]] {
    return AGT_ERROR_INVALID_ARGUMENT;
  }

  // TODO: Detect bad context?

  *pAsync = createAsync(ctx);

  return AGT_SUCCESS;
}

AGT_api void          AGT_stdcall agtCopyAsync(AgtAsync from, AgtAsync to) AGT_noexcept {
  asyncCopyTo(from, to);
}

AGT_api void          AGT_stdcall agtClearAsync(AgtAsync async) AGT_noexcept {
  asyncClear(async);
}

AGT_api void          AGT_stdcall agtDestroyAsync(AgtAsync async) AGT_noexcept {
  asyncDestroy(async);
}

AGT_api AgtStatus     AGT_stdcall agtWait(AgtAsync async, AgtTimeout timeout) AGT_noexcept {
  return asyncWait(async, timeout);
}

AGT_api AgtStatus     AGT_stdcall agtWaitMany(const AgtAsync* pAsyncs, AgtSize asyncCount, AgtTimeout timeout) AGT_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}




/* ========================= [ Signal ] ========================= */

AGT_api AgtStatus     AGT_stdcall agtNewSignal(AgtContext ctx, AgtSignal* pSignal) AGT_noexcept {

}

AGT_api void          AGT_stdcall agtAttachSignal(AgtSignal signal, AgtAsync async) AGT_noexcept {
  signalAttach(signal, async);
}

AGT_api void          AGT_stdcall agtRaiseSignal(AgtSignal signal) AGT_noexcept {
  signalRaise(signal);
}

AGT_api void          AGT_stdcall agtRaiseManySignals(AgtSignal signal) AGT_noexcept {

}

AGT_api void          AGT_stdcall agtDestroySignal(AgtSignal signal) AGT_noexcept {

}


}