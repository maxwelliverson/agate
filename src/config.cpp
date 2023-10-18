//
// Created by maxwe on 2023-08-14.
//

#include "init.hpp"

#include "agate/dictionary.hpp"
#include "agate/vector.hpp"
#include "agate/map.hpp"
#include "agate/set.hpp"

#include <memory>
#include <vector>
#include <string_view>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stringapiset.h>



agt::init::module::module(const char *name, const agt::init::attributes& attr, bool usingCustomSearchPath) noexcept {
  char libName[MAX_PATH];

  m_name = name;
  m_attributes = &attr;

  std::string_view libPrefix = "agate-";
  char* bufStart = &libName[libPrefix.size()];
  size_t bufSize = std::size(libName) - libPrefix.size();
  if (auto err = strcpy_s(bufStart, bufSize, name)) {
    // BLEH
    m_handle = nullptr;
  }
  else if (usingCustomSearchPath)
    m_handle = LoadLibraryEx(libName, nullptr, LOAD_LIBRARY_SEARCH_USER_DIRS);
  else
    m_handle = LoadLibrary(libName);
  m_hasRetrievedExports = false;
}

agt_proc_t agt::init::module::get_raw_proc(const char *procName) noexcept {
  if (m_handle)
    return nullptr;
  return (agt_proc_t) GetProcAddress((HMODULE)m_handle, procName);
}

std::span<const agt::init::export_info> agt::init::module::exports() noexcept {
  if (!m_handle)
    return { };
  if (m_hasRetrievedExports)
    return m_exports.exports();
  m_exports.clear();
  auto get_exports_proc = (get_exports_proc_t)this->get_raw_proc("agatelib_get_exports");
  get_exports_proc(m_exports, *m_attributes);
  m_hasRetrievedExports = true;
  return m_exports.exports();
}



struct tmp_export_info {
  size_t     moduleIndex;
  agt_proc_t address;
  size_t     tableOffset;
};



struct tmp_loader_data {
  agt::init::attributes            attributes;
  std::vector<agt::init::module>   modules;
  agt::dictionary<tmp_export_info> exports;
};



void load_modules(tmp_loader_data& data, std::span<const char* const> moduleNames) noexcept {
  std::unique_ptr<wchar_t[]> cvtPtr;
  const wchar_t* customSearchPath;
  DLL_DIRECTORY_COOKIE directoryCookie;
  if (auto maybeSearchPath = data.attributes.get_as<const char*>(AGT_ATTR_LIBRARY_PATH)) {
    auto cSearchPath = *maybeSearchPath;
    size_t cStrLength = std::strlen(cSearchPath);
    cvtPtr = std::make_unique<wchar_t[]>(cStrLength + 1);
    wchar_t* wBuffer = cvtPtr.get();
    int result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, cSearchPath, (int)cStrLength, wBuffer, (int)cStrLength + 1);
    if (result == 0) {
      // Raise invalid path error,
    }
    else {
      wBuffer[result] = L'\0';
      customSearchPath = wBuffer;
    }
  }
  else if (auto wSearchPath = data.attributes.get_as<const wchar_t*>(AGT_ATTR_LIBRARY_PATH)) {
    customSearchPath = wSearchPath.value();
  }

  if (customSearchPath != nullptr)
    directoryCookie = AddDllDirectory(customSearchPath);

  bool usingCustomSearchPath = directoryCookie != nullptr;

  agt::init::module_exports moduleExports{};

  for (size_t i = 0; i < moduleNames.size(); ++i) {
    auto& mod = data.modules.emplace_back(moduleNames[i], data.attributes, usingCustomSearchPath);
    for (auto&& exp : mod.exports(moduleExports)) {
      auto [iter, isNew] = data.exports.try_emplace(exp.procName);
      if (isNew) {
        auto& expInfo = iter->get();
        expInfo.moduleIndex = i;
        expInfo.address     = exp.address;
        expInfo.tableOffset = exp.tableOffset;
      }
    }
  }

  if (directoryCookie != nullptr)
    RemoveDllDirectory(directoryCookie);
}



static HMODULE get_exe_handle() noexcept {
  return GetModuleHandle(nullptr);
}

static HMODULE get_dll_handle() noexcept {
  MEMORY_BASIC_INFORMATION info;
  size_t len = VirtualQueryEx(GetCurrentProcess(), (void*)get_dll_handle, &info, sizeof(info));
  if (len != sizeof(info))
    return nullptr;
  return (HMODULE)info.AllocationBase;
}



extern "C" {


AGT_static_api agt_config_t AGT_stdcall agt_get_config(agt_config_t parentConfig, int headerVersion) AGT_noexcept {

  HMODULE moduleHandle;
  agt_config_t config;
  agt_config_t baseLoader = parentConfig;

  agt::init::loader_shared *shared;

  if (config) {
    moduleHandle = get_dll_handle();
    while (baseLoader->parent != nullptr)
      baseLoader = baseLoader->parent;
    shared = baseLoader->shared;
    auto [modIter, modIsNew] = shared->loaderMap.try_emplace(moduleHandle, nullptr);
    if (!modIsNew)
      return modIter->second;
    config = new agt_config_st;
    shared = baseLoader->shared;
    modIter->second = config;
    config->childLoaders.push_back(config);
  } else {
    moduleHandle = get_exe_handle();
    config = new agt_config_st{};
    shared = new agt::init::loader_shared;
    shared->baseLoader = config;
    shared->requestedModules.insert_or_assign("core", AGT_INIT_REQUIRED);
    shared->loaderMap[moduleHandle] = config;
  }


  config->parent = config;
  config->headerVersion = headerVersion;
  config->loaderVersion = AGT_API_VERSION;
  config->loaderSize = sizeof(agt_config_st);
  config->pExportTable = agt::init::get_local_export_table();
  config->moduleHandle = moduleHandle;
  config->shared = shared;


  return config;
}


AGT_static_api void AGT_stdcall agt_config_init_modules(agt_config_t config, agt_init_necessity_t necessity, size_t moduleCount, const char *const *pModules) AGT_noexcept {
  if (config == nullptr)
    return;

  assert(config->shared);
  assert(moduleCount == 0 || pModules != nullptr);

  auto &modules = config->shared->requestedModules;

  for (size_t i = 0; i < moduleCount; ++i) {
    auto &&[iter, isNew] = modules.try_emplace(pModules[i], necessity);
    if (!isNew) {
      if (necessity < iter->get())
        iter->get() = necessity;
    }
  }
}

AGT_static_api void AGT_stdcall agt_config_init_user_modules(agt_config_t config, size_t userModuleCount, const agt_user_module_info_t *pUserModuleInfos) AGT_noexcept {
  if (config == nullptr)
    return;

  assert(config->shared);
  assert(userModuleCount == 0 || pUserModuleInfos != nullptr);

  config->shared->userModules.append(pUserModuleInfos, pUserModuleInfos + userModuleCount);
}

AGT_static_api agt_status_t AGT_stdcall agt_loader_set_attr(agt_config_t config, agt_init_necessity_t necessity, const agt_attr_t *pAttr, agt_init_attr_flags_t flags) AGT_noexcept {
}

AGT_static_api void AGT_stdcall agt_config_set_options(agt_config_t config, size_t attrCount, const agt_init_attr_t *pInitAttrs) AGT_noexcept {
}


AGT_static_api agt_bool_t AGT_stdcall agt_loader_set_default_allocation_params(agt_config_t config, const agt_allocator_params_t *pDefaultParams) AGT_noexcept {
  if (!config || config->shared->hasCustomAllocatorParams)
    return AGT_FALSE;

  if (pDefaultParams)
    config->shared->allocatorParams = *pDefaultParams;

  return AGT_TRUE;
}

AGT_static_api void AGT_stdcall agt_config_set_log_handler(agt_config_t config, agt_init_scope_t scope, agt_internal_log_handler_t handlerProc, void *userData) AGT_noexcept {
  if (!config)
    return;

  if (scope == AGT_INIT_SCOPE_PROCESS_WIDE) {
    config->shared->logHandler = handlerProc;
    config->shared->logHandlerUserData = userData;
  } else if (scope == AGT_INIT_SCOPE_LIBRARY_ONLY) {
    config->logHandler = handlerProc;
    config->logHandlerUserData = userData;
  }
}

}