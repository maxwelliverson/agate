//
// Created by maxwe on 2022-02-19.
//

#include "thread.hpp"
#include "agate/error.hpp"
#include "channels/channel.hpp"
#include "core/instance.hpp"


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <processthreadsapi.h>


using namespace agt;

namespace {

  /** ========================= [ Thread Data ] =============================== */

  struct local_blocking_thread_data_header {
    local_blocking_thread*               threadHandle;
    local_mpsc_channel*                  channel;
    agt_blocking_thread_message_proc_t msgProc;
  };

  struct local_blocking_thread_inline_data : local_blocking_thread_data_header {
    size_t                           structSize;
    std::byte                        userData[];
  };

  struct local_blocking_thread_out_of_line_data : local_blocking_thread_data_header {
    agt_user_data_dtor_t userDataDtor;
    void*                userData;
  };

  struct shared_blocking_thread_inline_data {
    shared_blocking_thread*            threadHandle;
    shared_mpsc_channel_receiver*      channel;
    agt_blocking_thread_message_proc_t msgProc;
    size_t                             structSize;
    std::byte                          userData[];
  };



  struct shared_blocking_thread_out_of_line_data {
    shared_blocking_thread*            threadHandle;
    shared_mpsc_channel_receiver*      channel;
    agt_blocking_thread_message_proc_t msgProc;
    agt_user_data_dtor_t               userDataDtor;
    void*                              userData;
  };



  /** ========================= [ Thread Cleanup ] =============================== */

  struct thread_cleanup_callback {
    thread_cleanup_callback* pNext;
    void (* pfnCleanupFunction )(void* pUserData);
    void* pUserData;
  };

  class thread_cleanup_callback_stack {
    thread_cleanup_callback* listHead = nullptr;

  public:

    constexpr thread_cleanup_callback_stack() = default;

    ~thread_cleanup_callback_stack() {
      thread_cleanup_callback* currentCallback = listHead;
      thread_cleanup_callback* nextCallback    = nullptr;
      while (currentCallback) {
        nextCallback = currentCallback->pNext;
        currentCallback->pfnCleanupFunction(currentCallback->pUserData);
        delete currentCallback;
        currentCallback = nextCallback;
      }
    }

    void pushCallback(void(*pfnFunc)(void*), void* pUserData) noexcept {
      auto callbackFrame = new thread_cleanup_callback{
        .pNext              = listHead,
        .pfnCleanupFunction = pfnFunc,
        .pUserData          = pUserData
      };
      listHead = callbackFrame;
    }
  };

  constinit thread_local thread_cleanup_callback_stack ThreadCleanupCallbacks{};


  inline void callOnThreadExit(void(* pfnFunc)(void*), void* pUserData) noexcept {
    ThreadCleanupCallbacks.pushCallback(pfnFunc, pUserData);
  }


  template <typename Data>
  void threadDataDestructor(void* pData);


  template <>
  void threadDataDestructor<local_blocking_thread_inline_data>(void* pData) {
    const auto data = static_cast<local_blocking_thread_inline_data*>(pData);
    agt_ctx_t context = data->threadHandle->context;

    handleSetErrorState(data->channel, error_state::noReceivers);
    handleReleaseRef(data->channel);
    handleReleaseRef(data->threadHandle);

    ctxLocalFree(context, data, data->structSize, alignof(local_blocking_thread_inline_data));
  }
  template <>
  void threadDataDestructor<shared_blocking_thread_inline_data>(void* pData) {
    const auto data = static_cast<shared_blocking_thread_inline_data*>(pData);
    agt_ctx_t context = data->threadHandle->context;

    handleSetErrorState(data->channel, error_state::noReceivers);
    handleReleaseRef(data->channel);
    handleReleaseRef(data->threadHandle);

    ctxLocalFree(context, data, data->structSize, alignof(shared_blocking_thread_inline_data));
  }
  template <>
  void threadDataDestructor<local_blocking_thread_out_of_line_data>(void* pData) {
    const auto data = static_cast<local_blocking_thread_out_of_line_data*>(pData);
    agt_ctx_t context = data->threadHandle->context;

    handleSetErrorState(data->channel, error_state::noReceivers);
    handleReleaseRef(data->channel);
    handleReleaseRef(data->threadHandle);

    if (data->userDataDtor)
      data->userDataDtor(data->userData);

    ctxLocalFree(context, data,
                 sizeof(local_blocking_thread_out_of_line_data),
                 alignof(local_blocking_thread_out_of_line_data));
  }
  template <>
  void threadDataDestructor<shared_blocking_thread_out_of_line_data>(void* pData) {
    const auto data = static_cast<shared_blocking_thread_out_of_line_data*>(pData);
    agt_ctx_t context = data->threadHandle->context;

    handleSetErrorState(data->channel, error_state::noReceivers);
    handleReleaseRef(data->channel);
    handleReleaseRef(data->threadHandle);

    if (data->userDataDtor)
      data->userDataDtor(data->userData);

    ctxLocalFree(context, data,
                 sizeof(shared_blocking_thread_out_of_line_data),
                 alignof(shared_blocking_thread_out_of_line_data));
  }



  /** ========================= [ Thread Creation ] =============================== */

#if AGT_system_windows

  using PFN_threadStartRoutine = LPTHREAD_START_ROUTINE;


  DWORD __stdcall localBlockingThreadInlineStartRoutine(LPVOID threadParam) {
    const auto threadData = static_cast<local_blocking_thread_inline_data*>(threadParam);
    const auto threadHandle = threadData->threadHandle;
    const auto channel = threadData->channel;
    const auto msgProc = threadData->msgProc;
    void* const pUserData = threadData->userData;


    callOnThreadExit(threadDataDestructor<local_blocking_thread_inline_data>, threadParam);

    agt_message_info_t messageInfo;
    agt_status_t status;

    do {
      status = agt::handlePopQueue(channel, &messageInfo, AGT_WAIT);
      if ( status == AGT_SUCCESS ) {
        msgProc(threadHandle, &messageInfo, pUserData);
      }
      // TODO: Potential Bug, only handles AGT_SUCCESS and AGT_ERROR_NO_SENDERS. If channel is caught in some other state,
      //       this will either block indefinitely, or will get caught in an infinite loop. This is true for all 4 thread routine functions
    } while( status != AGT_ERROR_NO_SENDERS );

    return NO_ERROR;
  }
  DWORD __stdcall sharedBlockingThreadInlineStartRoutine(LPVOID threadParam) {
    const auto threadData = static_cast<shared_blocking_thread_inline_data*>(threadParam);
    const auto threadHandle = threadData->threadHandle;
    const auto channel = threadData->channel;
    const auto msgProc = threadData->msgProc;
    void* const pUserData = threadData->userData;

    callOnThreadExit(threadDataDestructor<shared_blocking_thread_inline_data>, threadParam);

    agt_message_info_t messageInfo;
    agt_status_t status;

    do {
      status = agt::handlePopQueue(channel, &messageInfo, AGT_WAIT);
      if ( status == AGT_SUCCESS ) {
        msgProc(threadHandle, &messageInfo, pUserData);
      }
    } while( status != AGT_ERROR_NO_SENDERS );

    return NO_ERROR;
  }
  DWORD __stdcall localBlockingThreadOutOfLineStartRoutine(LPVOID threadParam) {
    const auto threadData = static_cast<local_blocking_thread_out_of_line_data*>(threadParam);
    const auto threadHandle = threadData->threadHandle;
    const auto channel = threadData->channel;
    const auto msgProc = threadData->msgProc;
    void* const pUserData = threadData->userData;

    callOnThreadExit(threadDataDestructor<local_blocking_thread_out_of_line_data>, threadParam);

    agt_message_info_t messageInfo;
    agt_status_t status;

    do {
      status = agt::handlePopQueue(channel, &messageInfo, AGT_WAIT);
      if ( status == AGT_SUCCESS ) {
        msgProc(threadHandle, &messageInfo, pUserData);
      }
    } while( status != AGT_ERROR_NO_SENDERS );

    return NO_ERROR;
  }
  DWORD __stdcall sharedBlockingThreadOutOfLineStartRoutine(LPVOID threadParam) {
    const auto threadData = static_cast<shared_blocking_thread_out_of_line_data*>(threadParam);
    const auto threadHandle = threadData->threadHandle;
    const auto channel = threadData->channel;
    const auto msgProc = threadData->msgProc;
    void* const pUserData = threadData->userData;

    callOnThreadExit(threadDataDestructor<shared_blocking_thread_out_of_line_data>, threadParam);

    agt_message_info_t messageInfo;
    agt_status_t status;

    do {
      status = agt::handlePopQueue(channel, &messageInfo, AGT_WAIT);
      if ( status == AGT_SUCCESS ) {
        msgProc(threadHandle, &messageInfo, pUserData);
      }
    } while( status != AGT_ERROR_NO_SENDERS );

    return NO_ERROR;
  }


  agt_status_t createSystemThread(PFN_threadStartRoutine startRoutine, void* data, impl::system_thread& sysThread) noexcept {
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


  agt_status_t createSystemThread(PFN_threadStartRoutine startRoutine, void* data, Impl::SystemThread& sysThread) noexcept {

    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }

#endif
}


agt_status_t agt::createInstance(local_blocking_thread*& handle, agt_ctx_t ctx, const agt_blocking_thread_create_info_t& createInfo) noexcept {

  local_blocking_thread_data_header* header;
  local_mpsc_channel*                channel;
  local_blocking_thread*             threadHandle;

  agt_channel_create_info_t           channelCreateInfo;
  PFN_threadStartRoutine         startRoutine;
  name_token                      nameToken;
  agt_status_t                      status;
  size_t                        structSize;
  size_t                        structAlign;

  if ((status = ctxClaimLocalName(ctx, createInfo.name, createInfo.nameLength, nameToken)) != AGT_SUCCESS)
    return status;

  header       = nullptr;
  channel      = nullptr;
  threadHandle = nullptr;

  if (createInfo.flags & AGT_BLOCKING_THREAD_COPY_USER_DATA) {
    size_t dataLength = (createInfo.flags & AGT_BLOCKING_THREAD_USER_DATA_STRING)
                           ? (strlen((const char*)createInfo.pUserData) + 1)
                           : createInfo.dataSize;
    structSize = dataLength + sizeof(local_blocking_thread_inline_data);
    structAlign = alignof(local_blocking_thread_inline_data);
    if (auto data = (local_blocking_thread_inline_data*)ctxLocalAlloc(ctx, structSize, structAlign)) {
      data->structSize = structSize;
      std::memcpy(data->userData, createInfo.pUserData, dataLength);
      header = data;
      startRoutine = &localBlockingThreadInlineStartRoutine;
    }
  }
  else {
    structSize = sizeof(local_blocking_thread_out_of_line_data);
    structAlign = alignof(local_blocking_thread_out_of_line_data);
    if (auto data = (local_blocking_thread_out_of_line_data*)ctxLocalAlloc(ctx, structSize, structAlign)) {
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


  if (!(threadHandle = allocHandle<local_blocking_thread>(ctx)))
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




agt_status_t Agt::createInstance(SharedBlockingThread*& handle, agt_ctx_t ctx, const agt_blocking_thread_create_info_t& createInfo) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
