//
// Created by maxwe on 2023-08-14.
//

#ifndef AGATE_INTERNAL_INIT_HPP
#define AGATE_INTERNAL_INIT_HPP

#include "config.hpp"

#include "agate/codex.hpp"
#include "agate/dictionary.hpp"
#include "agate/vector.hpp"
#include "agate/map.hpp"
#include "agate/set.hpp"
#include "agate/string_pool.hpp"

#include <algorithm>
#include <functional>
#include <span>
#include <variant>
#include <optional>

/*template <>
struct agt::key_info<agt_attr_id_t> {
  [[nodiscard]] static uint32_t get_hash_value(agt_attr_id_t attrId) noexcept {
    *//*return (((uint32_t)attrId) >> 4) ^ (((uint32_t)attrId) >> 9);
    return (unsigned((uintptr_t)val) >> 4) ^
           (unsigned((uintptr_t)val) >> 9);*//*
  }
};*/

namespace agt::init {

  using attribute_variant = std::variant<
      bool,
      const void*,
      const char*,
      const wchar_t*,
      uint32_t,
      int32_t,
      uint64_t,
      int64_t,
      float,
      double>;

  template <typename T>
  concept attribute_visitor = requires(T&& fn, const attribute_variant& var) {
    std::visit(std::forward<T>(fn), var);
  };

  namespace dtl {
    template <agt_value_type_t Val>
    struct attr_enum_to;
    template <>
    struct attr_enum_to<AGT_TYPE_BOOLEAN> { using type = bool; };
    template <>
    struct attr_enum_to<AGT_TYPE_ADDRESS> { using type = const void*; };
    template <>
    struct attr_enum_to<AGT_TYPE_STRING> { using type = const char*; };
    template <>
    struct attr_enum_to<AGT_TYPE_WIDE_STRING> { using type = const wchar_t*; };
    template <>
    struct attr_enum_to<AGT_TYPE_UINT32> { using type = uint32_t; };
    template <>
    struct attr_enum_to<AGT_TYPE_UINT64> { using type = uint64_t; };
    template <>
    struct attr_enum_to<AGT_TYPE_INT32> { using type = int32_t; };
    template <>
    struct attr_enum_to<AGT_TYPE_INT64> { using type = int64_t; };
    template <>
    struct attr_enum_to<AGT_TYPE_FLOAT32> { using type = float; };
    template <>
    struct attr_enum_to<AGT_TYPE_FLOAT64> { using type = double; };

    template <agt_value_type_t Val>
    using value_type_t = typename attr_enum_to<Val>::type;

    template <typename T>
    struct attr_type_to;
    template <>
    struct attr_type_to<bool> { inline constexpr static agt_value_type_t value = AGT_TYPE_BOOLEAN; };
    template <>
    struct attr_type_to<const void*> { inline constexpr static agt_value_type_t value = AGT_TYPE_ADDRESS; };
    template <>
    struct attr_type_to<const char*> { inline constexpr static agt_value_type_t value = AGT_TYPE_STRING; };
    template <>
    struct attr_type_to<const wchar_t*> { inline constexpr static agt_value_type_t value = AGT_TYPE_WIDE_STRING; };
    template <>
    struct attr_type_to<uint32_t> { inline constexpr static agt_value_type_t value = AGT_TYPE_UINT32; };
    template <>
    struct attr_type_to<int32_t> { inline constexpr static agt_value_type_t value = AGT_TYPE_INT32; };
    template <>
    struct attr_type_to<version> { inline constexpr static agt_value_type_t value = AGT_TYPE_INT32; };
    template <>
    struct attr_type_to<uint64_t> { inline constexpr static agt_value_type_t value = AGT_TYPE_UINT64; };
    template <>
    struct attr_type_to<int64_t> { inline constexpr static agt_value_type_t value = AGT_TYPE_INT64; };
    template <>
    struct attr_type_to<float> { inline constexpr static agt_value_type_t value = AGT_TYPE_FLOAT32; };
    template <>
    struct attr_type_to<double> { inline constexpr static agt_value_type_t value = AGT_TYPE_FLOAT64; };

    template <typename T>
    inline constexpr static agt_value_type_t value_type_enum_v = attr_type_to<T>::value;

    template <typename T>
    struct alignas(uintptr_t) aligned_type {
      T value;
    };

    template <typename T>
    inline static T attr_cast(uintptr_t value) noexcept {
      if constexpr (sizeof(T) == sizeof(uintptr_t))
        return std::bit_cast<T>(value);
      else
        return *reinterpret_cast<const T*>(&value);
    }

    template <typename T>
    inline static agt_value_t value_cast(T val) noexcept {
      aligned_type<T> aligned{ val };
      return std::bit_cast<agt_value_t>(aligned);
    }
  }

  class library_configuration;
  class init_manager;

  struct config_options;

  class attributes {

    friend struct config_options;

    size_t                  m_attrCount = 0;
    const agt_value_type_t* m_attrType = nullptr;
    const uintptr_t*        m_attrValue = nullptr;
    bool                    m_usingNativeUnit = false;
    void                 (* m_destroyArrays)(const agt_value_type_t* pTypes, const uintptr_t* pAttrs);

  public:

    attributes() = default;

    ~attributes() {
      freeAll();
    }


    void initialize(const library_configuration& libConfig, init_manager& initManager) noexcept;

    template <attribute_visitor V>
    auto visit(agt_attr_id_t attrId, V&& v) const noexcept {
      return get(attrId).transform([&](attribute_variant var) mutable {
        return std::visit(std::forward<V>(v), var);
      });
    }

    [[nodiscard]] std::optional<attribute_variant> get(agt_attr_id_t attrId) const noexcept {
      attribute_variant var;
      if (!(attrId < m_attrCount))
        return std::nullopt;

      uintptr_t val = m_attrValue[attrId];

#define AGT_attr_as(type_) attribute_variant(std::in_place_type<type_>, (type_)val)

#define AGT_case_attr_type(type_) case dtl::value_type_enum_v<type_>: return attribute_variant(std::in_place_type<type_>, dtl::attr_cast<type_>(val))

      switch (m_attrType[attrId]) {
        AGT_case_attr_type(bool);
        AGT_case_attr_type(const void*);
        AGT_case_attr_type(const char*);
        AGT_case_attr_type(const wchar_t*);
        AGT_case_attr_type(uint32_t);
        AGT_case_attr_type(int32_t);
        AGT_case_attr_type(uint64_t);
        AGT_case_attr_type(int64_t);
        AGT_case_attr_type(float);
        AGT_case_attr_type(double);
        case AGT_TYPE_UNKNOWN:
          return std::nullopt;
      }
    }

    template <typename T>
    [[nodiscard]] std::optional<T> get_as(agt_attr_id_t attrId) const noexcept {
      if (attrId < m_attrCount && m_attrType[attrId] == dtl::value_type_enum_v<T>)
        return dtl::attr_cast<T>(m_attrValue[attrId]);
      return std::nullopt;
    }

    [[nodiscard]] size_t size() const noexcept {
      return m_attrCount;
    }

    [[nodiscard]] std::tuple<uint32_t, const agt_value_type_t*, const uintptr_t*> copy() const noexcept {
      auto pValues = new uintptr_t[m_attrCount];
      auto pTypes = new agt_value_type_t[m_attrCount];
      std::uninitialized_copy_n(m_attrValue, m_attrCount, pValues);
      std::uninitialized_copy_n(m_attrType, m_attrCount, pTypes);
      return { static_cast<uint32_t>(m_attrCount), pTypes, pValues };
    }

    void freeAll() noexcept;


    [[nodiscard]] bool using_native_unit() const noexcept {
      return m_usingNativeUnit;
    }
  };

  inline constexpr static uint32_t eExportIsPublic   = 0x1; // Export has a slot in the proc lookup table (can be found by agt_get_proc_address)
  inline constexpr static uint32_t eExportHasTableSlot = 0x2; // Export has a slot in the export table


  inline constexpr static uint32_t ePublicExport  = eExportIsPublic | eExportHasTableSlot;
  inline constexpr static uint32_t ePrivateExport = eExportHasTableSlot;
  inline constexpr static uint32_t eVirtualExport = eExportIsPublic;

  struct export_info {
    const char* procName;
    agt_proc_t  address;
    size_t      tableOffset;
    uint32_t    flags;
  };


  class environment {
    string_pool               m_envVarValuePool;
    dictionary<pooled_string> m_envVarCache;
    bool                      m_hasPrefetched = false;



    using cookie_t = void*;

    enum sysenv_result {
      SYSENV_UNDEFINED_VARIABLE,
      SYSENV_SUCCESS,
      SYSENV_INSUFFICIENT_BUFFER
    };


    [[nodiscard]] static sysenv_result _sysenv_lookup(std::string_view key, char* buffer, size_t& length) noexcept;

    sysenv_result (* m_lookup)(std::string_view key, char* buffer, size_t& length) noexcept = _sysenv_lookup;



    [[nodiscard]] pooled_string _lookup_value(std::string_view key) noexcept {
      constexpr static size_t StaticBufferSize = 256;
      char staticBuffer[StaticBufferSize];
      std::unique_ptr<char[]> dynBuffer;
      char*  data   = staticBuffer;
      size_t length = StaticBufferSize;


      do {
        switch (m_lookup(key, data, length)) {
          case SYSENV_UNDEFINED_VARIABLE:
            return {};
          case SYSENV_SUCCESS:
            return m_envVarValuePool.get(std::string_view{ data, length });
          case SYSENV_INSUFFICIENT_BUFFER:
            dynBuffer = std::make_unique<char[]>(length);
            data = dynBuffer.get();
        }
      } while(true);
    }


    [[nodiscard]] static cookie_t _read_all_environment_variables(any_vector<std::pair<std::string_view, std::string_view>>& keyValuePairs) noexcept;

    static void _release_cookie(cookie_t cookie) noexcept;

  public:
    void prefetch_all() noexcept {
      if (!m_hasPrefetched) {
        vector<std::pair<std::string_view, std::string_view>> keyValuePairs;
        auto cookie = _read_all_environment_variables(keyValuePairs);

        for (auto&& [key, value] : keyValuePairs) {
          auto [iter, isNew] = m_envVarCache.try_emplace(key);
          if (isNew)
            iter->get() = m_envVarValuePool.get(value);
        }

        _release_cookie(cookie);
        m_hasPrefetched = true;
      }
    }

    [[nodiscard]] std::optional<std::string_view> get(std::string_view key) noexcept {
      auto [iter, isNew] = m_envVarCache.try_emplace(key);
      if (isNew) {
        if (m_hasPrefetched)
          return std::nullopt;
        iter->get() = _lookup_value(key);
      }
      if (!iter->get().is_valid())
        return std::nullopt;
      return iter->get().view();
    }
  };






  class library_configuration {

    friend class attributes;

    struct var_t {
      agt_value_type_t type;
      agt_value_t      value;
    };

    map<agt_attr_id_t, var_t>     m_attributes;
    const agt_allocator_params_t* m_allocParams = nullptr;
    agt_attr_id_t                 m_maxKnownId = { };
    vector<agt_u32_t, 0>          m_unknownAttributes;
    mutable environment           m_env;

    config_options*               m_options = nullptr;
    void                       (* m_optionsDtor)(config_options*) = nullptr;
    void                       (* m_pfnCompileOptions)(config_options*, attributes& attr, environment& env, init_manager& initManager) = nullptr;
  public:

    ~library_configuration() {
      if (m_optionsDtor && m_options)
        m_optionsDtor(m_options);
    }

    void init_options(init_manager& initManager) noexcept;

    void set_max_known_attribute(agt_attr_id_t attrId) noexcept {
      m_maxKnownId = attrId;
    }

    void set_attribute(agt_attr_id_t attrId, bool boolean) noexcept {
      m_attributes[attrId] = var_t{ AGT_TYPE_BOOLEAN, { .boolean = boolean } };
      if (m_maxKnownId < attrId)
        m_maxKnownId = attrId;
    }

    void set_attribute(agt_attr_id_t attrId, const void* address) noexcept {
      m_attributes[attrId] = var_t{ AGT_TYPE_ADDRESS, { .address = address } };
      if (m_maxKnownId < attrId)
        m_maxKnownId = attrId;
    }

    void set_attribute(agt_attr_id_t attrId, const char* string) noexcept {
      m_attributes[attrId] = var_t{ AGT_TYPE_STRING, { .string = string } };
      if (m_maxKnownId < attrId)
        m_maxKnownId = attrId;
    }

    void set_attribute(agt_attr_id_t attrId, const wchar_t* wideString) noexcept {
      m_attributes[attrId] = var_t{ AGT_TYPE_WIDE_STRING, { .wideString = wideString } };
      if (m_maxKnownId < attrId)
        m_maxKnownId = attrId;
    }

    void set_attribute(agt_attr_id_t attrId, int32_t int32) noexcept {
      m_attributes[attrId] = var_t{ AGT_TYPE_INT32, { .int32 = int32 } };
      if (m_maxKnownId < attrId)
        m_maxKnownId = attrId;
    }

    void set_attribute(agt_attr_id_t attrId, uint32_t uint32) noexcept {
      m_attributes[attrId] = var_t{ AGT_TYPE_UINT32, { .uint32 = uint32 } };
      if (m_maxKnownId < attrId)
        m_maxKnownId = attrId;
    }

    void set_attribute(agt_attr_id_t attrId, int64_t int64) noexcept {
      m_attributes[attrId] = var_t{ AGT_TYPE_INT64, { .int64 = int64 } };
      if (m_maxKnownId < attrId)
        m_maxKnownId = attrId;
    }

    void set_attribute(agt_attr_id_t attrId, uint64_t uint64) noexcept {
      m_attributes[attrId] = var_t{ AGT_TYPE_UINT64, { .uint64 = uint64 } };
      if (m_maxKnownId < attrId)
        m_maxKnownId = attrId;
    }

    void set_attribute(agt_attr_id_t attrId, float float32) noexcept {
      m_attributes[attrId] = var_t{ AGT_TYPE_FLOAT32, { .float32 = float32 } };
      if (m_maxKnownId < attrId)
        m_maxKnownId = attrId;
    }

    void set_attribute(agt_attr_id_t attrId, double float64) noexcept {
      m_attributes[attrId] = var_t{ AGT_TYPE_FLOAT64, { .float64 = float64 } };
      if (m_maxKnownId < attrId)
        m_maxKnownId = attrId;
    }


    [[nodiscard]] environment& env() const noexcept {
      return m_env;
    }

    [[nodiscard]] config_options& options() const noexcept {
      assert( m_options != nullptr );
      return *m_options;
    }


    [[nodiscard]] bool attribute_is_set(agt_attr_id_t attrId) const noexcept {
      return m_attributes.count(attrId);
    }
  };


  class module_exports {

    agt::vector<export_info> m_exports;

  public:

    using iterator = export_info*;
    using const_iterator = const export_info*;



    module_exports() noexcept = default;

    module_exports(const module_exports&) = delete;
    module_exports(module_exports&&) noexcept = delete;


    template <typename Ret, typename ...Args>
    void add_export(uint32_t kind, const char* name, Ret(* procAddress)(Args...), size_t tableOffset) noexcept {
      auto& info       = m_exports.emplace_back();
      info.procName    = name;
      info.address     = (agt_proc_t)procAddress;
      info.tableOffset = tableOffset;
      info.flags       = kind;
    }

    template <typename Ret, typename ...Args>
    void add_virtual_export(const char* name, Ret(* procAddress)(Args...)) noexcept {
      auto& info       = m_exports.emplace_back();
      info.procName    = name;
      info.address     = (agt_proc_t)procAddress;
      info.tableOffset = 0; // ignore
      info.flags       = eVirtualExport;
    }


    [[nodiscard]] std::span<export_info> exports() noexcept {
      return m_exports;
    }

    [[nodiscard]] std::span<const export_info> exports() const noexcept {
      return m_exports;
    }

    [[nodiscard]] iterator begin() noexcept {
      return m_exports.data();
    }
    [[nodiscard]] iterator end()   noexcept {
      return begin() + m_exports.size();
    }

    [[nodiscard]] const_iterator begin() const noexcept {
      return m_exports.data();
    }
    [[nodiscard]] const_iterator end() const noexcept {
      return begin() + m_exports.size();
    }

    void clear() noexcept {
      m_exports.clear();
    }
  };


  class module;

  enum class undo_cookie_t : uintptr_t;

  inline constexpr static agt_config_flag_bits_t AGT_OPTION_IS_INTERNAL_CONSTRAINT = static_cast<agt_config_flag_bits_t>(0x200000); // This flag is only used internally, so is not exposed to clients
  inline constexpr static agt_config_flag_bits_t AGT_OPTION_IS_ENVVAR_OVERRIDE = static_cast<agt_config_flag_bits_t>(0x100000); // This flag is only used internally, so is not exposed to clients

  class init_manager {

    struct callback_header {
      callback_header* next;
      callback_header* prev;
    };

    struct cleanup_callback : callback_header {
      void (* cleanup)(void* userData);
      void (* dtor)(cleanup_callback* cb);
      void* userData;
    };


    callback_header m_callbackList = { &m_callbackList, &m_callbackList };
    agt_status_t m_status = AGT_SUCCESS;
    bool m_isComplete = false;


    static cleanup_callback* newBasicCallback(void(* callback)(void*), void* userData) noexcept {
      auto cbNode = new cleanup_callback;
      cbNode->cleanup = callback;
      cbNode->userData = userData;
      cbNode->dtor = [](cleanup_callback* cb) {
        delete cb;
      };
      return cbNode;
    }

    template <typename Fn>
    static cleanup_callback* newCallback(Fn&& fn) noexcept {
      using func_t = std::remove_cvref_t<Fn>;

      auto func = new func_t{std::forward<Fn>(fn)};
      auto callback = new cleanup_callback{
        .cleanup = [](void* ud) {
          std::invoke(*static_cast<func_t*>(ud));
        },
        .dtor = [](cleanup_callback* self) {
          delete static_cast<func_t*>(self->userData);
          delete self;
        },
        .userData = func
      };

      return callback;
    }

    static void insertCallbackAfter(callback_header* position, callback_header* callback) noexcept {
      callback->next = position->next;
      callback->prev = position;
      position->next->prev = callback;
      position->next = callback;
    }

    static void removeAndDestroyCallback(cleanup_callback* callback) noexcept {
      callback->prev->next = callback->next;
      callback->next->prev = callback->prev;
      callback->dtor(callback);
    }

    static undo_cookie_t     toCookie(cleanup_callback* cb) noexcept {
      return static_cast<undo_cookie_t>(reinterpret_cast<uintptr_t>(cb));
    }

    static cleanup_callback* fromCookie(undo_cookie_t cookie) noexcept {
      return reinterpret_cast<cleanup_callback*>(static_cast<uintptr_t>(cookie));
    }

    undo_cookie_t pushCallback(cleanup_callback* cb) noexcept {
      insertCallbackAfter(&m_callbackList, cb);
      return toCookie(cb);
    }

  public:

    ~init_manager() {
      const auto end = &m_callbackList;
      auto begin = m_callbackList.next;

      while (begin != end) {
        auto cb = static_cast<cleanup_callback*>(begin);
        auto next = begin->next;
        cb->cleanup(cb->userData);
        cb->dtor(cb);
        begin = next;
      }
    }


    void reportFatalError(std::string_view message) noexcept;
    void reportError(std::string_view message) noexcept;
    void reportWarning(std::string_view message) noexcept;

    void reportErrno(errno_t errorCode, std::string_view descriptionOfCause) noexcept;
    void raiseWin32Error(std::string_view descriptionOfCause) noexcept;
    void reportModuleMissingGetExportsFunc(const char* moduleName) noexcept;

    void reportCoreModuleMissingConfigLibraryFunc() noexcept;

    void reportModuleNameTooLong(const char* moduleName, size_t maxLength, errno_t errorCode) noexcept;

    void reportModuleNotFound(const char* moduleName, agt_init_necessity_t necessity, const void* customSearchPath = nullptr) noexcept;

    void reportExportTooLong(const char* name, size_t maxSize) noexcept;

    void reportInvalidFunctionIndex(const export_info& exportInfo) noexcept;

    void reportBadUtf8Encoding(const char* string, agt_init_necessity_t necessity) noexcept;

    void reportOptionBadType(const agt_config_option_t& option, std::initializer_list<agt_value_type_t> expectedTypes) noexcept;

    void reportInvalidPathWin32(const void* path, agt_init_necessity_t necessity, unsigned long errorCode) noexcept;

    void reportPathIsNotADirectory(const void* path, agt_init_necessity_t necessity) noexcept;

    void reportBadNegativeValue(const agt_config_option_t& option) noexcept;

    // given value is out of range for the actual option type.
    void reportOptionTypeOutOfRange(const agt_config_option_t& option, agt_value_type_t optionType) noexcept;

    // When a specified option is ignored; generally because it is overriden by a different option with a greater necessity.
    void reportOptionIgnored(const agt_config_option_t& option) noexcept;

    void reportBadEnvVarValue(std::string_view name, std::string_view value, agt_value_type_t targetType) noexcept;

    void reportBadVersionString(std::string_view value) noexcept;

    void reportEnvVarOverride(std::string_view envVar) noexcept;


    void reportFatalOptionConflict(const agt_config_option_t& firstOpt, const agt_config_option_t& secondOpt) noexcept;


    [[nodiscard]] bool isFatal() const noexcept;



    undo_cookie_t registerCleanup(void(* callback)(void*), void* userData) noexcept {
      return pushCallback(newBasicCallback(callback, userData));
    }

    undo_cookie_t registerCleanupAndDtor(void(* callback)(void*), void(* dtor)(void*), void* userData) noexcept {
      struct callback_info {
        void(* pCallback)(void*);
        void(* pDtor)(void*);
        void*  pUserData;
      };

      auto cbInfo = new callback_info{
        .pCallback = callback,
        .pDtor     = dtor,
        .pUserData = userData
      };

      auto cleanup = new cleanup_callback{
        .cleanup = [](void* ud) {
          auto info = static_cast<callback_info*>(ud);
          info->pCallback(info->pUserData);
        },
        .dtor = [](cleanup_callback* self) {
          auto info = static_cast<callback_info*>(self->userData);
          info->pDtor(info->pUserData);
          delete info;
          delete self;
        },
        .userData = cbInfo
      };

      return pushCallback(cleanup);
    }

    template <typename T> requires((std::integral<T> || std::is_enum_v<T>) && sizeof(T) <= sizeof(void*))
    undo_cookie_t registerCleanup(void(* callback)(T), T userData) noexcept {
      return pushCallback(newBasicCallback(reinterpret_cast<void(*)(void*)>(callback), reinterpret_cast<void*>(static_cast<uintptr_t>(userData))));
    }

    template <typename T> requires(!std::same_as<void, T>)
    undo_cookie_t registerCleanup(void(* callback)(T*), T* userData) noexcept {
      return pushCallback(newBasicCallback(reinterpret_cast<void(*)(void*)>(callback), (void*)userData));
    }

    template <typename Fn> requires std::invocable<Fn>
    undo_cookie_t registerCleanup(Fn&& fn) noexcept {
      return pushCallback(newCallback(std::forward<Fn>(fn)));
    }



    static void unregister(undo_cookie_t cookie) noexcept {
      removeAndDestroyCallback(fromCookie(cookie));
    }



    // Indicates that the initialization has completed; this means that cleanup functions will only execute if a fatal error was encountered.
    void complete() noexcept {
      m_isComplete = true;
    }


    // 1. Dumps status messages
    // 2. If incomplete (or fatal), call the cleanup callbacks in the order they were registered
    // 3. Determine appropriate status value to return
    // 4. Return status
    [[nodiscard]] agt_status_t finish() noexcept {
      // dump...

      const auto end = &m_callbackList;
      auto begin = m_callbackList.next;

      const bool shouldCleanup = !m_isComplete || isFatal();

      while (begin != end) {
        auto cb = static_cast<cleanup_callback*>(begin);
        auto next = begin->next;
        if (shouldCleanup)
          cb->cleanup(cb->userData);
        cb->dtor(cb);
        begin = next;
      }

      m_callbackList.next = &m_callbackList;
      m_callbackList.prev = &m_callbackList;

      return m_status;
    }
  };



  class module {
    const char*          m_name;
    void*                m_handle;
    init_manager*        m_errList;
    agt_init_necessity_t m_necessity;
    bool                 m_ownsHandle = true;
  public:

    module(dictionary_entry<agt_init_necessity_t>* name, init_manager& initManager, bool usingCustomSearchPaths) noexcept;

    module(const module&) = delete;
    module(module&& mod) noexcept
      : m_name(mod.m_name),
        m_handle(std::exchange(mod.m_handle, nullptr)),
        m_errList(mod.m_errList),
        m_necessity(mod.m_necessity)
    { }

    ~module();


    void loadExports(module_exports& exp, const attributes& attr) const noexcept;

    void configLibrary(library_configuration& out, agt_config_t config) const noexcept;

    void postInit(agt_ctx_t ctx) const noexcept;

    [[nodiscard]] bool  isValid() const noexcept {
      return m_handle != nullptr;
    }

    [[nodiscard]] void* releaseHandle() noexcept {
      m_ownsHandle = false;
      return m_handle;
    }

    [[nodiscard]] const char* name() const noexcept {
      return m_name;
    }

    [[nodiscard]] agt_init_necessity_t necessity() const noexcept {
      return m_necessity;
    }


    void freeHandle();
  };

  class user_module {
  public:

  };

  struct loader_shared {
    agt_config_t                      baseLoader = nullptr;
    map<void*, agt_config_t>          loaderMap;
    dictionary<agt_init_necessity_t>  requestedModules;
    agt_allocator_params_t            allocatorParams = {};
    vector<agt_user_module_info_t, 0> userModules;
    bool                              hasCustomAllocatorParams = false;
    agt_internal_log_handler_t        logHandler;
    void*                             logHandlerUserData;
  };



  // Required to be implemented by every module. Simply declares and defines the functions exported by the module.
  using get_exports_proc_t    = void(*)(module_exports& exports, const attributes& attrs);

  // Required to be implemented by the core module; other modules may optionally implement
  using config_library_proc_t = void(*)(library_configuration& out, agt_config_t config, init_manager& manager);

  // Completely optional; is called following the creation of the instance, contexts, and export tables. If the module needs to do some local initialization, it should do it by exporting this function.
  using post_init_proc_t = void(*)(agt_ctx_t ctx, init_manager& manager);



  export_table* get_local_export_table() noexcept;

}

extern "C" {

struct agt_config_st {
  size_t                       loaderSize;
  agt_config_t                 parent;
  int                          headerVersion;
  int                          loaderVersion;
  agt::export_table*           pExportTable;
  size_t                       exportTableSize; // shouldn't ever really grow but ya kno, just in case
  void*                        moduleHandle;
  agt::init::loader_shared*    shared;
  agt::vector<agt_config_option_t, 0> options;
  agt::vector<agt_config_t, 0> childLoaders;
  agt_internal_log_handler_t   logHandler;
  void*                        logHandlerUserData;
  agt_init_callback_t          initCallback;
  void*                        callbackUserData;
  agt_ctx_t*                   pCtxResult;

};

}



inline void agt::init::attributes::initialize(const library_configuration &libConfig, init_manager &initManager) noexcept {
  assert( libConfig.m_pfnCompileOptions != nullptr );

  libConfig.m_pfnCompileOptions(libConfig.m_options, *this, libConfig.m_env, initManager);
}



AGT_noinline inline void agt::init::init_manager::reportFatalError(std::string_view message) noexcept {
}
AGT_noinline inline void agt::init::init_manager::reportError(std::string_view message) noexcept {
}
AGT_noinline inline void agt::init::init_manager::reportWarning(std::string_view message) noexcept {
}
AGT_noinline inline void agt::init::init_manager::reportBadNegativeValue(const agt_config_option_t &option) noexcept {
}
AGT_noinline inline void agt::init::init_manager::reportOptionTypeOutOfRange(const agt_config_option_t &option, agt_value_type_t optionType) noexcept {
}
AGT_noinline inline void agt::init::init_manager::reportOptionIgnored(const agt_config_option_t &option) noexcept {
}
AGT_noinline inline void agt::init::init_manager::reportBadEnvVarValue(std::string_view name, std::string_view value, agt_value_type_t targetType) noexcept {
}
AGT_noinline inline void agt::init::init_manager::reportBadVersionString(std::string_view value) noexcept {
}
AGT_noinline inline void agt::init::init_manager::reportEnvVarOverride(std::string_view envVar) noexcept {
}
AGT_noinline inline void agt::init::init_manager::reportFatalOptionConflict(const agt_config_option_t &firstOpt, const agt_config_option_t &secondOpt) noexcept {
}
AGT_noinline inline void agt::init::init_manager::reportErrno(errno_t errorCode, std::string_view descriptionOfCause) noexcept {
  assert(false);
}

AGT_noinline inline void agt::init::init_manager::raiseWin32Error(std::string_view descriptionOfCause) noexcept {
  assert(false);
}
AGT_noinline inline void agt::init::init_manager::reportModuleMissingGetExportsFunc(const char* moduleName) noexcept {
  assert(false);
}

AGT_noinline inline void agt::init::init_manager::reportCoreModuleMissingConfigLibraryFunc() noexcept {
  assert(false);
}

AGT_noinline inline void agt::init::init_manager::reportModuleNameTooLong(const char* moduleName, size_t maxLength, errno_t errorCode) noexcept {
  assert(false);
}

AGT_noinline inline void agt::init::init_manager::reportModuleNotFound(const char* moduleName, agt_init_necessity_t necessity, const void* customSearchPath) noexcept {

  if (necessity == AGT_INIT_REQUIRED) {
    m_status = AGT_ERROR_MODULE_NOT_FOUND;
    //TODO: add error logging??
  }
  //assert(necessity != AGT_INIT_REQUIRED);
}

AGT_noinline inline void agt::init::init_manager::reportExportTooLong(const char* name, size_t maxSize) noexcept {
  assert(false);
}

AGT_noinline inline void agt::init::init_manager::reportInvalidFunctionIndex(const export_info& exportInfo) noexcept {
  assert(false);
}

AGT_noinline inline void agt::init::init_manager::reportBadUtf8Encoding(const char* string, agt_init_necessity_t necessity) noexcept {
  assert(necessity != AGT_INIT_REQUIRED);
}

AGT_noinline inline void agt::init::init_manager::reportOptionBadType(const agt_config_option_t& option, std::initializer_list<agt_value_type_t> expectedTypes) noexcept {
  assert(option.necessity != AGT_INIT_REQUIRED);
}

AGT_noinline inline void agt::init::init_manager::reportInvalidPathWin32(const void* path, agt_init_necessity_t necessity, unsigned long errorCode) noexcept {
  assert(necessity != AGT_INIT_REQUIRED);
}

AGT_noinline inline void agt::init::init_manager::reportPathIsNotADirectory(const void* path, agt_init_necessity_t necessity) noexcept {
  assert(necessity != AGT_INIT_REQUIRED);
}

[[nodiscard]] inline bool agt::init::init_manager::isFatal() const noexcept {
  return m_status != AGT_SUCCESS;
}







#endif//AGATE_INTERNAL_INIT_HPP
