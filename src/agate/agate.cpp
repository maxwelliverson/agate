//
// Created by maxwe on 2021-11-23.
//

// #include "internal.hpp"
// #include "handle.hpp"

#define AGT_UNDEFINED_ASYNC_STRUCT

#include "agate.h"

#include "environment.hpp"
#include "integer_division.hpp"
#include "module.hpp"
#include "sys_string.hpp"

#include "priority_queue.hpp"

#include "async/async.hpp"
#include "async/signal.hpp"
#include "channels/message.hpp"
#include "core/instance.hpp"
#include "core/object.hpp"

#include "agents/state.hpp"
#include "agents/agents.hpp"

#include "channels/message_pool.hpp"
#include "channels/message_queue.hpp"




#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma comment(lib, "mincore")


#include <cstring>
#include <string_view>
#include <optional>
#include <charconv>
#include <algorithm>
#include <memory>
#include <bit>

#define GET_MODULE_VERSION_FUNCTION_NAME "_agt_get_module_version"

#define AGATE_LIBRARY_PATH_ENVVAR           AGT_sys_string("AGATE_LIBRARY_PATH")
#define AGATE_MIN_LIBRARY_VERSION_ENVVAR    AGT_sys_string("AGATE_MIN_VERSION")
#define SHARED_CONTEXT_ENVVAR               AGT_sys_string("AGATE_SHARED_CONTEXT")
#define SHARED_NAMESPACE_ENVVAR             AGT_sys_string("AGATE_SHARED_NAMESPACE")
#define CHANNEL_DEFAULT_CAPACITY_ENVVAR     AGT_sys_string("AGATE_CHANNEL_DEFAULT_CAPACITY")
#define CHANNEL_DEFAULT_MESSAGE_SIZE_ENVVAR AGT_sys_string("AGATE_CHANNEL_DEFAULT_MESSAGE_SIZE")
#define CHANNEL_DEFAULT_TIMEOUT_MS_ENVVAR   AGT_sys_string("AGATE_CHANNEL_DEFAULT_TIMEOUT_MS")


#define AGT_DEFAULT_SHARED_CONTEXT       false
#define AGT_DEFAULT_SHARED_NAMESPACE     AGT_sys_string("agate-default-namespace")
#define AGT_DEFAULT_DEFAULT_CAPACITY     255
#define AGT_DEFAULT_DEFAULT_MESSAGE_SIZE 196
#define AGT_DEFAULT_DEFAULT_TIMEOUT_MS   5000


#define AGT_CORE_LIBRARY_NAME     AGT_sys_string("agate-core")
#define AGT_AGENTS_LIBRARY_NAME   AGT_sys_string("agate-agents")
#define AGT_ASYNC_LIBRARY_NAME    AGT_sys_string("agate-async")
#define AGT_CHANNELS_LIBRARY_NAME AGT_sys_string("agate-channels")




using namespace agt;

struct module_info {
  module_id id;
  version   moduleVersion;
  HMODULE   libraryHandle;
  char      moduleName[16];
  wchar_t   libraryPath[260];
};

namespace {

  inline export_table g_lib = {};



  using function_ptr = void(*)();

  struct function_info {
    size_t      offset;
    const char* name;
    module_id   moduleBit;
  };

  struct api_version_specific_info {
    version              minApiVersion;
    size_t               asyncStructSize;
    size_t               signalStructSize;
    const function_info* functions;
  };


#define AGT_func(funcName, moduleIdBit)                \
  {                                                    \
    .offset    = offsetof(export_table, _pfn_##funcName), \
    .name      = #funcName,                  \
    .moduleBit = (moduleIdBit)                         \
  }

#define AGT_core_func(funcName) AGT_func(funcName, AGT_CORE_MODULE)
#define AGT_agent_func(funcName) AGT_func(funcName, AGT_AGENTS_MODULE)
#define AGT_async_func(_funcName) AGT_func(_funcName, AGT_ASYNC_MODULE)
#define AGT_channel_func(funcName) AGT_func(funcName, AGT_CHANNELS_MODULE)



  const function_info g_functionList[] = {

      AGT_core_func(finalize),

      AGT_async_func(new_async),
      AGT_async_func(copy_async),
      AGT_async_func(move_async),
      AGT_async_func(clear_async),
      AGT_async_func(destroy_async),
      AGT_async_func(async_status),
      AGT_async_func(wait),
      AGT_async_func(wait_all),
      AGT_async_func(wait_any),
      AGT_async_func(new_signal),
      AGT_async_func(attach_signal),
      AGT_async_func(attach_many_signals),
      AGT_async_func(raise_signal),
      AGT_async_func(raise_many_signals),
      AGT_async_func(destroy_signal),

      AGT_agent_func(create_agent),
      AGT_agent_func(create_busy_agent_on_current_thread),
      AGT_agent_func(send)
  };

#define AGT_async_struct_size(val) .asyncStructSize = val
#define AGT_signal_struct_size(val) .signalStructSize = val

#define AGT_version_info(verStr, asyncStruct_, signalStruct_, firstFunc_) \
  {                                                                       \
      .minApiVersion = parse_version(verStr).value(),                     \
      asyncStruct_,                                                       \
      signalStruct_,                                                      \
      .functions     = &g_functionList[firstFunc_]                        \
  }

  constexpr api_version_specific_info g_apiVersionSpecificInfo[] = {
      AGT_version_info("0.0",
                       AGT_async_struct_size(40),
                       AGT_signal_struct_size(24),
                       0)
  };


  struct init_options {
    version                       apiVersion = {};
    version                       minLibraryVersion = {};
    agt_flags32_t                 moduleFlags = 0;
    agt_flags32_t                 initFlags = 0;
    bool                          sharedContext = false;
    sys_cstring                   sharedNamespace = nullptr;
    sys_cstring                   sharedNamespaceDynPtr = nullptr;
    size_t                        channelDefaultCapacity = 0;
    size_t                        channelDefaultMessageSize = 0;
    size_t                        channelDefaultTimeoutMs = 0;
    std::unique_ptr<agate_module> modules[MaxModuleCount] = {};
  };


  template <typename T>
  struct option {
    bool             attrSpecified      = false;
    bool             envOverrideEnabled = true;
    std::optional<T> value              = std::nullopt;
  };



  agt_status_t  load_module(init_options& opt, agt_init_flags_t flags, module_id id, sys_cstring libName) noexcept {
    if ((flags & id) == id)
      opt.modules[module_index(id)] = agate_module::load(libName);
    return AGT_SUCCESS;
  }

  agt_status_t  load_all_modules(init_options& opt, sys_cstring searchPath, agt_init_flags_t flags) noexcept {
    DLL_DIRECTORY_COOKIE directoryCookie = {};
    const DWORD    PathBufSize = 512;
    sys_char       absPathBuffer[PathBufSize];
    unique_sys_str strDeleter;

    if (searchPath) {
      std::memset(absPathBuffer, 0, sizeof absPathBuffer);
      const wchar_t* fullSearchPath;
      LPWSTR pFilePart = nullptr;
      DWORD pathLength = GetFullPathNameW(searchPath, PathBufSize, absPathBuffer, &pFilePart);

      if (pathLength == 0)
        return AGT_ERROR_NOT_A_DIRECTORY;
      else if (pathLength >= PathBufSize) {
        auto* dynamicBuffer = (wchar_t*)::malloc(pathLength * sizeof(wchar_t));
        DWORD newPathLength = GetFullPathNameW(searchPath, pathLength, dynamicBuffer, &pFilePart);
        AGT_assert( newPathLength == pathLength - 1 );
        strDeleter.reset(dynamicBuffer);
        fullSearchPath = dynamicBuffer;
      }
      else {
        fullSearchPath = absPathBuffer;
      }

      if (pFilePart != nullptr)
        return AGT_ERROR_NOT_A_DIRECTORY;

      directoryCookie = AddDllDirectory(fullSearchPath);
    }

    if (auto result = load_module(opt, flags, AGT_CORE_MODULE, AGT_CORE_LIBRARY_NAME))
      return result;
    if (auto result = load_module(opt, flags, AGT_AGENTS_MODULE, AGT_AGENTS_LIBRARY_NAME))
      return result;
    if (auto result = load_module(opt, flags, AGT_ASYNC_MODULE, AGT_ASYNC_LIBRARY_NAME))
      return result;
    if (auto result = load_module(opt, flags, AGT_CHANNELS_MODULE, AGT_CHANNELS_LIBRARY_NAME))
      return result;

    if (directoryCookie) {
      BOOL result = RemoveDllDirectory(directoryCookie);
      AGT_assert( result != FALSE );
    }
  }






  agt_status_t  prepare_init_options(init_options& opt, const agt_init_info_t& info) noexcept {
    // opt.sharedCtxNamespace.clear();

    environment env;

    std::memset(&opt, 0, sizeof(init_options));

    opt.apiVersion = version::from_integer(info.headerVersion);

    option<unique_sys_str> optLibraryPath;
    option<version>        optMinLibVersion;
    option<bool>           optSharedContext;
    option<unique_sys_str> optSharedNamespace;
    option<agt_u32_t>      optChannelDefaultCapacity;
    option<agt_u32_t>      optChannelDefaultMessageSize;
    option<agt_u32_t>      optChannelDefaultTimeoutMs;


    if (info.attributes) {
      AGT_assert( info.attributeCount > 0 );
      for (size_t i = 0; i < info.attributeCount; ++i) {
        auto&& attr = info.attributes[i];
        switch (attr.id) {
          case AGT_ATTR_LIBRARY_PATH:
            if ( !attr.value.ptr )
              return AGT_ERROR_INVALID_ATTRIBUTE_VALUE;
            optLibraryPath.attrSpecified = true;
            optLibraryPath.envOverrideEnabled = attr.allowEnvironmentOverride;
            if (attr.type == AGT_ATTR_TYPE_STRING)
              optLibraryPath.value.emplace(make_unique_string((const char*)attr.value.ptr));
            else if (attr.type == AGT_ATTR_TYPE_WIDE_STRING)
              optLibraryPath.value.emplace(make_unique_string((const wchar_t*)attr.value.ptr));
            else
              return AGT_ERROR_INVALID_ATTRIBUTE_VALUE;
            break;

          case AGT_ATTR_MIN_LIBRARY_VERSION:
            if ( attr.type != AGT_ATTR_TYPE_UINT32)
              return AGT_ERROR_INVALID_ATTRIBUTE_VALUE;
            optMinLibVersion.attrSpecified = true;
            optMinLibVersion.envOverrideEnabled = attr.allowEnvironmentOverride;
            optMinLibVersion.value.emplace(version::from_integer(attr.value.u32));
            break;

          case AGT_ATTR_SHARED_CONTEXT:
            if ( attr.type != AGT_ATTR_TYPE_BOOLEAN )
              return AGT_ERROR_INVALID_ATTRIBUTE_VALUE;

            optSharedContext.attrSpecified = true;
            optSharedContext.envOverrideEnabled = attr.allowEnvironmentOverride;
            optSharedContext.value.emplace(attr.value.boolean);
            break;

          case AGT_ATTR_SHARED_NAMESPACE:
            if ( !attr.value.ptr )
              return AGT_ERROR_INVALID_ATTRIBUTE_VALUE;
            optSharedNamespace.attrSpecified = true;
            optSharedNamespace.envOverrideEnabled = attr.allowEnvironmentOverride;
            if (attr.type == AGT_ATTR_TYPE_STRING)
              optSharedNamespace.value.emplace(make_unique_string((const char*)attr.value.ptr));
            else if (attr.type == AGT_ATTR_TYPE_WIDE_STRING)
              optSharedNamespace.value.emplace(make_unique_string((const wchar_t*)attr.value.ptr));
            else
              return AGT_ERROR_INVALID_ATTRIBUTE_VALUE;
            break;

          case AGT_ATTR_CHANNEL_DEFAULT_CAPACITY:
            optChannelDefaultCapacity.attrSpecified = true;
            optChannelDefaultCapacity.envOverrideEnabled = attr.allowEnvironmentOverride;
            optChannelDefaultCapacity.value.emplace(attr.value.u32);
            break;

          case AGT_ATTR_CHANNEL_DEFAULT_MESSAGE_SIZE:
            optChannelDefaultMessageSize.attrSpecified = true;
            optChannelDefaultMessageSize.envOverrideEnabled = attr.allowEnvironmentOverride;
            optChannelDefaultMessageSize.value.emplace(attr.value.u32);
            break;

          case AGT_ATTR_CHANNEL_DEFAULT_TIMEOUT_MS:
            optChannelDefaultTimeoutMs.attrSpecified = true;
            optChannelDefaultTimeoutMs.envOverrideEnabled = attr.allowEnvironmentOverride;
            optChannelDefaultTimeoutMs.value.emplace(attr.value.u32);
            break;
        }
      }
    }

    envvar libPathEnvVar,
           minLibVersionEnvVar,
           sharedContextEnvVar,
           sharedNamespaceEnvVar,
           channelDefaultCapacityEnvVar,
           channelDefaultMsgSizeEnvVar,
           defaultTimeoutMsEnvVar;


    if (optLibraryPath.envOverrideEnabled && (libPathEnvVar = env[AGATE_LIBRARY_PATH_ENVVAR])) {
      auto libPathStr = libPathEnvVar.str();
      if (!libPathStr.empty())
        optLibraryPath.value.emplace(make_unique_string(libPathStr));
    }


    if (optMinLibVersion.envOverrideEnabled && (minLibVersionEnvVar = env[AGATE_MIN_LIBRARY_VERSION_ENVVAR])) {
      if (auto parsedVersion = envvar_cast<version>(minLibVersionEnvVar))
        optMinLibVersion.value = parsedVersion;
      else
        return AGT_ERROR_INVALID_ENVVAR_VALUE;
    }
    else if (!optMinLibVersion.attrSpecified)
      optMinLibVersion.value.emplace(0);


    if (optSharedContext.envOverrideEnabled && (sharedContextEnvVar = env[SHARED_CONTEXT_ENVVAR]))
      optSharedContext.value.emplace(envvar_cast<bool>(sharedContextEnvVar).value_or(true));
    else if (!optSharedContext.attrSpecified)
      optSharedContext.value.emplace(AGT_DEFAULT_SHARED_CONTEXT);


    if (optSharedNamespace.envOverrideEnabled && (sharedNamespaceEnvVar = env[SHARED_NAMESPACE_ENVVAR]))
      optSharedNamespace.value.emplace(make_unique_string(sharedNamespaceEnvVar.str()));
    else if (!optSharedNamespace.attrSpecified)
      optSharedNamespace.value.emplace(make_unique_string(AGT_DEFAULT_SHARED_NAMESPACE));


    if (optChannelDefaultCapacity.envOverrideEnabled && (channelDefaultCapacityEnvVar = env[CHANNEL_DEFAULT_CAPACITY_ENVVAR]))
      optChannelDefaultCapacity.value = envvar_cast<agt_u32_t>(channelDefaultCapacityEnvVar);
    else if (!optChannelDefaultCapacity.attrSpecified)
      optChannelDefaultCapacity.value.emplace(AGT_DEFAULT_DEFAULT_CAPACITY);


    if (optChannelDefaultMessageSize.envOverrideEnabled && (channelDefaultMsgSizeEnvVar = env[CHANNEL_DEFAULT_MESSAGE_SIZE_ENVVAR])) {
      if (auto size = envvar_cast<agt_u32_t>(channelDefaultMsgSizeEnvVar))
        optChannelDefaultMessageSize.value = size;
      else
        return AGT_ERROR_INVALID_ENVVAR_VALUE;
    }
    else if (!optChannelDefaultMessageSize.attrSpecified)
      optChannelDefaultMessageSize.value.emplace(AGT_DEFAULT_DEFAULT_MESSAGE_SIZE);



    if (optChannelDefaultTimeoutMs.envOverrideEnabled && (defaultTimeoutMsEnvVar = env[CHANNEL_DEFAULT_TIMEOUT_MS_ENVVAR])) {
      if (auto timeout = envvar_cast<agt_u32_t>(defaultTimeoutMsEnvVar))
        optChannelDefaultTimeoutMs.value = timeout;
      else
        return AGT_ERROR_INVALID_ENVVAR_VALUE;
    }
    else if (!optChannelDefaultTimeoutMs.attrSpecified)
      optChannelDefaultTimeoutMs.value.emplace(AGT_DEFAULT_DEFAULT_TIMEOUT_MS);



    if (optSharedNamespace.value) {
      opt.sharedNamespace       = optSharedNamespace.value->release();
      opt.sharedNamespaceDynPtr = opt.sharedNamespace;
    }
    else {
      opt.sharedNamespace       = AGT_DEFAULT_SHARED_NAMESPACE;
      opt.sharedNamespaceDynPtr = nullptr;
    }

    opt.sharedContext             = optSharedContext.value.value();
    opt.minLibraryVersion         = optMinLibVersion.value.value();
    opt.channelDefaultCapacity    = optChannelDefaultCapacity.value.value();
    opt.channelDefaultMessageSize = optChannelDefaultMessageSize.value.value();
    opt.channelDefaultTimeoutMs   = optChannelDefaultTimeoutMs.value.value();


    return load_all_modules(opt, optLibraryPath.value->get(), info.flags);
  }

  module_table* make_module_table(const init_options& opt) noexcept {
    size_t moduleCount = std::popcount(opt.moduleFlags) + 1;
    auto table         = new module_table;
    auto infoArray     = new module_info[moduleCount];

    table->table    = infoArray;
    table->count    = moduleCount;
    table->refCount = 1;

    std::memcpy(&infoArray[0], &opt.modules[0], sizeof(module_info)); // Always copy core module info

    auto id = (module_id)0x1;
    size_t moduleIndex = 1;
    for (size_t i = 1; i < MaxModuleCount; ++i, id = (module_id)(id << 1)) {
      if (id & opt.moduleFlags) {
        std::memcpy(&infoArray[moduleIndex], &opt.modules[i], sizeof(module_info));
        moduleIndex += 1;
      }
    }

    return table;
  }

  void          destroy_init_options(init_options& opt) noexcept {
    ::free((void*)opt.sharedNamespaceDynPtr);

    for (auto& mod : opt.modules)
      mod.reset();
  }


  const api_version_specific_info* get_version_info(version apiVersion) noexcept {
    if (apiVersion == version(0))
      return &g_apiVersionSpecificInfo[0];

    auto begin = std::make_reverse_iterator(std::end(g_apiVersionSpecificInfo));
    auto end   = std::make_reverse_iterator(std::begin(g_apiVersionSpecificInfo));

    auto ptr = std::lower_bound(
                   begin,
                   end,
                   apiVersion,
                   [](const api_version_specific_info& info, version v){
                     return info.minApiVersion < v;
                   }).base();

    AGT_assert( &g_apiVersionSpecificInfo[0] <= ptr );

    return ptr;
  }

  function_ptr select_function(const init_options& opt, agate_module* mod, std::string_view funcName) noexcept {
    // TODO: Implement dispatch!!!
  }

  void load_function(const init_options& opt, export_table* table, const function_info& func) noexcept {
    constexpr static size_t MaxFuncName = 128;
    char funcName[MaxFuncName] = {};

    if ((func.moduleBit & opt.moduleFlags) == func.moduleBit) {
      auto& mod = opt.modules[module_index(func.moduleBit)];

      auto funcPtr = select_function(opt, mod.get(), std::string_view(func.name));

      std::memcpy(((char*)table) + func.offset, &funcPtr, sizeof(void*));
    }
  }

  void load_functions(const init_options& opt, version libraryVersion) noexcept {
    const api_version_specific_info* versionSpecificInfo = get_version_info(libraryVersion);

    auto begin = &g_functionList[0];
    const function_info* end;

    const auto table = &g_lib;

    auto nextVersionInfo = versionSpecificInfo + 1;
    if (nextVersionInfo < std::end(g_apiVersionSpecificInfo))
      end = nextVersionInfo->functions;
    else
      end = std::end(g_functionList);

    for (auto f = begin; f != end; ++f)
      load_function(opt, table, *f);
  }

  version get_effective_version(const init_options& opt) noexcept {
    version minVer = version::from_integer(AGT_API_VERSION);

    for (auto&& mod : opt.modules) {
      if (mod) {
        auto ver = mod->version();
        if (ver < minVer)
          minVer = ver;
      }
    }

    return minVer;
  }
}














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


AGT_api       agt_status_t AGT_stdcall agt_init(agt_ctx_t* pContext, const agt_init_info_t* pInitInfo) AGT_noexcept {

  if (pInitInfo == nullptr)
    return AGT_ERROR_INVALID_ARGUMENT;

  init_options options;

  const auto loaderVersion = version::from_integer(AGT_API_VERSION);

  if (auto status = prepare_init_options(options, *pInitInfo))
    return status;



  auto apiVersionInfo = get_version_info(options.apiVersion);

  g_lib._size_of_async_struct      = apiVersionInfo->asyncStructSize;
  g_lib._size_of_signal_struct     = apiVersionInfo->signalStructSize;
  g_lib._header_version            = options.apiVersion;
  g_lib._effective_library_version = ;

  auto libVersionInfo = get_version_info(std::min(AGT_API_VERSION, options.apiVersion));






  destroy_init_options(options);


}



AGT_agent_api agt_status_t AGT_stdcall agt_send(agt_self_t self, agt_agent_t recipientHandle, const agt_send_info_t* pSendInfo) AGT_noexcept {

  assert(pSendInfo != nullptr);
  assert(recipientHandle != nullptr);

  auto sender    = agt_self();
  auto recipient = recipientHandle->instance;

  assert(sender != nullptr);
  assert(recipient != nullptr);

  auto ctx       = agt::tl_state.context;

  auto messageSize = pSendInfo->size;


  auto message   = agt::acquire_message(ctx, recipient->messagePool, messageSize);
  if (!message)
    return AGT_ERROR_MESSAGE_TOO_LARGE;



  message->messageType = agt::AGT_CMD_SEND;
  message->sender      = sender;
  message->receiver    = recipient;
  message->payloadSize = static_cast<uint32_t>(messageSize);

  std::memcpy(message->inlineBuffer, pSendInfo->buffer, messageSize);

  if (pSendInfo->asyncHandle) {
    auto& async = *pSendInfo->asyncHandle;
    agt::asyncAttachLocal(async, 1, 1);
    auto asyncData = agt::asyncGetData(async);
    message->asyncData = asyncData;
    message->asyncDataKey = agt::asyncDataGetKey(asyncData, nullptr);
  }
#if !defined(NDEBUG)
  else {
    message->asyncData    = {};
    message->asyncDataKey = {};
  }
#endif

  if (auto enqueueStatus = agt::enqueueMessage(recipient->messageQueue, message)) {
    agt::release_message(ctx, recipient->messagePool, message);
    return enqueueStatus;
  }

  return AGT_SUCCESS;
}

AGT_agent_api agt_status_t AGT_stdcall agt_send_as(agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept {

}

AGT_agent_api agt_status_t AGT_stdcall agt_send_many(const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept {

}

AGT_agent_api agt_status_t AGT_stdcall agt_send_many_as(agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept {

}




}




AGT_noinline static agt_status_t agtGetSharedObjectInfoById(agt_ctx_t context, agt_object_info_t* pObjectInfo) noexcept;


extern "C" {


agt_status_t        AGT_stdcall agt_query_instance_attributes(agt_instance_t instance, agt_attr_t* pAttributes, size_t attributeCount) noexcept {
  if (!instance) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;
  if (attributeCount) [[likely]] {
    if (!pAttributes) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;

    agt_status_t status = AGT_SUCCESS;

    const auto attrValues = g_lib.attrValues;
    const auto attrTypes  = g_lib.attrTypes;
    const auto attrCount  = g_lib.attrCount;

    for (size_t i = 0; i < attributeCount; ++i) {
      auto& attr = pAttributes[i];
      if (attr.id >= attrCount) [[unlikely]] {
        status = AGT_ERROR_UNKNOWN_ATTRIBUTE;
        attr.type = AGT_ATTR_TYPE_UNKNOWN;
        attr.u64 = 0;
      }
      else {
        attr.u64 = attrValues[attr.id];
        attr.type = attrTypes[attr.id];
      }
    }

    return status;
  }

}

agt_instance_t      AGT_stdcall agt_get_instance(agt_ctx_t context) noexcept {
  return agt::get_instance(context);
}

agt_ctx_t           AGT_stdcall agt_current_context() noexcept {
  return agt::get_ctx();
}

int                 AGT_stdcall agt_get_instance_version(agt_instance_t instance) noexcept {}

agt_error_handler_t AGT_stdcall agt_get_error_handler(agt_instance_t instance) noexcept {}

agt_error_handler_t AGT_stdcall agt_set_error_handler(agt_instance_t instance, agt_error_handler_t errorHandlerCallback) noexcept {}



/*


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




*/
/* ========================= [ Async ] ========================= *//*


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




*/
/* ========================= [ Signal ] ========================= *//*


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
*/


}