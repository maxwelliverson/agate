//
// Created by maxwe on 2023-08-23.
//

#include "../module_exports.hpp"
#include "exports.hpp"
#include "../config_options.hpp"

#include "agate/processor.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "fiber.hpp"


#include <Windows.h>

#pragma comment(lib, "mincore")



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

namespace {

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

  sys_cstring                 lookupLibraryPath(init::init_manager& initManager) noexcept {
    auto dllHandle = get_dll_handle();
    constexpr static DWORD InitialBufSize = 32;

    DWORD currentbufferSize = InitialBufSize;
    std::unique_ptr<wchar_t[]> buffer = std::make_unique<wchar_t[]>(currentbufferSize);
    wchar_t* result;

    while (true) {
      DWORD stringSize = GetModuleFileNameW(dllHandle, buffer.get(), currentbufferSize);
      if (stringSize == 0) {
        initManager.raiseWin32Error("Could not determine path of core module DLL.");
        return nullptr;
      }

      if (stringSize < currentbufferSize) {
        result = buffer.release();
        break;
      }

      DWORD err = GetLastError();
      if (err != ERROR_INSUFFICIENT_BUFFER) {
        initManager.raiseWin32Error("GetModuleFileNameW failed for unknown reasons");
        return nullptr;
      }

      currentbufferSize *= 2;
      buffer = std::make_unique<wchar_t[]>(currentbufferSize);
    }

    initManager.registerCleanup([result] {
      delete[] result;
    });

    return result;
  }

  size_t                      lookupNativeDurationUnit(init::init_manager& initManager) noexcept {
    LARGE_INTEGER lrgInt;
    auto result = QueryPerformanceFrequency(&lrgInt);
    assert( result == TRUE ); // Only returns false on pre-windows XP systems, soooo

    uint64_t oneBillion = 1'000'000'000ULL;
    uint64_t oneBillionPlusHalf = oneBillion + (lrgInt.QuadPart / 2);

    return oneBillionPlusHalf / lrgInt.QuadPart; // this should be uhhhhh more or less right....
  }

  size_t                      lookupStackSizeAlignment(init::init_manager& initManager) noexcept {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwAllocationGranularity;
  }

  std::pair<uint64_t, size_t> lookupStateSaveInfo(init::init_manager& initManager) noexcept {
    int results[4];
    uint64_t stateSaveMask;
    size_t   stateSaveSize;

    __cpuid(results, 0xd);
    stateSaveSize = results[2];

    stateSaveMask = std::bit_cast<uint32_t>(results[3]);
    stateSaveMask <<= 32;
    stateSaveMask |= std::bit_cast<uint32_t>(results[0]);


    return { stateSaveMask, stateSaveSize };
  }

  uint32_t getMaxSystemThreadCount() noexcept {
    const auto caches = agt::cpu::getCacheInfos();
    for (auto cache : caches) {

    }
    return GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS);
  }


  bool isDosHeader(HMODULE mod) noexcept
  {
    return std::memcmp(mod, &"MZ", 2) == 0;
  }

  PIMAGE_NT_HEADERS toCoffHeader(HMODULE mod) noexcept
  {
    auto addr = reinterpret_cast<std::byte*>(mod);
    return reinterpret_cast<PIMAGE_NT_HEADERS>(addr + *reinterpret_cast<uint32_t*>(addr + 0x3C));
  }

  size_t getDefaultStackSize() noexcept {
    auto exeBase = get_exe_handle();
    auto exeHandle = GetModuleHandle(nullptr);

    PIMAGE_NT_HEADERS ntHeaders;

    if (isDosHeader(exeHandle))
      ntHeaders = toCoffHeader(exeHandle);
    else
      ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(exeHandle);

    return ntHeaders->OptionalHeader.SizeOfStackReserve;
  }
}



init::config_options::config_options(init_manager& manager, std::pair<uint64_t, size_t> stateSaveInfo) noexcept
  : internalAsyncSize(static_cast<uint32_t>(AGT_ASYNC_STRUCT_SIZE)),
    libraryPath(lookupLibraryPath(manager)),
    nativeDurationUnitSizeNs(lookupNativeDurationUnit(manager)),
    stackSizeAlignment(lookupStackSizeAlignment(manager)),
    fullStateSaveMask(stateSaveInfo.first),
    stateSaveSize(stateSaveInfo.second) {

  constexpr static size_t Size64K = 0x1ULL << 16;

  threadCount.setDefault(getMaxSystemThreadCount());
  cxxExceptionsEnabled.setDefault(false);
  durationUnitSizeNs.setDefault(nativeDurationUnitSizeNs);
  defaultFiberStackSize.setDefault(align_to(Size64K, stackSizeAlignment));
  defaultThreadStackSize.setDefault(getDefaultStackSize());
}

init::config_options::config_options(init_manager& manager) noexcept
  : config_options(manager, lookupStateSaveInfo(manager)) { }




void init::library_configuration::init_options(init_manager& initManager) noexcept {
  m_options = new config_options{initManager};
  m_optionsDtor = [](config_options* opt) {
    delete opt;
  };
  m_pfnCompileOptions = [](config_options* opt, attributes& attr, environment& env, init_manager& initManager) {
    opt->compileOptions(attr, env, initManager);
  };
}







void init::config_options::compileOptions(attributes &attr, environment &env, init_manager &initManager) noexcept  {
  const size_t attrCount = 18;
  const auto pTypes  = new agt_value_type_t[attrCount]{};
  const auto pValues = new uintptr_t[attrCount]{};


  size_t finalDefaultFiberStackSize = defaultFiberStackSize.resolve(env, initManager);
  size_t finalDefaultThreadStackSize = defaultThreadStackSize.resolve(env, initManager);

  finalDefaultFiberStackSize = align_to(finalDefaultFiberStackSize, stackSizeAlignment);
  finalDefaultThreadStackSize = align_to(finalDefaultThreadStackSize, stackSizeAlignment);

  sys_cstring resolvedSharedNamespace = sharedNamespace.resolve(env, initManager);

  const bool usingDefaultSharedNamespace = sharedNamespace.selectedDefault();


  sharedContext.setDefault(!usingDefaultSharedNamespace);

  const bool sharedContextIsEnabled = sharedContext.resolve(env, initManager);

  if (!sharedContextIsEnabled)
    resolvedSharedNamespace = nullptr;

  const auto finalSharedNamespace = std::bit_cast<uintptr_t>(resolvedSharedNamespace);


  // ...

  uint64_t durationUnitSize = durationUnitSizeNs.resolve(env, initManager);


  constexpr static agt_value_type_t SysStringType =
#if AGT_system_windows
    AGT_TYPE_WIDE_STRING
#else
      AGT_TYPE_STRING
#endif
    ;




  pTypes[AGT_ATTR_ASYNC_STRUCT_SIZE] = AGT_TYPE_UINT32;
  pValues[AGT_ATTR_ASYNC_STRUCT_SIZE] = internalAsyncSize;
  pTypes[AGT_ATTR_THREAD_COUNT] = AGT_TYPE_UINT32;
  pValues[AGT_ATTR_THREAD_COUNT] = threadCount.resolve(env, initManager);
  pTypes[AGT_ATTR_LIBRARY_PATH] = SysStringType;
  pValues[AGT_ATTR_LIBRARY_PATH] = std::bit_cast<uintptr_t>(libraryPath);
  pTypes[AGT_ATTR_LIBRARY_VERSION] = AGT_TYPE_INT32;
  pValues[AGT_ATTR_LIBRARY_VERSION] = std::bit_cast<uint32_t>(libraryVersion.resolve(env, initManager));
  pTypes[AGT_ATTR_SHARED_CONTEXT] = AGT_TYPE_BOOLEAN;
  pValues[AGT_ATTR_SHARED_CONTEXT] = sharedContextIsEnabled;

  pTypes[AGT_ATTR_CXX_EXCEPTIONS_ENABLED] = AGT_TYPE_BOOLEAN;
  pValues[AGT_ATTR_CXX_EXCEPTIONS_ENABLED] = cxxExceptionsEnabled.resolve(env, initManager);

  pTypes[AGT_ATTR_SHARED_NAMESPACE] = SysStringType;
  pValues[AGT_ATTR_SHARED_NAMESPACE] = finalSharedNamespace;


  pTypes[AGT_ATTR_DURATION_UNIT_SIZE_NS] = AGT_TYPE_UINT64;
  pValues[AGT_ATTR_DURATION_UNIT_SIZE_NS] = durationUnitSize;
  pTypes[AGT_ATTR_NATIVE_DURATION_UNIT_SIZE_NS] = AGT_TYPE_UINT64;
  pValues[AGT_ATTR_NATIVE_DURATION_UNIT_SIZE_NS] = nativeDurationUnitSizeNs;

  pTypes[AGT_ATTR_FIXED_CHANNEL_SIZE_GRANULARITY] = AGT_TYPE_UINT64;
  pValues[AGT_ATTR_FIXED_CHANNEL_SIZE_GRANULARITY] = fixedChannelSizeGranularity.resolve(env, initManager);

  pTypes[AGT_ATTR_STACK_SIZE_ALIGNMENT] = AGT_TYPE_UINT64;
  pValues[AGT_ATTR_STACK_SIZE_ALIGNMENT] = stackSizeAlignment;
  pTypes[AGT_ATTR_DEFAULT_THREAD_STACK_SIZE] = AGT_TYPE_UINT64;
  pValues[AGT_ATTR_DEFAULT_THREAD_STACK_SIZE] = finalDefaultThreadStackSize;
  pTypes[AGT_ATTR_DEFAULT_FIBER_STACK_SIZE] = AGT_TYPE_UINT64;
  pValues[AGT_ATTR_DEFAULT_FIBER_STACK_SIZE] = finalDefaultFiberStackSize;

  pTypes[AGT_ATTR_FULL_STATE_SAVE_MASK] = AGT_TYPE_UINT64;
  pValues[AGT_ATTR_FULL_STATE_SAVE_MASK] = fullStateSaveMask;
  pTypes[AGT_ATTR_MAX_STATE_SAVE_SIZE] = AGT_TYPE_UINT32;
  pValues[AGT_ATTR_MAX_STATE_SAVE_SIZE] = static_cast<uint32_t>(stateSaveSize);



  attr.m_attrCount       = attrCount;
  attr.m_attrType        = pTypes;
  attr.m_attrValue       = pValues;
  attr.m_usingNativeUnit = durationUnitSize == nativeDurationUnitSizeNs;
  attr.m_destroyArrays   = [](const agt_value_type_t* pTypes, const uintptr_t* pAttrs) {
    delete[] pTypes;
    delete[] pAttrs;
  };
}



namespace {

  /*
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



  template <typename T>
  class
  */








}




// This is an example of how the export selection works.


extern "C" AGT_export void agatelib_config_library(init::library_configuration& out, agt_config_t config, init::init_manager& initManager) noexcept {
  out.init_options(initManager);

  auto& opt = out.options();

  auto& val = config->options.emplace_back();
  val.id = AGT_CONFIG_LIBRARY_VERSION;
  val.flags = init::AGT_OPTION_IS_INTERNAL_CONSTRAINT;
  val.necessity = AGT_INIT_OPTIONAL;
  val.type = AGT_TYPE_INT32;
  val.value.int32 = AGT_API_VERSION;


  for (auto&& configOption : config->options) {
    switch (configOption.id) {
      case AGT_CONFIG_ASYNC_STRUCT_SIZE:
        break;
      case AGT_CONFIG_THREAD_COUNT:
        opt.threadCount.addConstraint(config, configOption, initManager);
        break;
      case AGT_CONFIG_SEARCH_PATH:
        break;
      case AGT_CONFIG_LIBRARY_VERSION:
        opt.libraryVersion.addConstraint(config, configOption, initManager);
        break;
      case AGT_CONFIG_IS_SHARED:
        opt.sharedContext.addConstraint(config, configOption, initManager);
        break;
      case AGT_CONFIG_SHARED_NAMESPACE:
        break;
      case AGT_CONFIG_CHANNEL_DEFAULT_CAPACITY:
        break;
      case AGT_CONFIG_CHANNEL_DEFAULT_MESSAGE_SIZE:
        break;
      case AGT_CONFIG_CHANNEL_DEFAULT_TIMEOUT_MS:
        break;
      case AGT_CONFIG_DURATION_GRANULARITY_NS:
        break;
      case AGT_CONFIG_FIXED_CHANNEL_SIZE_GRANULARITY:
        break;
      case AGT_CONFIG_STACK_SIZE_ALIGNMENT:
        break;
      case AGT_CONFIG_DEFAULT_THREAD_STACK_SIZE:
        opt.defaultThreadStackSize.addConstraint(config, configOption, initManager);
        break;
      case AGT_CONFIG_DEFAULT_FIBER_STACK_SIZE:
        opt.defaultFiberStackSize.addConstraint(config, configOption, initManager);
        break;
      case AGT_CONFIG_STATE_SAVE_MASK:
        break;
      case AGT_CONFIG_DEFAULT_ALLOCATOR_PARAMS:
        break;
    }
  }
}


extern "C" AGT_export void agatelib_get_exports(agt::init::module_exports& moduleExports, const agt::init::attributes& attributes) noexcept {
  bool exceptionsEnabled = true;
  bool isShared = false;
  bool usingNativeUnit = attributes.using_native_unit();

  if (auto maybeExceptionsEnabled = attributes.get_as<bool>(AGT_ATTR_CXX_EXCEPTIONS_ENABLED))
    exceptionsEnabled = *maybeExceptionsEnabled;
  if (auto maybeIsShared = attributes.get_as<bool>(AGT_ATTR_SHARED_CONTEXT))
    isShared = *maybeIsShared;


  if (isShared) {
    AGT_add_private_export(create_instance, create_instance_shared);
    AGT_add_private_export(destroy_instance, destroy_instance_shared);
    AGT_add_private_export(create_ctx, create_ctx_shared);
    AGT_add_private_export(release_ctx, release_ctx_shared);
    // AGT_add_public_export(finalize, finalize_ctx_shared);
  }
  else {
    AGT_add_private_export(create_instance, create_instance_private);
    AGT_add_private_export(destroy_instance, destroy_instance_private);
    AGT_add_private_export(create_ctx, create_ctx_private);
    AGT_add_private_export(release_ctx, release_ctx_private);
    // AGT_add_public_export(finalize, finalize_ctx_private);
  }

  AGT_add_public_export(ctx, get_ctx);
  AGT_add_private_export(do_acquire_ctx, acquire_ctx);


  /*if (exceptionsEnabled) {
    AGT_add_private_export(fiber_init, agt::fiber_init_except);
    AGT_add_public_export(fiber_fork, agt::fiber_fork_except);
    AGT_add_public_export(fiber_loop, agt::fiber_loop_except);
    AGT_add_public_export(fiber_switch, agt::fiber_switch_except);

  }
  else {
    AGT_add_private_export(fiber_init, agt::fiber_init_noexcept);
    AGT_add_public_export(fiber_fork, agt::fiber_fork_noexcept);
    AGT_add_public_export(fiber_loop, agt::fiber_loop_noexcept);
    AGT_add_public_export(fiber_switch, agt::fiber_switch_noexcept);
  }*/


  AGT_add_public_export(enter_fctx, enter_fctx);
  AGT_add_public_export(exit_fctx, exit_fctx);
  AGT_add_public_export(new_fiber, new_fiber);
  AGT_add_public_export(destroy_fiber, destroy_fiber);

  AGT_add_private_export(fiber_init, agt::impl::assembly::afiber_init);
  AGT_add_public_export(fiber_fork, agt::impl::assembly::afiber_fork);
  AGT_add_public_export(fiber_jump, agt::impl::assembly::afiber_jump);
  AGT_add_public_export(fiber_loop, agt::impl::assembly::afiber_loop);
  AGT_add_public_export(fiber_switch, agt::impl::assembly::afiber_switch);

  /*if (isShared) {
    AGT_add_private_export(initialize_async, agt::init_async_shared);
    AGT_add_public_export(copy_async, agt::copy_async_shared);
    if (usingNativeUnit)
      AGT_add_public_export(wait, agt::async_wait_native_unit_shared);
    else
      AGT_add_public_export(wait, agt::async_wait_foreign_unit_shared);
  }
  else {
    if (usingNativeUnit)
      AGT_add_public_export(wait, agt::async_wait_native_unit_private);
    else
      AGT_add_public_export(wait, agt::async_wait_foreign_unit_private);
  }*/

}






