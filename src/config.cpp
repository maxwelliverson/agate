//
// Created by maxwe on 2023-08-14.
//

#define AGT_BUILDING_STATIC_LOADER

#include "init.hpp"

#include "agate/dictionary.hpp"
#include "agate/vector.hpp"
#include "agate/map.hpp"
#include "agate/set.hpp"

#include "module_exports.hpp"

#include <memory>
#include <vector>
#include <string_view>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <core/ctx.hpp>
#include <core/instance.hpp>
#include <stringapiset.h>


namespace {


  std::optional<agt_value_t> convertToType(agt_value_type_t fromType, agt_value_type_t toType, agt_value_t value) noexcept {
    using convert_fn = agt_value_t(*)(agt_value_t);

    constexpr static convert_fn identity    = [](agt_value_t val){ return val; };
    constexpr static convert_fn bool_to_i32 = [](agt_value_t val) { return agt_value_t{ .int32 = val.boolean }; };
    constexpr static convert_fn bool_to_u32 = [](agt_value_t val) { return agt_value_t{ .uint32 = static_cast<bool>(val.boolean) }; };
    constexpr static convert_fn bool_to_i64 = [](agt_value_t val) { return agt_value_t{ .int64 = val.boolean }; };
    constexpr static convert_fn bool_to_u64 = [](agt_value_t val) { return agt_value_t{ .uint64 = static_cast<bool>(val.boolean) }; };
    constexpr static convert_fn f32_to_f64  = [](agt_value_t val){ return agt_value_t{ .float64 = val.float32 }; };
    constexpr static convert_fn i32_to_i64  = [](agt_value_t val) { return agt_value_t{ .int64 = val.int32 }; };
    constexpr static convert_fn u32_to_i64  = [](agt_value_t val){ return agt_value_t{ .int64 = val.uint32 }; };
    constexpr static convert_fn u32_to_u64  = [](agt_value_t val) { return agt_value_t{ .int64 = val.uint32 }; };
    constexpr static convert_fn i32_to_u64  = [](agt_value_t val) { return agt_value_t{ .uint64 = static_cast<uint64_t>(val.int32) }; };
    constexpr static convert_fn u32_to_i32  = [](agt_value_t val) { return agt_value_t{ .int32 = static_cast<int32_t>(val.uint32) }; };
    constexpr static convert_fn u64_to_i64  = [](agt_value_t val) { return agt_value_t{ .int64 = static_cast<int64_t>(val.uint64) }; };
    constexpr static convert_fn u32_to_f32  = [](agt_value_t val) { return agt_value_t{ .float32 = static_cast<float>(val.uint32) }; };
    constexpr static convert_fn i32_to_f32  = [](agt_value_t val) { return agt_value_t{ .float32 = static_cast<float>(val.int32) }; };
    constexpr static convert_fn u64_to_f32  = [](agt_value_t val) { return agt_value_t{ .float32 = static_cast<float>(val.uint64) }; };
    constexpr static convert_fn i64_to_f32  = [](agt_value_t val) { return agt_value_t{ .float32 = static_cast<float>(val.int64) }; };
    constexpr static convert_fn u32_to_f64  = [](agt_value_t val) { return agt_value_t{ .float64 = static_cast<double>(val.uint32) }; };
    constexpr static convert_fn i32_to_f64  = [](agt_value_t val) { return agt_value_t{ .float64 = static_cast<double>(val.int32) }; };
    constexpr static convert_fn u64_to_f64  = [](agt_value_t val) { return agt_value_t{ .float64 = static_cast<double>(val.uint64) }; };
    constexpr static convert_fn i64_to_f64  = [](agt_value_t val) { return agt_value_t{ .float64 = static_cast<double>(val.int64) }; };


    // unknown, bool, addr, string, wstring, u32, i32, u64, i64, f32, f64

    constexpr static convert_fn DispatchTable[] = {
    /*            unknown,     bool,     addr,   string,  wstring,         u32,         i32,         u64,         i64,        f32,        f64, */
    /* unknown */ nullptr,  nullptr,  nullptr,  nullptr,  nullptr,     nullptr,     nullptr,     nullptr,     nullptr,    nullptr,    nullptr,
    /* bool    */ nullptr, identity,  nullptr,  nullptr,  nullptr, bool_to_u32, bool_to_i32, bool_to_u64, bool_to_i64,    nullptr,    nullptr,
    /* addr    */ nullptr,  nullptr, identity,  nullptr,  nullptr,     nullptr,     nullptr,     nullptr,     nullptr,    nullptr,    nullptr,
    /* string  */ nullptr,  nullptr,  nullptr, identity,  nullptr,     nullptr,     nullptr,     nullptr,     nullptr,    nullptr,    nullptr,
    /* wstring */ nullptr,  nullptr,  nullptr,  nullptr, identity,     nullptr,     nullptr,     nullptr,     nullptr,    nullptr,    nullptr,
    /* u32     */ nullptr,  nullptr,  nullptr,  nullptr,  nullptr,    identity,  u32_to_i32,  u32_to_u64,  u32_to_i64, u32_to_f32, u32_to_f64,
    /* i32     */ nullptr,  nullptr,  nullptr,  nullptr,  nullptr,     nullptr,    identity,  i32_to_u64,  i32_to_i64, i32_to_f32, i32_to_f64,
    /* u64     */ nullptr,  nullptr,  nullptr,  nullptr,  nullptr,     nullptr,     nullptr,    identity,  u64_to_i64, u64_to_f32, u64_to_f64,
    /* i64     */ nullptr,  nullptr,  nullptr,  nullptr,  nullptr,     nullptr,     nullptr,     nullptr,    identity, i64_to_f32, i64_to_f64,
    /* f32     */ nullptr,  nullptr,  nullptr,  nullptr,  nullptr,     nullptr,     nullptr,     nullptr,     nullptr,   identity, f32_to_f64,
    /* f32     */ nullptr,  nullptr,  nullptr,  nullptr,  nullptr,     nullptr,     nullptr,     nullptr,     nullptr,    nullptr,   identity
    };

    if (const auto func = DispatchTable[toType + (fromType * AGT_VALUE_TYPE_COUNT)])
      return func(value);
    return std::nullopt;
  }

  std::partial_ordering compare(agt_value_type_t type, agt_value_t first, agt_value_t second) noexcept {
    switch (type) {
      case AGT_TYPE_BOOLEAN:
        return static_cast<bool>(first.boolean) <=> static_cast<bool>(second.boolean);
      case AGT_TYPE_ADDRESS:
        return std::partial_ordering::unordered; // no comparison of addresses :(
      case AGT_TYPE_STRING:
        return std::strcmp(first.string, second.string) <=> 0;
      case AGT_TYPE_WIDE_STRING:
        return std::wcscmp(first.wideString, second.wideString) <=> 0;
      case AGT_TYPE_UINT32:
        return first.uint32 <=> second.uint32;
      case AGT_TYPE_INT32:
        return first.int32 <=> second.int32;
      case AGT_TYPE_UINT64:
        return first.uint64 <=> second.uint64;
      case AGT_TYPE_INT64:
        return first.int64 <=> second.int64;
      case AGT_TYPE_FLOAT32:
        return first.float32 <=> second.float32;
      case AGT_TYPE_FLOAT64:
        return first.float64 <=> second.float64;
      AGT_no_default;
    }
  }



  std::partial_ordering try_compare(agt_value_type_t typeA, agt_value_t valueA, agt_value_type_t typeB, agt_value_t valueB) noexcept {
    if (typeA < AGT_VALUE_TYPE_COUNT && typeB < AGT_VALUE_TYPE_COUNT) [[likely]] {
      if (const auto convA = convertToType(typeA, typeB, valueA))
        return compare(typeB, *convA, valueB);
      if (const auto convB = convertToType(typeB, typeA, valueB))
        return compare(typeA, valueA, *convB);
    }
    return std::partial_ordering::unordered;
  }
}



struct agt::module_info {
  const char*   name;
  void*         handle;
  agt_flags32_t flags; // eg. IsIndirect (wasn't directly requested by any users, but was required by a loaded module).
};

struct agt::module_table {
  uint32_t    count;
  module_info modules[];
};







void agt::init::attributes::freeAll() noexcept {
  if (m_attrCount > 0 && m_destroyArrays)
    m_destroyArrays(m_attrType, m_attrValue);
  m_attrCount = 0;
}


[[nodiscard]] agt::init::environment::sysenv_result agt::init::environment::_sysenv_lookup(std::string_view key, char* buffer, size_t& length) noexcept {
#if defined(_WIN32)
  auto bufLength = static_cast<DWORD>(length);
  auto resultLength = GetEnvironmentVariable(key.data(), buffer, bufLength);
  if (resultLength == 0)
    return SYSENV_UNDEFINED_VARIABLE;

  length = resultLength;

  if (resultLength < bufLength)
    return SYSENV_SUCCESS;

  return SYSENV_INSUFFICIENT_BUFFER;

#else
#endif
}


[[nodiscard]] agt::init::environment::cookie_t agt::init::environment::_read_all_environment_variables(any_vector<std::pair<std::string_view, std::string_view>>& keyValuePairs) noexcept {
#if defined(_WIN32)
  const auto str = GetEnvironmentStrings();

  auto pos = str;

  while (*pos != '\0') {
    size_t entryLength = std::strlen(pos);
    auto entryEnd = pos + entryLength;
    auto identEnd = static_cast<char*>(std::memchr(pos, '=', entryLength));
    assert( identEnd != nullptr );
    std::string_view ident{ pos, identEnd };
    std::string_view value{ identEnd + 1, entryEnd };
    keyValuePairs.emplace_back(ident, value);

    pos = entryEnd + 1;
  }

  return str;
#else
#endif
}

void agt::init::environment::_release_cookie(cookie_t cookie) noexcept {
#if defined(_WIN32)
  auto result = FreeEnvironmentStrings(static_cast<LPCH>(cookie));
  AGT_assert( result );
#else
#endif
}



agt::init::module::module(dictionary_entry<agt_init_necessity_t>* name, init_manager& initManager, bool usingCustomSearchPaths) noexcept {
  char libName[MAX_PATH];

  m_name = name->get_key_data();
  m_errList = &initManager;
  m_necessity = name->get();
  m_handle = nullptr;

  if (m_necessity != AGT_INIT_UNNECESSARY) {
    constexpr static std::string_view LibPrefix = "agate-";
    strcpy_s(libName, LibPrefix.data());
    char* bufStart = &libName[LibPrefix.size()];
    constexpr static size_t BufSize = std::size(libName) - LibPrefix.size();
    if (const auto err = strcpy_s(bufStart, BufSize, m_name)) {
      initManager.reportModuleNameTooLong(m_name, MAX_PATH, err);
      return;
    }

    if (usingCustomSearchPaths)
      m_handle = LoadLibraryEx(libName, nullptr, LOAD_LIBRARY_SEARCH_USER_DIRS);
    else
      m_handle = LoadLibrary(libName);
  }
}


agt::init::module::~module() {
  freeHandle();
}


void agt::init::module::loadExports(module_exports& exp, const attributes& attr) const noexcept {

  if (!isValid())
    return;

  if (const auto getExportsFunc = GetProcAddress(static_cast<HMODULE>(m_handle), "agatelib_get_exports"))
    reinterpret_cast<get_exports_proc_t>(getExportsFunc)(exp, attr);
  else
    m_errList->reportModuleMissingGetExportsFunc(m_name);
}


void agt::init::module::configLibrary(library_configuration& out, agt_config_t config) const noexcept {
  if (!isValid())
    return;

  if (const auto configLibFunc = GetProcAddress(static_cast<HMODULE>(m_handle), "agatelib_config_library"))
    return reinterpret_cast<config_library_proc_t>(configLibFunc)(out, config, *m_errList);

  if (std::strcmp(m_name, "core") == 0)
    m_errList->reportCoreModuleMissingConfigLibraryFunc();
}

void agt::init::module::postInit(agt_ctx_t ctx) const noexcept {
  if (isValid())
    if (const auto postInitFunc = GetProcAddress(static_cast<HMODULE>(m_handle), "agatelib_post_init"))
      reinterpret_cast<post_init_proc_t>(postInitFunc)(ctx, *m_errList);
}

void agt::init::module::freeHandle() {
  if (m_handle && m_ownsHandle) {
    FreeLibrary(static_cast<HMODULE>(m_handle));
    m_handle = nullptr;
  }
}
















namespace {

  void freeModules(module_table* pTable) {
    if (pTable) {
      auto& table = *pTable;
      const uint32_t count = table.count;
      for (uint32_t i = 0; i < count; ++i) {
        auto& module = table.modules[i];
        if (module.handle) {
          FreeLibrary(static_cast<HMODULE>(module.handle));
          module.handle = nullptr;
        }
      }

      _aligned_free(pTable);
    }
  }

  void destroyInstanceManually(agt_instance_t instance) {
    auto exports = instance->exports;
    auto modules = exports->modules;
    exports->_pfn_destroy_instance(instance, false);
    freeModules(modules);
  }

  void CALLBACK freeModuleCallback(PTP_CALLBACK_INSTANCE instance, PVOID context) {
    struct callback_data {
      HANDLE  thread;
      HMODULE library;
    };
    const auto data = static_cast<callback_data*>(context);
    WaitForSingleObject(data->thread, INFINITE); // The thread was in the process of closing, so this shouldn't be long.
    FreeLibraryWhenCallbackReturns(instance, data->library);
    delete data;
  }

  void freeModulesOnThreadExit(module_table* pTable) {
    module_info coreModuleInfo{};

    if (pTable) {
      auto& table = *pTable;
      const uint32_t count = table.count;
      assert( count > 0 );
      coreModuleInfo = table.modules[0];

      // free in reverse of load order
      for (uint32_t i = count; i > 1;) {
        --i;
        auto& module = table.modules[i];
        if (module.handle) {
          FreeLibrary(static_cast<HMODULE>(module.handle));
          module.handle = nullptr;
        }
      }
      _aligned_free(pTable);

      // This stupid fucking workaround for the fact that a thread executing in a DLL cannot close that DLL while
      // doing thread_local destruction. We have to let it execute on a *new* thread,
      // and then use a special API to ensure that the library is closed when that thread returns. *Sigh*

      if (coreModuleInfo.handle) {
        struct callback_data {
          HANDLE  thread;
          HMODULE library;
        };
        auto data = new callback_data;
        data->thread = OpenThread(SYNCHRONIZE, TRUE, GetCurrentThreadId());
        assert ( data->thread != INVALID_HANDLE_VALUE );
        data->library = static_cast<HMODULE>(coreModuleInfo.handle);

        auto result = TrySubmitThreadpoolCallback(freeModuleCallback, data, nullptr);
        if (result == FALSE) {
          // idk fuckin die I guess
          abort();
        }
      }
    }
  }


  class loader {
    agt::init::init_manager          initManager;

    agt::vector<std::pair<const wchar_t*, agt_init_necessity_t>, 0> customSearchPaths;
    agt::vector<std::unique_ptr<wchar_t[]>, 0> dynPtrs;
    agt::vector<DLL_DIRECTORY_COOKIE, 0>       cookies;
    const void*                                searchPaths = nullptr;


    agt::init::module_exports        exports;
    agt::vector<agt::init::module>   loadedModules;
    agt::init::library_configuration libraryConfig;
    agt::init::attributes            attributes;

    agt::proc_entry*                 procEntryTable = nullptr;
    size_t                           procTableSize = 0;

    agt_instance_t                   instance;


    init::undo_cookie_t              moduleFreeCookie = { };
    init::undo_cookie_t              instanceCleanupCookie = { };



    void addUtf8SearchPath(std::string_view path, agt_init_necessity_t necessity) noexcept {
      auto& dynPtr = dynPtrs.emplace_back(std::make_unique<wchar_t[]>(path.size() + 1));
      wchar_t* wBuffer = dynPtr.get();
      const auto cStrLength = static_cast<int>(path.size());
      const int result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), cStrLength, wBuffer, cStrLength + 1);
      wBuffer[result] = L'\0';
      if (result == 0)
        initManager.reportBadUtf8Encoding(path.data(), necessity);
      else
        customSearchPaths.emplace_back(wBuffer, necessity);
    }

    void addSearchPathOption(const agt_config_option_t& option) noexcept {
      if (option.type == AGT_TYPE_STRING)
        addUtf8SearchPath(option.value.string, option.necessity);
      else if (option.type == AGT_TYPE_WIDE_STRING)
        customSearchPaths.emplace_back(option.value.wideString, option.necessity);
      else
        initManager.reportOptionBadType(option, { AGT_TYPE_STRING, AGT_TYPE_WIDE_STRING });
    }

    void addSearchPathsFromConfig(agt_config_t config) noexcept {
      for (auto&& option : config->options) {
        if (option.id == AGT_CONFIG_SEARCH_PATH) {
          addSearchPathOption(option);
        }
      }

      for (const auto child : config->childLoaders)
        addSearchPathsFromConfig(child);
    }

    const void* concatenatedSearchPaths() noexcept {
      if (searchPaths || customSearchPaths.empty())
        return searchPaths;

      bool requiresSeperator = false;
      agt::vector<wchar_t, 0> buffer;

      for (const auto path : customSearchPaths | std::views::keys) {
        if (requiresSeperator) {
          buffer.push_back(';');
          buffer.push_back(' ');
        }
        const size_t length = std::wcslen(path);
        buffer.append(path, path + length);
        requiresSeperator = true;
      }


      auto& dynPtr = dynPtrs.emplace_back(std::make_unique<wchar_t[]>(buffer.size() + 1));
      std::wmemcpy(dynPtr.get(), buffer.data(), buffer.size());
      dynPtr[buffer.size()] = 0;

      searchPaths = dynPtr.get();
      return searchPaths;
    }


    bool isValidSearchPath(const wchar_t* path, agt_init_necessity_t necessity) noexcept {
      const auto attributes = GetFileAttributesW(path);
      if (attributes == INVALID_FILE_ATTRIBUTES) {
        initManager.reportInvalidPathWin32(path, necessity, GetLastError());
        return false;
      }

      const bool result = attributes & FILE_ATTRIBUTE_DIRECTORY;

      if (!result)
        initManager.reportPathIsNotADirectory(path, necessity);

      return result;
    }







    void pushUserSearchPaths(agt_config_t config) noexcept {

      // do something to get a search path from environment so long as environment override is allowed.

      addSearchPathsFromConfig(config);

      if (const auto searchPathEnvVar = libraryConfig.env().get("AGT_MODULE_SEARCH_PATH"))
        addUtf8SearchPath(*searchPathEnvVar, AGT_INIT_HINT);

      for (auto [customSearchPath, necessity] : customSearchPaths) {
        if (isValidSearchPath(customSearchPath, necessity))
          cookies.push_back(AddDllDirectory(customSearchPath));
      }
    }

    void popUserSearchPaths() noexcept {
      for (const auto cookie : cookies)
        RemoveDllDirectory(cookie);

      customSearchPaths.clear();
      dynPtrs.clear();
      cookies.clear();
    }



    // Returns false if the initialization process needs to die early
    bool loadModuleFiles(agt_config_t config) noexcept {

      // config->shared->requestedModules["core"] = AGT_INIT_REQUIRED;

      agt::vector<agt::dictionary_entry<agt_init_necessity_t>*> requestedModules;
      requestedModules.reserve(config->shared->requestedModules.size());

      for (auto&& module : config->shared->requestedModules) {
        requestedModules.push_back(&module);
        if (module.key() == "core") {
          requestedModules.back() = requestedModules.front();
          requestedModules.front() = &module;
        }
      }

      pushUserSearchPaths(config);

      for (auto moduleEntry : requestedModules) {
        auto& module = loadedModules.emplace_back(moduleEntry, initManager, !customSearchPaths.empty());

        if (!module.isValid())
          initManager.reportModuleNotFound(module.name(), module.necessity(), concatenatedSearchPaths());
      }

      popUserSearchPaths();

      moduleFreeCookie = initManager.registerCleanup([this] {
        for (auto&& mod : loadedModules)
          mod.freeHandle();
      });

      return !initManager.isFatal();
    }

    // Returns false if the initialization process should return early
    bool configureLibrary(agt::init::module& mod, agt_config_t config) noexcept {
      mod.configLibrary(libraryConfig, config);
      return !initManager.isFatal();
    }

    bool initializeAttributes() noexcept {
      attributes.initialize(libraryConfig, initManager);
      return !initManager.isFatal();
    }

    void getExportsFromEachModule() noexcept {
      getVirtualExports();
      for (auto&& module : loadedModules)
        loadExportsFromModule(module);
    }

    void getVirtualExports() noexcept {
      auto& moduleExports = exports;
      AGT_add_virtual_export(finalize);
      AGT_add_virtual_export(acquire_ctx);
      // AGT_add_private_export(free_modules, freeModulesCallback);
      AGT_add_private_export(free_modules_on_thread_exit, freeModulesOnThreadExit);
      AGT_add_virtual_export(get_current_fiber);
      AGT_add_virtual_export(get_fiber_data);
      AGT_add_virtual_export(set_fiber_data);
    }

    void loadExportsFromModule(agt::init::module& mod) noexcept {
      mod.loadExports(exports, attributes);
    }

    bool generateProcEntryTable() noexcept {
      std::ranges::sort(exports, [](const char* a, const char* b){ return std::strcmp(a, b) < 0; }, &agt::init::export_info::procName);
      const auto procTableSize = std::ranges::count_if(exports, [](const agt::init::export_info& exp) {
        return exp.flags & agt::init::eExportIsPublic;
      });

      this->procTableSize = procTableSize;
      auto procTable = new agt::proc_entry[procTableSize];
      auto tableEntry = exports.begin();
      const auto tableEnd = exports.end();
      for (size_t i = 0; tableEntry != tableEnd; ++tableEntry) {
        if (tableEntry->flags & agt::init::eExportIsPublic) {
          auto& procTableEntry = procTable[i++];
          procTableEntry.address = tableEntry->address;
          if (strcpy_s(procTableEntry.name, tableEntry->procName)) {
            initManager.reportExportTooLong(tableEntry->procName, std::size(procTableEntry.name));
            std::memset(procTableEntry.name, 0, std::size(procTableEntry.name) * sizeof(procTableEntry.name[0]));
            procTableEntry.address = nullptr;
          }
        }
      }
      this->procEntryTable = procTable;

      initManager.registerCleanup([this] {
        delete[] procEntryTable;
      });

      return !initManager.isFatal();
    }

    agt_instance_t makeInstance(agt_config_t config) noexcept {

      const auto exportTable = config->pExportTable;

      if (const auto instance = exportTable->_pfn_create_instance(config, initManager)) {
        instanceCleanupCookie = initManager.registerCleanup(destroyInstanceManually, instance);
        initManager.unregister(moduleFreeCookie);
        return instance;
      }
      return nullptr;
    }

    agt::module_table* makeModuleTable() noexcept {
      uint32_t moduleCount = loadedModules.size();
      size_t tableSize   = sizeof(module_table) + (moduleCount * sizeof(module_info));
      tableSize = align_to<alignof(module_table)>(tableSize);
      void* mem = _aligned_malloc(tableSize, alignof(module_table));
      auto table = new(mem) module_table;
      table->count = moduleCount;

      for (uint32_t i = 0; i < moduleCount; ++i) {
        auto& entry = table->modules[i];
        auto& module = loadedModules[i];
        entry.name = module.name();
        entry.handle = module.releaseHandle();
        entry.flags = 0; // for now
      }

      return table;
    }

    bool copyExportTable(agt_config_t config, export_table* directParent, export_table*& src) noexcept {
      auto dst = config->pExportTable;

      size_t dstSize = config->exportTableSize;
      size_t srcSize = src->tableSize;

      std::memcpy(dst, src, (std::min)(dstSize, srcSize));
      if (srcSize < dstSize) {
        std::memset(reinterpret_cast<std::byte*>(dst) + srcSize, 0, dstSize - srcSize); // zero the rest of the memory if src was smaller
        src = dst;
      }

      dst->refCount = 0;
      auto rootTable = directParent->parentExportTable;
      dst->parentExportTable = rootTable;
      rootTable->refCount += 1;

      dst->headerVersion = agt::version::from_integer(config->headerVersion);
      dst->tableSize = dstSize;

      if (config->logHandler != nullptr) {
        dst->logHandler = config->logHandler;
        dst->logHandlerData = config->logHandlerUserData;
      }
      else {
        dst->logHandler = dst->instance->logCallback;
        dst->logHandlerData = dst->instance->logCallbackUserData;
      }

      if (dst == src) { // this was replaced as the largest, so there are potentially some functions that couldn't fit into previous tables that can fit into this one. Therefore, we have to slot those ones in.
        const size_t prevMaxOffset = dst->maxValidOffset;
        size_t newMaxOffset = prevMaxOffset;

        const static size_t MinTableOffset = offsetof(agt::export_table, _pfn_create_instance);
        const size_t MaxTableOffset = dstSize;
        constexpr static size_t MinTableAlign = alignof(void*);

        const auto exportTableAddress = reinterpret_cast<std::byte*>(dst);

        for (auto&& exp : exports) {
          if (exp.flags & agt::init::eExportHasTableSlot) {
            if (MinTableOffset <= exp.tableOffset && exp.tableOffset < MaxTableOffset && (exp.tableOffset % MinTableAlign == 0)) [[likely]] {
              if (prevMaxOffset < exp.tableOffset) {
                *reinterpret_cast<agt_proc_t*>(std::assume_aligned<MinTableAlign>(exportTableAddress + exp.tableOffset)) = exp.address;
                if (newMaxOffset < exp.tableOffset)
                  newMaxOffset = exp.tableOffset;
              }
            }
            else
              initManager.reportInvalidFunctionIndex(exp);
          }
        }

        dst->maxValidOffset = newMaxOffset;
      }

      for (const auto child : config->childLoaders) {
        if (not copyExportTable(child, dst, src))
          return false;
      }

      return true;
    }

    bool initExportTables(agt_config_t config) noexcept {
      auto exportTable = config->pExportTable;

      std::memset(exportTable, 0, config->exportTableSize); // this memory *should* be zeroed by the system, but better safe than sorry

      exportTable->refCount = 1;
      exportTable->headerVersion = agt::version::from_integer(config->headerVersion);
      exportTable->loaderVersion = agt::version::from_integer(AGT_API_VERSION);
      exportTable->effectiveLibraryVersion = agt::version::from_integer(attributes.get_as<uint32_t>(AGT_ATTR_LIBRARY_VERSION).value_or(0));

      exportTable->userModules = nullptr;

      exportTable->pProcSet = procEntryTable;
      exportTable->procSetSize = static_cast<uint32_t>(procTableSize);
      exportTable->parentExportTable = exportTable; // eventually this will be set to null, but for now, set it to itself, so child export tables can simply copy the field of their parent

      std::tie(exportTable->attrCount, exportTable->attrTypes, exportTable->attrValues) = attributes.copy();




      const static size_t MinTableOffset = offsetof(agt::export_table, _pfn_create_instance);
      const size_t MaxTableOffset = config->exportTableSize;
      constexpr static size_t MinTableAlign = alignof(void*);


      exportTable->tableSize = static_cast<agt_u32_t>(MaxTableOffset);

      size_t maxOffset = 0;


      const auto exportTableAddress = reinterpret_cast<std::byte*>(exportTable);

      for (auto&& exp : exports) {
        if (exp.flags & agt::init::eExportHasTableSlot) {
          if (MinTableOffset <= exp.tableOffset && exp.tableOffset < MaxTableOffset && (exp.tableOffset % MinTableAlign == 0)) [[likely]] {
            *reinterpret_cast<agt_proc_t*>(std::assume_aligned<MinTableAlign>(exportTableAddress + exp.tableOffset)) = exp.address;
            if (maxOffset < exp.tableOffset)
              maxOffset = exp.tableOffset;
          }
          else
            initManager.reportInvalidFunctionIndex(exp);
        }
      }

      exportTable->maxValidOffset = static_cast<uint32_t>(maxOffset);


      // Make the instance now; we need to have filled the export table before doing so
      instance = makeInstance(config);

      if (!instance)
        return false;

      exportTable->modules = makeModuleTable();

      exportTable->instance = instance;
      if (config->logHandler != nullptr) {
        exportTable->logHandler = config->logHandler;
        exportTable->logHandlerData = config->logHandlerUserData;
      }
      else {
        exportTable->logHandler = instance->logCallback;
        exportTable->logHandlerData = instance->logCallbackUserData;
      }

      agt::export_table* maxSizeExportTable = exportTable;

      for (auto child : config->childLoaders) {
        if (!copyExportTable(child, exportTable, maxSizeExportTable))
          return false;
      }

      exportTable->parentExportTable = nullptr;

      return !initManager.isFatal();
    }


    agt_ctx_t makeInitialContext() noexcept {

      auto exportTable = instance->exports;

      if (const auto ctx = exportTable->_pfn_do_acquire_ctx(instance, nullptr)) {
        initManager.registerCleanup([exportTable, ctx] {
          const auto instance = ctx->instance;
          if ( exportTable->_pfn_release_ctx(ctx) )
            destroyInstanceManually(instance);
        });
        initManager.unregister(instanceCleanupCookie);
        return ctx;
      }

      return nullptr;
    }

    bool resolveModules(agt_ctx_t ctx) noexcept {
      for (auto&& mod : loadedModules)
        mod.postInit(ctx);
      return !initManager.isFatal();
    }

    static void resolveConfigWithContext(agt_config_t config, agt_ctx_t ctx) noexcept {
      if (config->pCtxResult)
        *config->pCtxResult = ctx;
      else if (config->initCallback)
        config->initCallback(ctx, config->callbackUserData);

      for (auto child : config->childLoaders)
        resolveConfigWithContext(child, ctx);
    }



    [[nodiscard]] agt_status_t abort() noexcept {
      return initManager.finish();
    }

    [[nodiscard]] agt_status_t complete() noexcept {
      initManager.complete();
      return initManager.finish();
    }
  public:


    agt_status_t initFromRootConfig(agt_config_t config) noexcept {

      if (not loadModuleFiles(config))
        return abort();


      for (auto&& mod : loadedModules) {
        if (not configureLibrary(mod, config))
          return abort();
      }


      if (not initializeAttributes())
        return abort();


      getExportsFromEachModule();


      if (not generateProcEntryTable())
        return abort();


      if (not initExportTables(config))
        return abort();


      auto ctx = makeInitialContext();

      if (!ctx || !resolveModules(ctx))
        return abort();

      resolveConfigWithContext(config, ctx);

      return complete();

      //   compile a list of all required modules

      //   load modules

      //   if a required module could not be loaded, jump to end.

      //   call library config functions

      //   If errorList has failed (generally should only happen if the core module does not export agatelib_config_library), jump to end.

      //   generate attributes based on library configuration

      //   load exports from each module, organize these exports

      //   create single instance object

      //   fill export table for each config object in the loader tree

      //   create ctx for local thread

      //   try calling the post initialization hook for each loaded module.

      // end:
      //   dump errorList
      //   return final error code given by errorList.




    }
  };





}




extern "C" {

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


AGT_static_api void         AGT_stdcall agt_config_set_options(agt_config_t config, size_t optionCount, const agt_config_option_t* pConfigOptions) AGT_noexcept {
  if (!config || !optionCount || !pConfigOptions)
    return;
  config->options.append(pConfigOptions, pConfigOptions + optionCount);
}

AGT_static_api agt_bool_t AGT_stdcall agt_loader_set_default_allocation_params(agt_config_t config, const agt_allocator_params_t *pDefaultParams) AGT_noexcept {
  if (!config || config->shared->hasCustomAllocatorParams)
    return AGT_FALSE;

  if (pDefaultParams) {
    config->shared->allocatorParams = *pDefaultParams;
    config->shared->hasCustomAllocatorParams = true;
  }

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



AGT_static_api agt_status_t AGT_stdcall _agt_default_init(agt_ctx_t* pCtx, int headerVersion) AGT_noexcept {
  return agt_init(pCtx, _agt_get_config(AGT_ROOT_CONFIG, headerVersion));
}


AGT_static_api agt_status_t AGT_stdcall agt_init(agt_ctx_t* pLocalContext, agt_config_t config) AGT_noexcept {
  if (!pLocalContext || !config)
    return AGT_ERROR_INVALID_ARGUMENT;

  config->pCtxResult = pLocalContext;

  if (config->parent != nullptr)
    return AGT_DEFERRED;

  return loader{}.initFromRootConfig(config);
}

AGT_static_api agt_status_t AGT_stdcall agt_init_with_callback(agt_config_t config, agt_init_callback_t callback, void* userData) AGT_noexcept {
  if (!config)
    return AGT_ERROR_INVALID_ARGUMENT;

  config->initCallback = callback;
  config->callbackUserData = userData;

  if (config->parent != nullptr)
    return AGT_DEFERRED;

  return loader{}.initFromRootConfig(config);
}

AGT_static_api agt_status_t AGT_stdcall _agt_default_init_with_callback(int headerVersion, agt_init_callback_t callback, void* userData) AGT_noexcept {
  return agt_init_with_callback(_agt_get_config(AGT_ROOT_CONFIG, headerVersion), callback, userData);
}


AGT_core_api   agt_status_t AGT_stdcall agt_finalize(agt_ctx_t ctx) AGT_noexcept {
  // if ( ctx == nullptr )
  // ctx = agt::get_ctx();
  // if ( ctx == nullptr )
  // return AGT_ERROR_INVALID_ARGUMENT;

  if (!ctx)
    return AGT_ERROR_INVALID_ARGUMENT;
  auto instance = ctx->instance;
  const auto exports = instance->exports;
  if ( exports->_pfn_release_ctx(ctx) )
    destroyInstanceManually(instance);

  return AGT_SUCCESS;

  // return g_lib._pfn_finalize(ctx);
}


}