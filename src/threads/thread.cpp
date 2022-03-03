//
// Created by maxwe on 2022-02-19.
//

#include "thread.hpp"
#include "../channels/channel.hpp"
#include "../context/context.hpp"
#include "../support/error.hpp"


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <processthreadsapi.h>


using namespace Agt;

namespace {

  /** ========================= [ Thread Data ] =============================== */

  struct LocalBlockingThreadDataHeader {
    LocalBlockingThread*             threadHandle;
    LocalMpScChannel*                channel;
    PFN_agtBlockingThreadMessageProc msgProc;
  };

  struct LocalBlockingThreadInlineData : LocalBlockingThreadDataHeader {
    AgtSize                          structSize;
    std::byte                        userData[];
  };

  struct LocalBlockingThreadOutOfLineData  : LocalBlockingThreadDataHeader {
    PFN_agtUserDataDtor              userDataDtor;
    void*                            userData;
  };

  struct SharedBlockingThreadInlineData {
    SharedBlockingThread*      threadHandle;
    SharedMpScChannelReceiver* channel;
    PFN_agtBlockingThreadMessageProc msgProc;
    AgtSize                    structSize;
    std::byte                  userData[];
  };



  struct SharedBlockingThreadOutOfLineData {
    SharedBlockingThread*            threadHandle;
    SharedMpScChannelReceiver*       channel;
    PFN_agtBlockingThreadMessageProc msgProc;
    PFN_agtUserDataDtor              userDataDtor;
    void*                            userData;
  };



  /** ========================= [ Thread Cleanup ] =============================== */

  struct ThreadCleanupCallback {
    ThreadCleanupCallback* pNext;
    void (* pfnCleanupFunction )(void* pUserData);
    void* pUserData;
  };

  class ThreadCleanupCallbackStack {
    ThreadCleanupCallback* listHead = nullptr;

  public:

    constexpr ThreadCleanupCallbackStack() = default;

    ~ThreadCleanupCallbackStack() {
      ThreadCleanupCallback* currentCallback = listHead;
      ThreadCleanupCallback* nextCallback    = nullptr;
      while (currentCallback) {
        nextCallback = currentCallback->pNext;
        currentCallback->pfnCleanupFunction(currentCallback->pUserData);
        delete currentCallback;
        currentCallback = nextCallback;
      }
    }

    void pushCallback(void(*pfnFunc)(void*), void* pUserData) noexcept {
      auto callbackFrame = new ThreadCleanupCallback{
        .pNext              = listHead,
        .pfnCleanupFunction = pfnFunc,
        .pUserData          = pUserData
      };
      listHead = callbackFrame;
    }
  };

  constinit thread_local ThreadCleanupCallbackStack ThreadCleanupCallbacks{};


  inline void callOnThreadExit(void(* pfnFunc)(void*), void* pUserData) noexcept {
    ThreadCleanupCallbacks.pushCallback(pfnFunc, pUserData);
  }


  template <typename Data>
  void threadDataDestructor(void* pData);


  template <>
  void threadDataDestructor<LocalBlockingThreadInlineData>(void* pData) {
    const auto data = static_cast<LocalBlockingThreadInlineData*>(pData);
    AgtContext context = data->threadHandle->context;

    handleSetErrorState(data->channel, ErrorState::noReceivers);
    handleReleaseRef(data->channel);
    handleReleaseRef(data->threadHandle);

    ctxLocalFree(context, data, data->structSize, alignof(LocalBlockingThreadInlineData));
  }
  template <>
  void threadDataDestructor<SharedBlockingThreadInlineData>(void* pData) {
    const auto data = static_cast<SharedBlockingThreadInlineData*>(pData);
    AgtContext context = data->threadHandle->context;

    handleSetErrorState(data->channel, ErrorState::noReceivers);
    handleReleaseRef(data->channel);
    handleReleaseRef(data->threadHandle);

    ctxLocalFree(context, data, data->structSize, alignof(SharedBlockingThreadInlineData));
  }
  template <>
  void threadDataDestructor<LocalBlockingThreadOutOfLineData>(void* pData) {
    const auto data = static_cast<LocalBlockingThreadOutOfLineData*>(pData);
    AgtContext context = data->threadHandle->context;

    handleSetErrorState(data->channel, ErrorState::noReceivers);
    handleReleaseRef(data->channel);
    handleReleaseRef(data->threadHandle);

    if (data->userDataDtor)
      data->userDataDtor(data->userData);

    ctxLocalFree(context, data,
                 sizeof(LocalBlockingThreadOutOfLineData),
                 alignof(LocalBlockingThreadOutOfLineData));
  }
  template <>
  void threadDataDestructor<SharedBlockingThreadOutOfLineData>(void* pData) {
    const auto data = static_cast<SharedBlockingThreadOutOfLineData*>(pData);
    AgtContext context = data->threadHandle->context;

    handleSetErrorState(data->channel, ErrorState::noReceivers);
    handleReleaseRef(data->channel);
    handleReleaseRef(data->threadHandle);

    if (data->userDataDtor)
      data->userDataDtor(data->userData);

    ctxLocalFree(context, data,
                 sizeof(SharedBlockingThreadOutOfLineData),
                 alignof(SharedBlockingThreadOutOfLineData));
  }



  /** ========================= [ Thread Creation ] =============================== */

#if AGT_system_windows

  using PFN_threadStartRoutine = LPTHREAD_START_ROUTINE;


  DWORD __stdcall localBlockingThreadInlineStartRoutine(LPVOID threadParam) {
    const auto threadData = static_cast<LocalBlockingThreadInlineData*>(threadParam);
    const auto threadHandle = threadData->threadHandle;
    const auto channel = threadData->channel;
    const auto msgProc = threadData->msgProc;
    void* const pUserData = threadData->userData;


    callOnThreadExit(threadDataDestructor<LocalBlockingThreadInlineData>, threadParam);

    AgtMessageInfo messageInfo;
    AgtStatus status;

    do {
      status = Agt::handlePopQueue(channel, &messageInfo, AGT_WAIT);
      if ( status == AGT_SUCCESS ) {
        msgProc(threadHandle, &messageInfo, pUserData);
      }
      // TODO: Potential Bug, only handles AGT_SUCCESS and AGT_ERROR_NO_SENDERS. If channel is caught in some other state,
      //       this will either block indefinitely, or will get caught in an infinite loop. This is true for all 4 thread routine functions
    } while( status != AGT_ERROR_NO_SENDERS );

    return NO_ERROR;
  }
  DWORD __stdcall sharedBlockingThreadInlineStartRoutine(LPVOID threadParam) {
    const auto threadData = static_cast<SharedBlockingThreadInlineData*>(threadParam);
    const auto threadHandle = threadData->threadHandle;
    const auto channel = threadData->channel;
    const auto msgProc = threadData->msgProc;
    void* const pUserData = threadData->userData;

    callOnThreadExit(threadDataDestructor<SharedBlockingThreadInlineData>, threadParam);

    AgtMessageInfo messageInfo;
    AgtStatus status;

    do {
      status = Agt::handlePopQueue(channel, &messageInfo, AGT_WAIT);
      if ( status == AGT_SUCCESS ) {
        msgProc(threadHandle, &messageInfo, pUserData);
      }
    } while( status != AGT_ERROR_NO_SENDERS );

    return NO_ERROR;
  }
  DWORD __stdcall localBlockingThreadOutOfLineStartRoutine(LPVOID threadParam) {
    const auto threadData = static_cast<LocalBlockingThreadOutOfLineData*>(threadParam);
    const auto threadHandle = threadData->threadHandle;
    const auto channel = threadData->channel;
    const auto msgProc = threadData->msgProc;
    void* const pUserData = threadData->userData;

    callOnThreadExit(threadDataDestructor<LocalBlockingThreadOutOfLineData>, threadParam);

    AgtMessageInfo messageInfo;
    AgtStatus status;

    do {
      status = Agt::handlePopQueue(channel, &messageInfo, AGT_WAIT);
      if ( status == AGT_SUCCESS ) {
        msgProc(threadHandle, &messageInfo, pUserData);
      }
    } while( status != AGT_ERROR_NO_SENDERS );

    return NO_ERROR;
  }
  DWORD __stdcall sharedBlockingThreadOutOfLineStartRoutine(LPVOID threadParam) {
    const auto threadData = static_cast<SharedBlockingThreadOutOfLineData*>(threadParam);
    const auto threadHandle = threadData->threadHandle;
    const auto channel = threadData->channel;
    const auto msgProc = threadData->msgProc;
    void* const pUserData = threadData->userData;

    callOnThreadExit(threadDataDestructor<SharedBlockingThreadOutOfLineData>, threadParam);

    AgtMessageInfo messageInfo;
    AgtStatus status;

    do {
      status = Agt::handlePopQueue(channel, &messageInfo, AGT_WAIT);
      if ( status == AGT_SUCCESS ) {
        msgProc(threadHandle, &messageInfo, pUserData);
      }
    } while( status != AGT_ERROR_NO_SENDERS );

    return NO_ERROR;
  }


  AgtStatus createSystemThread(PFN_threadStartRoutine startRoutine, void* data, Impl::SystemThread& sysThread) noexcept {
    SECURITY_ATTRIBUTES securityAttributes;
    securityAttributes.bInheritHandle = false;
    securityAttributes.lpSecurityDescriptor = nullptr;
    securityAttributes.nLength = sizeof(securityAttributes);
    DWORD threadId;
    HANDLE threadHandle = CreateThread(&securityAttributes, 0, startRoutine, data, 0, &threadId);

    if (!threadHandle) {
      // TODO: Error checking
      return AGT_ERROR_UNKNOWN;
    }

    sysThread.handle_ = threadHandle;
    sysThread.id_     = (int)threadId;

    return AGT_SUCCESS;
  }

#else


  using PFN_threadStartRoutine = void*(*)(void*);


  AgtStatus createSystemThread(PFN_threadStartRoutine startRoutine, void* data, Impl::SystemThread& sysThread) noexcept {

    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }

#endif
}


AgtStatus Agt::createInstance(LocalBlockingThread*& handle, AgtContext ctx, const AgtBlockingThreadCreateInfo& createInfo) noexcept {

  LocalBlockingThreadDataHeader* header;
  LocalMpScChannel*              channel;
  LocalBlockingThread*           threadHandle;

  AgtChannelCreateInfo           channelCreateInfo;
  PFN_threadStartRoutine         startRoutine;
  NameToken                      nameToken;
  AgtStatus                      status;
  AgtSize                        structSize;
  AgtSize                        structAlign;

  if ((status = ctxClaimLocalName(ctx, createInfo.name, createInfo.nameLength, nameToken)) != AGT_SUCCESS)
    return status;

  header       = nullptr;
  channel      = nullptr;
  threadHandle = nullptr;

  if (createInfo.flags & AGT_BLOCKING_THREAD_COPY_USER_DATA) {
    AgtSize dataLength = (createInfo.flags & AGT_BLOCKING_THREAD_USER_DATA_STRING)
                           ? (strlen((const char*)createInfo.pUserData) + 1)
                           : createInfo.dataSize;
    structSize = dataLength + sizeof(LocalBlockingThreadInlineData);
    structAlign = alignof(LocalBlockingThreadInlineData);
    if (auto data = (LocalBlockingThreadInlineData*)ctxLocalAlloc(ctx, structSize, structAlign)) {
      data->structSize = structSize;
      std::memcpy(data->userData, createInfo.pUserData, dataLength);
      header = data;
      startRoutine = &localBlockingThreadInlineStartRoutine;
    }
  }
  else {
    structSize = sizeof(LocalBlockingThreadOutOfLineData);
    structAlign = alignof(LocalBlockingThreadInlineData);
    if (auto data = (LocalBlockingThreadOutOfLineData*)ctxLocalAlloc(ctx, structSize, structAlign)) {
      data->userData = createInfo.pUserData;
      data->userDataDtor = createInfo.pfnUserDataDtor;
      header = data;
      startRoutine = &localBlockingThreadOutOfLineStartRoutine;
    }
  }
  if (!header) {
    status = AGT_ERROR_BAD_ALLOC;
    goto error;
  }


  if (!(threadHandle = allocHandle<LocalBlockingThread>(ctx)))
    goto error;


  channelCreateInfo.scope = AGT_SCOPE_LOCAL;
  channelCreateInfo.maxReceivers = 1;
  channelCreateInfo.maxSenders = createInfo.maxSenders;
  channelCreateInfo.minCapacity = createInfo.minCapacity;
  channelCreateInfo.maxMessageSize = createInfo.maxMessageSize;
  channelCreateInfo.asyncHandle = AGT_SYNCHRONIZE;
  channelCreateInfo.name = nullptr;
  channelCreateInfo.nameLength = 0;



  if ((status = createInstance(channel, ctx, channelCreateInfo, nullptr, nullptr)) != AGT_SUCCESS)
    goto error;

  header->channel = channel;
  header->threadHandle = threadHandle;
  header->msgProc = createInfo.pfnMessageProc;

  if ((status = createSystemThread(startRoutine, header, threadHandle->sysThread)) != AGT_SUCCESS)
    goto error;


  return AGT_SUCCESS;

error:
  ctxReleaseLocalName(ctx, nameToken);
  if (header)
    ctxLocalFree(ctx, header, structSize, structAlign);
  if (channel)
    handleReleaseRef(channel);
  if (threadHandle)
    handleReleaseRef(threadHandle);
  return status;
}

AgtStatus Agt::createInstance(SharedBlockingThread*& handle, AgtContext ctx, const AgtBlockingThreadCreateInfo& createInfo) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
