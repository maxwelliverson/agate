//
// Created by maxwe on 2021-11-23.
//

// #include "internal.hpp"
// #include "handle.hpp"

#define AGT_UNDEFINED_ASYNC_STRUCT

#include "agate.h"

#include "init.hpp"

#include "agate/environment.hpp"
#include "agate/integer_division.hpp"
// #include "agate/module.hpp"
#include "agate/sys_string.hpp"

#include "agate/priority_queue.hpp"

#include "core/async.hpp"
#include "core/attr.hpp"
#include "core/signal.hpp"
#include "channels/message.hpp"
#include "core/instance.hpp"
#include "core/object.hpp"
#include "core/ctx.hpp"
#include "core/fiber.hpp"

#include "agents/agents.hpp"

#include "channels/message_pool.hpp"
#include "channels/message_queue.hpp"




#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma comment(lib, "mincore")

#include <algorithm>
#include <bit>
#include <charconv>
#include <cstring>
#include <memory>
#include <optional>
#include <string_view>






using namespace agt;

/*struct module_info {
  module_id id;
  version   moduleVersion;
  HMODULE   libraryHandle;
  char      moduleName[16];
  wchar_t   libraryPath[260];
};*/

namespace {

  inline export_table g_lib = {};







  const agt::proc_entry* lookupProcSet(const char* procName) noexcept {
    const agt::proc_entry* procSet = g_lib.pProcSet;
    agt_u32_t procSetSize = g_lib.procSetSize;

    if (!procSet)
      return nullptr;

    const size_t procNameLength = std::strlen(procName);

    if (procNameLength > agt::MaxAPINameLength)
      return nullptr;

    auto procSetEnd = procSet + procSetSize;
    auto iter = std::lower_bound(procSet, procSetEnd, procName, [procNameLength](const agt::proc_entry& entry, const char* name) {
      return std::memcmp(&entry.name[0], name, procNameLength) < 0;
    });

    if (iter == procSetEnd || (std::memcmp(&iter->name[0], procName, procNameLength) != 0))
      return nullptr;
    return iter;
  }


  HMODULE get_exe_handle() noexcept {
    return GetModuleHandle(nullptr);
  }

  HMODULE get_dll_handle() noexcept {
    MEMORY_BASIC_INFORMATION info;
    size_t len = VirtualQueryEx(GetCurrentProcess(), (void*)get_dll_handle, &info, sizeof(info));
    if (len != sizeof(info))
      return nullptr;
    return (HMODULE)info.AllocationBase;
  }
}








extern "C" {



  AGT_static_api agt_config_t AGT_stdcall _agt_get_config(agt_config_t parentConfig, int headerVersion) AGT_noexcept {

    HMODULE moduleHandle;
    agt_config_t config;
    agt_config_t baseLoader = parentConfig;

    agt::init::loader_shared *shared;

    if (parentConfig) {
      moduleHandle = get_dll_handle();
      while (baseLoader->parent != AGT_ROOT_CONFIG)
        baseLoader = baseLoader->parent;
      shared = baseLoader->shared;
      auto [modIter, modIsNew] = shared->loaderMap.try_emplace(moduleHandle, nullptr);
      if (!modIsNew)
        return modIter->second;
      config = new agt_config_st{};
      modIter->second = config;
      parentConfig->childLoaders.push_back(config);
    } else {
      moduleHandle = get_exe_handle();
      config = new agt_config_st{};
      shared = new init::loader_shared;
      shared->baseLoader = config;
      shared->requestedModules.insert_or_assign("core", AGT_INIT_REQUIRED);
      shared->loaderMap[moduleHandle] = config;
    }


    config->parent        = parentConfig;
    config->headerVersion = headerVersion;
    config->loaderVersion = AGT_API_VERSION;
    config->loaderSize    = sizeof(agt_config_st);
    config->pExportTable  = &g_lib;
    config->exportTableSize = sizeof(export_table);
    config->moduleHandle  = moduleHandle;
    config->shared        = shared;
    config->pCtxResult    = nullptr;
    config->initCallback  = nullptr;
    config->callbackUserData = nullptr;

    std::memset(config->pExportTable, 0, config->exportTableSize);


    return config;
  }




AGT_static_api agt_proc_t   AGT_stdcall agt_get_proc_address(const char* symbol) AGT_noexcept {
  if (auto result = lookupProcSet(symbol))
    return result->address;
  return nullptr;
}



AGT_static_api agt_bool_t   AGT_stdcall agt_query_attributes(size_t attrCount, const agt_attr_id_t* pAttrId, agt_value_type_t* pTypes, agt_value_t* pVars) AGT_noexcept {
  if (attrCount == 0)
    return AGT_TRUE;

  if (!pAttrId || !pTypes || !pVars)
    return AGT_FALSE;

  const uint32_t maxAttrVal = g_lib.attrCount;
  const auto pAttrValues = g_lib.attrValues;
  const auto pAttrTypes  = g_lib.attrTypes;

  agt_bool_t result = AGT_FALSE;

  for (size_t i = 0; i < attrCount; ++i) {
    const uint32_t attrId = pAttrId[i];
    if (attrId < maxAttrVal) {
      pVars[i].uint64 = pAttrValues[attrId];
      pTypes[i]       = pAttrTypes[attrId];
      result          = AGT_TRUE;
    }
    else
      pTypes[i] = AGT_TYPE_UNKNOWN;
  }

  return result;
}





}




// AGT_noinline static agt_status_t agtGetSharedObjectInfoById(agt_ctx_t context, agt_object_info_t* pObjectInfo) noexcept;


extern "C" {




/** ===================[ Static API ]==================== **/

AGT_static_api agt_status_t        AGT_stdcall agt_query_instance_attributes(agt_instance_t instance, agt_attr_t* pAttributes, size_t attributeCount) noexcept {
  if (!instance) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;

  agt_status_t status = AGT_SUCCESS;

  if (attributeCount) [[likely]] {
    if (!pAttributes) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;



    const auto attrValues = g_lib.attrValues;
    const auto attrTypes  = g_lib.attrTypes;
    const auto attrCount  = g_lib.attrCount;

    for (size_t i = 0; i < attributeCount; ++i) {
      auto& attr = pAttributes[i];
      if (attr.id >= attrCount) [[unlikely]] {
        status    = AGT_ERROR_UNKNOWN_ATTRIBUTE;
        attr.type = AGT_TYPE_UNKNOWN;
        attr.value.uint64 = 0;
      }
      else {
        attr.value.uint64 = attrValues[attr.id];
        attr.type = attrTypes[attr.id];
      }
    }
  }

  return status;

}

AGT_static_api agt_instance_t      AGT_stdcall agt_get_instance(agt_ctx_t context) noexcept {
  return agt::get_instance(context);
}

AGT_core_api agt_ctx_t           AGT_stdcall agt_ctx() noexcept {
  return g_lib._pfn_ctx();
}

AGT_core_api agt_ctx_t           AGT_stdcall agt_acquire_ctx(const agt_allocator_params_t* pAllocParams) noexcept {
  return g_lib._pfn_do_acquire_ctx(g_lib.instance, pAllocParams);
}

AGT_static_api int                 AGT_stdcall agt_get_instance_version(agt_instance_t instance) noexcept {
  return instance->version.to_i32();
}

AGT_static_api agt_error_handler_t AGT_stdcall agt_get_error_handler(agt_instance_t instance) noexcept {
  AGT_assert( instance != AGT_INVALID_INSTANCE );
  return instance->errorHandler;
}

AGT_static_api agt_error_handler_t AGT_stdcall agt_set_error_handler(agt_instance_t instance, agt_error_handler_t errorHandlerCallback) noexcept {
  AGT_assert( instance != AGT_INVALID_INSTANCE );
  auto oldHandler = instance->errorHandler;
  instance->errorHandler = errorHandlerCallback;
  return oldHandler;
}



/** ==========================[ Agents API ]=========================== **/

AGT_core_api agt_status_t AGT_stdcall agt_send(agt_self_t self, agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept {

  assert( self      != nullptr );
  assert( recipient != nullptr );
  assert( pSendInfo != nullptr );

  return g_lib._pfn_send(self, recipient, pSendInfo);
}

AGT_core_api agt_status_t AGT_stdcall agt_send_as(agt_self_t self, agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept {

  assert( self        != nullptr );
  assert( spoofSender != nullptr );
  assert( recipient   != nullptr );
  assert( pSendInfo   != nullptr );


  return g_lib._pfn_send_as(self, spoofSender, recipient, pSendInfo);
}

AGT_core_api agt_status_t AGT_stdcall agt_send_many(agt_self_t self, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept {
  assert( self       != nullptr );
  assert( agentCount == 0 || recipients != nullptr );
  assert( pSendInfo  != nullptr );

  if (agentCount == 0)
    return AGT_SUCCESS; // This is a valid operation.

  return g_lib._pfn_send_many(self, recipients, agentCount, pSendInfo);
}

AGT_core_api agt_status_t AGT_stdcall agt_send_many_as(agt_self_t self, agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept {
  assert( self        != nullptr );
  assert( spoofSender != nullptr );
  assert( agentCount == 0 || recipients != nullptr );
  assert( pSendInfo   != nullptr );

  if (agentCount == 0)
    return AGT_SUCCESS; // This is a valid operation.

  return g_lib._pfn_send_many_as(self, spoofSender, recipients, agentCount, pSendInfo);
}

AGT_core_api agt_status_t AGT_stdcall agt_reply(agt_self_t self, const agt_send_info_t* pSendInfo) AGT_noexcept {
  assert( self      != nullptr );
  assert( pSendInfo != nullptr );

  return g_lib._pfn_reply(self, pSendInfo);
}

AGT_core_api agt_status_t AGT_stdcall agt_reply_as(agt_self_t self, agt_agent_t spoofReplier, const agt_send_info_t* pSendInfo) AGT_noexcept {
  assert( self         != nullptr );
  assert( spoofReplier != nullptr );
  assert( pSendInfo    != nullptr );

  return g_lib._pfn_reply_as(self, spoofReplier, pSendInfo);
}



AGT_core_api agt_status_t AGT_stdcall agt_raw_acquire(agt_self_t self, agt_agent_t recipient, size_t desiredMessageSize, agt_raw_send_info_t* pRawSendInfo, void** ppRawBuffer) AGT_noexcept {
  assert( self        != nullptr );
  assert( recipient   != nullptr );
  assert( ppRawBuffer != nullptr );

  return g_lib._pfn_raw_acquire(self, recipient, desiredMessageSize, pRawSendInfo, ppRawBuffer);
}

AGT_core_api agt_status_t AGT_stdcall agt_raw_send(agt_self_t self, agt_agent_t recipient, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_raw_send_as(agt_self_t self, agt_agent_t spoofSender, agt_agent_t recipient, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_raw_send_many(agt_self_t self, const agt_agent_t* recipients, agt_size_t agentCount, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_raw_send_many_as(agt_self_t self, agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_raw_reply(agt_self_t self, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_raw_reply_as(agt_self_t self, agt_agent_t spoofSender, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;



/* ========================= [ Async ] ========================= */

#define AGT_ASYNC_MEMORY_IS_OWNED 0x8000


/*
AGT_core_api agt_async_t  AGT_stdcall agt_new_async(agt_ctx_t ctx, agt_async_flags_t flags) AGT_noexcept {
  assert( ctx != nullptr );


  auto asyncBase         = g_lib._pfn_alloc_async(ctx);

  assert( asyncBase->structSize >= sizeof(async_base) );

  asyncBase->ctx         = ctx;
  asyncBase->flags       = flags | AGT_ASYNC_MEMORY_IS_OWNED;
  asyncBase->resultValue = 0;
  asyncBase->status      = AGT_ERROR_NOT_BOUND;
  g_lib._pfn_initialize_async(asyncBase);

  return (agt_async_t)asyncBase;
}

AGT_core_api agt_async_t  AGT_stdcall agt_init_async(agt_inline_async_t* pInlineAsync, agt_async_flags_t flags) AGT_noexcept {
  assert( pInlineAsync != nullptr );
  // assert( pInlineAsync->ctx != nullptr );

  auto asyncBase = (async_base*)pInlineAsync;
  if ( asyncBase->ctx == AGT_INVALID_CTX )
    asyncBase->ctx = agt::get_ctx();
  asyncBase->structSize  = static_cast<agt_u32_t>(g_lib.attrValues[AGT_ATTR_ASYNC_STRUCT_SIZE]);
  asyncBase->flags       = flags;
  asyncBase->resultValue = 0;
  asyncBase->status      = AGT_ERROR_NOT_BOUND;


  g_lib._pfn_initialize_async(asyncBase);

  return (agt_async_t)asyncBase;
}

AGT_core_api void         AGT_stdcall agt_copy_async(agt_async_t from_, agt_async_t to_) AGT_noexcept {
  assert( from_ != nullptr );
  assert( to_   != nullptr );

  auto from = (async_base*)from_;
  auto to   = (async_base*)to_;

  g_lib._pfn_copy_async(from, to);
}

AGT_core_api void         AGT_stdcall agt_move_async(agt_async_t from_, agt_async_t to_) AGT_noexcept {
  assert( from_ != nullptr );
  assert( to_   != nullptr );

  auto from = (async_base*)from_;
  auto to   = (async_base*)to_;

  g_lib._pfn_move_async(from, to);



  // if ((from->flags & AGT_ASYNC_MEMORY_IS_OWNED) != 0)
    // g_lib._pfn_free_async(from->ctx, (agt_async_t)from);
}

AGT_core_api void         AGT_stdcall agt_clear_async(agt_async_t async) AGT_noexcept {

}

AGT_core_api void         AGT_stdcall agt_destroy_async(agt_async_t async) AGT_noexcept {
  if (async) {
    const auto asyncBase      = (async_base*)async;
    const auto ctx            = asyncBase->ctx;
    const bool isLibAllocated = (asyncBase->flags & AGT_ASYNC_MEMORY_IS_OWNED) != 0;
    g_lib._pfn_destroy_async(async);
    if (isLibAllocated)
      g_lib._pfn_free_async(ctx, async);
  }
}


AGT_core_api agt_status_t AGT_stdcall agt_async_status(agt_async_t async_, agt_u64_t* pResult) AGT_noexcept {
  assert( async_ != nullptr );

  auto async = (agt::async*)async_;

  agt_status_t status;

  if (test(async->flags, async_flags::eCacheStatus)) {
    if (async->status == AGT_NOT_READY) {
      if ((status = g_lib._pfn_async_get_status(async, pResult)) == AGT_SUCCESS && pResult != nullptr)
        async->resultValue = *pResult;
      async->status = status;
    }
    else {
      status = async->status;
      if (pResult != nullptr)
        *pResult = async->resultValue;
    }
  }
  else
    status = g_lib._pfn_async_get_status(async, pResult);

  return status;
}
AGT_core_api agt_status_t AGT_stdcall agt_async_status_all(const agt_async_t* pAsyncs, agt_size_t asyncCount, agt_status_t* pStatuses, agt_u64_t* pResults) AGT_noexcept {
  assert( asyncCount == 0 || pAsyncs   != nullptr );
  assert( asyncCount == 0 || pStatuses != nullptr );

  if (asyncCount == 0)
    return AGT_SUCCESS;

  size_t successCount = 0;
  size_t errorCount   = 0;

  agt_status_t status;
  agt_u64_t    resultValue;

  for (size_t i = 0; i < asyncCount; ++i) {
    auto async = (async_base*)pAsyncs[i];

    if ((async->flags & AGT_ASYNC_CACHE_STATUS) != 0) {
      if (async->status == AGT_NOT_READY) {
        if ((status = g_lib._pfn_async_get_status(async, &resultValue)) == AGT_SUCCESS)
          async->resultValue = resultValue;
        async->status = status;
      }
      else {
        status = async->status;
        resultValue = async->resultValue;
      }
    }
    else
      status = g_lib._pfn_async_get_status(async, &resultValue);

    pStatuses[i] = status;
    if (pResults)
      pResults[i] = resultValue;

    switch (status) {
      case AGT_SUCCESS:
        ++successCount;
        break;
      case AGT_NOT_READY:
        break;
      default:
        ++errorCount;
    }
  }

  if (successCount == asyncCount)
    return AGT_SUCCESS;
  if (successCount > 0)
    return AGT_PARTIAL_SUCCESS;
  if (errorCount == asyncCount)
    return AGT_FAILURE;
  return AGT_NOT_READY;
}

AGT_core_api agt_status_t AGT_stdcall agt_wait(agt_async_t async, agt_u64_t* pResult, agt_timeout_t timeout) AGT_noexcept;
AGT_core_api agt_status_t AGT_stdcall agt_wait_all(const agt_async_t* pAsyncs, agt_size_t asyncCount, agt_timeout_t timeout) AGT_noexcept;
AGT_core_api agt_status_t AGT_stdcall agt_wait_any(const agt_async_t* pAsyncs, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) AGT_noexcept;
*/


/** ========================= [ Signal ] ========================= **/



AGT_core_api void         AGT_stdcall agt_attach_signal(agt_signal_t signal, agt_async_t async) AGT_noexcept {

}

AGT_core_api void         AGT_stdcall agt_attach_many_signals(const agt_signal_t* pSignals, agt_size_t signalCount, agt_async_t async, size_t waitForN) AGT_noexcept {

}


AGT_core_api void         AGT_stdcall agt_raise_signal(agt_signal_t signal) AGT_noexcept {

}






/** ==== [ Fibers ] ==== **/

/*
AGT_exec_api agt_status_t AGT_stdcall agt_enter_fctx(agt_ctx_t ctx, const agt_fctx_desc_t* pFiberDesc) AGT_noexcept {

  AGT_assert( ctx != nullptr );
  AGT_assert( pFiberDesc != nullptr );
  AGT_assert( pFiberDesc->proc != nullptr );

  if (ctx->boundFiber != nullptr)
    return AGT_ERROR_ALREADY_FIBER;
  if (ctx->boundAgent != nullptr)
    return AGT_ERROR_IN_AGENT_CONTEXT;

  agt::impl::fiber_init_info initInfo {};
  initInfo.proc = pFiberDesc->proc;
  initInfo.userData = pFiberDesc->userData;

  auto parent = pFiberDesc->parentFiber;

  if (parent != nullptr) {
    g_lib._pfn_retain_fiber_pool(ctx, parent->pool);
    initInfo.root = parent->root;
    initInfo.pool = parent->pool;
    initInfo.stateSaveMask = parent->storeStateFlags;
  }
  else {
    size_t stackSize;

    if (pFiberDesc->stackSize != 0) {
      size_t stackAlignment = g_lib.attrValues[AGT_ATTR_STACK_SIZE_ALIGNMENT];
      stackSize = pFiberDesc->stackSize;
      stackSize = align_to(stackSize, stackAlignment);
    }
    else
      stackSize = g_lib.attrValues[AGT_ATTR_DEFAULT_FIBER_STACK_SIZE];

    initInfo.root = nullptr;
    initInfo.pool = g_lib._pfn_new_fiber_pool(ctx, stackSize, pFiberDesc);
    initInfo.stateSaveMask = g_lib.attrValues[AGT_ATTR_FULL_STATE_SAVE_MASK];
  }

  auto status = agt::impl::assembly::afiber_init(ctx, pFiberDesc->flags, pFiberDesc->initialParam, &initInfo);

  g_lib._pfn_release_fiber_pool(ctx, initInfo.pool);

  return status;
}
*/




AGT_core_api agt_status_t AGT_stdcall agt_enter_fctx(agt_ctx_t ctx, const agt_fctx_desc_t* pFCtxDesc, int* pExitCode) AGT_noexcept {
  return g_lib._pfn_enter_fctx(ctx, pFCtxDesc, pExitCode);
}

AGT_core_api void         AGT_stdcall agt_exit_fctx(agt_ctx_t ctx, int exitCode) AGT_noexcept {
  return g_lib._pfn_exit_fctx(ctx, exitCode);
}

AGT_exec_api agt_status_t AGT_stdcall agt_new_fiber(agt_fiber_t* pFiber, agt_fiber_proc_t proc, void* userData) AGT_noexcept {
  /*AGT_assert( ctx != nullptr );
  AGT_assert( pFiber != nullptr );

  if (ctx->boundFiber == nullptr)
    return AGT_ERROR_NO_FIBER_BOUND;

  auto pool = ctx->boundFiber->pool;

  auto status = g_lib._pfn_fiber_pool_alloc(ctx, pool, proc, userData, pFiber);

  if (status == AGT_SUCCESS) {
    g_lib._pfn_retain_fiber_pool(ctx, pool);
    (*pFiber)->pool = pool;
  }

  return status;*/

  return g_lib._pfn_new_fiber(pFiber, proc, userData);

}

AGT_exec_api agt_status_t AGT_stdcall agt_destroy_fiber(agt_fiber_t fiber) AGT_noexcept {
  return g_lib._pfn_destroy_fiber(fiber);
}




AGT_exec_api void*        AGT_stdcall agt_get_fiber_data(agt_fiber_t fiber) AGT_noexcept {
  if ( fiber == nullptr )
    return nullptr;

  if ( fiber == AGT_CURRENT_FIBER )
    fiber = agt_get_current_fiber(nullptr);
  // assert( fiber != nullptr );
  return fiber->userData;
  // return fiber->privateData->procData;
}

AGT_exec_api void*        AGT_stdcall agt_set_fiber_data(agt_fiber_t fiber, void* userData) AGT_noexcept {
  if (fiber == nullptr)
    return nullptr;
  if (fiber == AGT_CURRENT_FIBER)
    fiber = agt_get_current_fiber(nullptr);
  return std::exchange(fiber->userData, userData);
}

AGT_exec_api agt_fiber_t  AGT_stdcall agt_get_current_fiber(agt_ctx_t ctx) AGT_noexcept {
  (void)ctx;
  return static_cast<agt_fiber_t>(reinterpret_cast<PNT_TIB>(NtCurrentTeb())->ArbitraryUserPointer);
  // return static_cast<agt_fiber_t>(GetCurrentFiber());
  /*if (ctx == AGT_DEFAULT_CTX)
    ctx = g_lib._pfn_acquire_ctx(g_lib.instance);

  AGT_assert( ctx != AGT_INVALID_CTX );

  return ctx->boundFiber;*/
}





/*
AGT_exec_api agt_u64_t    AGT_stdcall agt_fiber_switch(agt_ctx_t ctx, agt_fiber_t fiber, agt_u64_t param, agt_fiber_save_flags_t flags) AGT_noexcept {
  AGT_assert( ctx != nullptr );
  AGT_assert( fiber != nullptr );

  namespace x64 = agt::impl::assembly;

  fiber_data saveData;

  // alignas(agt::impl::SaveDataAlignment) char saveStorage[agt::impl::FullSaveDataSize];

  auto currentFiber = ctx->boundFiber;
  agt_u64_t resultParam;

  if (currentFiber == nullptr) {
    g_lib._pfn_raise(ctx, AGT_ERROR_NO_FIBER_BOUND, nullptr);
    return param;
  }

  if (currentFiber == fiber)
    return param;

  if (x64::afiber_save(currentFiber, &saveData, &resultParam, flags))
    return resultParam;

  ctx->boundFiber = fiber;

  x64::afiber_jump(fiber, param, agt::FIBER_JUMP_SWITCH_FIBER);
}
*/

/*
AGT_exec_api void         AGT_stdcall agt_fiber_jump(agt_ctx_t ctx, agt_fiber_t fiber, agt_u64_t param) AGT_noexcept {
  AGT_assert( ctx != nullptr );
  AGT_assert( fiber != nullptr );
  AGT_assert( ctx->boundFiber != nullptr );

  namespace x64 = agt::impl::assembly;

  if (ctx->boundFiber != nullptr) {
    g_lib._pfn_raise(ctx, AGT_ERROR_NO_FIBER_BOUND, nullptr);
    return;
  }


  if (fiber == AGT_CURRENT_FIBER) {
    x64::afiber_jump(ctx->boundFiber, param, 0);
  }
  else {
    ctx->boundFiber = fiber;
    x64::afiber_jump(fiber, param, agt::FIBER_JUMP_SWITCH_FIBER);
  }
}
*/


AGT_exec_api agt_fiber_transfer_t AGT_stdcall agt_fiber_switch(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags) AGT_noexcept {
  return g_lib._pfn_fiber_switch(fiber, param, flags);
}

AGT_exec_api AGT_noreturn void    AGT_stdcall agt_fiber_jump(agt_fiber_t fiber, agt_fiber_param_t param) AGT_noexcept {
  g_lib._pfn_fiber_jump(fiber, param);
}

AGT_exec_api agt_fiber_transfer_t AGT_stdcall agt_fiber_fork(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags) AGT_noexcept {
  return g_lib._pfn_fiber_fork(proc, param, flags);
}

AGT_exec_api agt_fiber_param_t    AGT_stdcall agt_fiber_loop(agt_fiber_proc_t proc, agt_u64_t param, agt_fiber_flags_t flags) AGT_noexcept {
  return g_lib._pfn_fiber_loop(proc, param, flags);
}


/*


agt_status_t     AGT_stdcall agtNewContext(agt_ctx_t* pContext) AGT_noexcept {
  if (!pContext)
    return AGT_ERROR_INVALID_ARGUMENT;
  return createCtx(*pContext);
}

agt_status_t     AGT_stdcall agtDestroyContext(agt_ctx_t context) AGT_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}


agt_status_t     AGT_stdcall agtGetObjectInfo(agt_ctx_t context, agt_object_info_t* pObjectInfo) AGT_noexcept {
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

agt_status_t     AGT_stdcall agtDuplicateHandle(agt_handle_t inHandle, agt_handle_t* pOutHandle) AGT_noexcept {
  if (!pOutHandle) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  if (inHandle == AGT_NULL_HANDLE) [[unlikely]] {
    *pOutHandle = AGT_NULL_HANDLE;
    return AGT_ERROR_NULL_HANDLE;
  }
  return static_cast<Handle*>(inHandle)->duplicate(reinterpret_cast<Handle*&>(*pOutHandle));
}

void          AGT_stdcall agtCloseHandle(agt_handle_t handle) AGT_noexcept {
  if (handle) [[likely]]
    static_cast<Handle*>(handle)->close();
}



agt_status_t     AGT_stdcall agtCreateChannel(agt_ctx_t context, const agt_channel_create_info_t* cpCreateInfo, agt_handle_t* pSender, agt_handle_t* pReceiver) AGT_noexcept;
agt_status_t     AGT_stdcall agtCreateAgent(agt_ctx_t context, const agt_agent_create_info_t* cpCreateInfo, agt_handle_t* pAgent) AGT_noexcept;
agt_status_t     AGT_stdcall agtCreateAgency(agt_ctx_t context, const agt_agency_create_info_t* cpCreateInfo, agt_handle_t* pAgency) AGT_noexcept;
agt_status_t     AGT_stdcall agtCreateThread(agt_ctx_t context, const agt_thread_create_info_t* cpCreateInfo, agt_handle_t* pThread) AGT_noexcept;

agt_status_t     AGT_stdcall agtStage(agt_handle_t sender, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) AGT_noexcept {
  if (sender == AGT_NULL_HANDLE) [[unlikely]]
    return AGT_ERROR_NULL_HANDLE;
  if (pStagedMessage == nullptr) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  return static_cast<Handle*>(sender)->stage(*pStagedMessage, timeout);
}
void          AGT_stdcall agtSend(const agt_staged_message_t* cpStagedMessage, agt_async_t asyncHandle, agt_send_flags_t flags) AGT_noexcept {
  const auto& stagedMsg = reinterpret_cast<const StagedMessage&>(*cpStagedMessage);
  const auto message = stagedMsg.message;
  setMessageId(message, stagedMsg.id);
  setMessageReturnHandle(message, stagedMsg.returnHandle);
  setMessageAsyncHandle(message, asyncHandle);
  stagedMsg.receiver->send(stagedMsg.message, flags);
}
agt_status_t     AGT_stdcall agtReceive(agt_handle_t receiver, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) AGT_noexcept {
  if (receiver == AGT_NULL_HANDLE) [[unlikely]]
    return AGT_ERROR_NULL_HANDLE;
  if (pMessageInfo == nullptr) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  return static_cast<Handle*>(receiver)->receive(*pMessageInfo, timeout);
}




void          AGT_stdcall agtReturn(agt_message_t message, agt_status_t status) AGT_noexcept {

}


void          AGT_stdcall agtYieldExecution() AGT_noexcept;

agt_status_t     AGT_stdcall agtDispatchMessage(const agt_actor_t* pActor, const agt_message_info_t* pMessageInfo) AGT_noexcept;
// agt_status_t     AGT_stdcall agtExecuteOnThread(agt_thread_t thread, ) AGT_noexcept;





agt_status_t     AGT_stdcall agtGetSenderHandle(agt_message_t message, agt_handle_t* pSenderHandle) AGT_noexcept;




*/
/* ========================= [ Async ] ========================= *//*


agt_status_t     AGT_stdcall agtNewAsync(agt_ctx_t ctx, agt_async_t* pAsync) AGT_noexcept {

  if (!pAsync) [[unlikely]] {
    return AGT_ERROR_INVALID_ARGUMENT;
  }

  // TODO: Detect bad context?

  *pAsync = createAsync(ctx);

  return AGT_SUCCESS;
}

void          AGT_stdcall agtCopyAsync(agt_async_t from, agt_async_t to) AGT_noexcept {
  asyncCopyTo(from, to);
}

void          AGT_stdcall agtClearAsync(agt_async_t async) AGT_noexcept {
  asyncClear(async);
}

void          AGT_stdcall agtDestroyAsync(agt_async_t async) AGT_noexcept {
  asyncDestroy(async);
}

agt_status_t     AGT_stdcall agtWait(agt_async_t async, agt_timeout_t timeout) AGT_noexcept {
  return asyncWait(async, timeout);
}

agt_status_t     AGT_stdcall agtWaitMany(const agt_async_t* pAsyncs, size_t asyncCount, agt_timeout_t timeout) AGT_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}




*/
/* ========================= [ Signal ] ========================= *//*


agt_status_t     AGT_stdcall agtNewSignal(agt_ctx_t ctx, agt_signal_t* pSignal) AGT_noexcept {

}

void          AGT_stdcall agtAttachSignal(agt_signal_t signal, agt_async_t async) AGT_noexcept {
  signalAttach(signal, async);
}

void          AGT_stdcall agtRaiseSignal(agt_signal_t signal) AGT_noexcept {
  signalRaise(signal);
}

void          AGT_stdcall agtRaiseManySignals(agt_signal_t signal) AGT_noexcept {

}

void          AGT_stdcall agtDestroySignal(agt_signal_t signal) AGT_noexcept {

}
*/


}