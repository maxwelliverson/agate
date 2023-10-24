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

#include <span>
#include <variant>
#include <optional>

/*template <>
struct agt::map_key_info<agt_attr_id_t> {
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
  }


  class attribute_setter {
  public:

  };

  class attributes {
    size_t                  m_attrCount;
    const agt_value_type_t* m_attrType;
    const uintptr_t*        m_attrValue;

  public:

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
  };

  struct export_info {
    const char* procName;
    agt_proc_t  address;
    size_t      tableOffset;
  };


  class environment {
    string_pool               m_envVarValuePool;
    dictionary<pooled_string> m_envVarCache;
    bool                      m_hasPrefetched;


    using cookie_t = void*;

    enum sysenv_result {
      SYSENV_UNDEFINED_VARIABLE,
      SYSENV_SUCCESS,
      SYSENV_INSUFFICIENT_BUFFER
    };

    [[nodiscard]] static sysenv_result _sysenv_lookup(std::string_view key, char* buffer, size_t& length) noexcept;

    [[nodiscard]] pooled_string _lookup_value(std::string_view key) noexcept {
      constexpr static size_t StaticBufferSize = 256;
      char staticBuffer[StaticBufferSize];
      std::unique_ptr<char[]> dynBuffer;
      char*  data   = staticBuffer;
      size_t length = StaticBufferSize;


      do {
        switch (_sysenv_lookup(key, data, length)) {
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


    [[nodiscard]] cookie_t _read_all_environment_variables(any_vector<std::pair<std::string_view, std::string_view>>& keyValuePairs) noexcept;

    void _release_cookie(cookie_t cookie) noexcept;

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
    struct var_t {
      agt_value_type_t type;
      agt_value_t      value;
    };

    map<agt_attr_id_t, var_t>     m_attributes;
    const agt_allocator_params_t* m_allocParams = nullptr;
    agt_attr_id_t                 m_maxKnownId = { };
    vector<agt_u32_t, 0>          m_unknownAttributes;
    environment                   m_env;
  public:

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

    [[nodiscard]] bool attribute_is_set(agt_attr_id_t attrId) const noexcept {
      return m_attributes.count(attrId);
    }
  };


  class module_exports {
    vector<export_info> m_exports;
    void (*m_pfnDtor)(vector<export_info>& exports); // Passing a destructor pointer ensures that we don't run into any weird issues where the dll and the process are using difference allocation functions. This shouldn't be the case 99% of the time, but it can happen.

    inline constexpr static size_t DefaultInitialCapacity = 8;

  public:

    module_exports() noexcept : module_exports(DefaultInitialCapacity) { }
    explicit module_exports(size_t initialCapacity) noexcept {
      m_exports.reserve(initialCapacity);
    }

    module_exports(const module_exports&) = delete;
    module_exports(module_exports&&) noexcept = delete;

    ~module_exports() {
      assert( m_pfnDtor );
      clear();
    }

    void init() noexcept {
      clear();
      m_pfnDtor = [](vector<export_info>& exports){
        exports.destroy_all(); // By wrapping any deallocation in function pointer set by the module that would be doing the allocation; ensuring there's never an issue with mismatched allocators.
      };
    }

    template <typename Ret, typename ...Args>
    void add_export(const char* name, Ret(* procAddress)(Args...), size_t tableOffset) noexcept {
      auto& info       = m_exports.emplace_back();
      info.procName    = name;
      info.address     = (agt_proc_t)procAddress;
      info.tableOffset = tableOffset;
    }

    [[nodiscard]] std::span<const export_info> exports() const noexcept {
      return m_exports;
    }

    void clear() noexcept {
      if (m_pfnDtor)
        m_pfnDtor(m_exports);
    }
  };

  class module {
    const char*         m_name;
    void*               m_handle;
    const attributes*   m_attributes;
    module_exports      m_exports;
    bool                m_hasRetrievedExports;

    [[nodiscard]] agt_proc_t get_raw_proc(const char* procName) noexcept;
  public:

    module(const char* name, const attributes& attr, bool usingCustomSearchPath) noexcept;

    [[nodiscard]] std::span<const export_info> exports() noexcept;

    template <typename Fn>
    [[nodiscard]] Fn* get_proc(const char* procName) noexcept {
      return (Fn*)this->get_raw_proc(procName);
    }
  };

  class user_module {
  public:

  };

  class error_list {
  public:
  };

  struct shared_config {
    agt_config_t                           baseConfig = nullptr;
    agt::map<void*, agt_config_t>          configMap;
    agt::dictionary<agt_init_necessity_t>  requestedModules;
    agt_allocator_params_t                 allocatorParams = {};
    agt::vector<agt::vector<agt_config_option_t, 0>, 0> userOptions;
    agt::vector<agt_user_module_info_t, 0> userModules;
    bool                                   hasCustomAllocatorParams = false;
    agt_internal_log_handler_t             logHandler;
    void*                                  logHandlerUserData;
  };


  using get_exports_proc_t    = void(*)(module_exports& exports, const attributes& attrs);

  using config_library_proc_t = bool(*)(library_configuration& out, agt_config_t config, error_list& errList);


  export_table* get_local_export_table() noexcept;

}

extern "C" {

struct agt_config_st {
  size_t                       configSize;
  agt_config_t                 parent;
  int                          headerVersion;
  int                          loaderVersion;
  agt::export_table*           pExportTable;
  void*                        moduleHandle;
  agt::init::shared_config*    shared;
  agt::vector<agt_config_t, 0> childLoaders;
  agt_internal_log_handler_t   logHandler;
  void*                        logHandlerUserData;
};

}



#endif//AGATE_INTERNAL_INIT_HPP
