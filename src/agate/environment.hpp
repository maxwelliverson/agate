//
// Created by maxwe on 2022-12-17.
//

#ifndef AGATE_INTERNAL_ENVIRONMENT_HPP
#define AGATE_INTERNAL_ENVIRONMENT_HPP

#include "sys_string.hpp"
#include "parse.hpp"

namespace agt {

  class environment;

  class str_header {
  protected:
    agt_u32_t m_length;
  public:

    [[nodiscard]] size_t          size() const noexcept {
      return m_length;
    }

    [[nodiscard]] sys_char*       data()       noexcept {
      return reinterpret_cast<sys_char*>(reinterpret_cast<char*>(this) + sizeof(str_header));
    }
    [[nodiscard]] const sys_char* data() const noexcept {
      return reinterpret_cast<const sys_char*>(reinterpret_cast<const char*>(this) + sizeof(str_header));
    }
  };

  template <size_t N>
  class fixed_str : public str_header {
    sys_char m_buffer[N];
  public:
    fixed_str() = default;
    explicit fixed_str(size_t n) noexcept {
      set_length(n);
    }

    void set_length(size_t n) noexcept {
      this->m_length = static_cast<agt_u32_t>(n);
    }
  };

  class dyn_str : public str_header {
    sys_char  m_buffer[];
  public:

    [[nodiscard]] size_t capacity() const noexcept {
      return this->size() + 1;
    }

    static dyn_str* alloc(size_t length) noexcept {
      auto str = (dyn_str*)::malloc(sizeof(dyn_str) + (sizeof(sys_char) * (length + 1)));
      str->m_length = static_cast<agt_u32_t>(length);
      return str;
    }

    static dyn_str* alloc(sys_cstring str, size_t length) noexcept {
      auto dynStr = dyn_str::alloc(length);
      if (length)
        std::memcpy(dynStr->m_buffer, str, length * sizeof(sys_char));
      dynStr->m_buffer[length] = {};
      return dynStr;
    }

    static dyn_str* alloc(sys_cstring str) noexcept {
      return dyn_str::alloc(str, sys_strlen(str));
    }
  };

  class envvar {

    struct envvar_holder {
      const str_header* str;
      agt_u32_t         refCount;
      bool              strIsDynamic;
    };

    friend class environment;


    envvar_holder*    m_ptr = nullptr;


    static envvar_holder* alloc_holder() noexcept {
      auto holder                   = new envvar_holder;
      holder->str                   = nullptr;
      holder->refCount              = 1;
      holder->strIsDynamic          = false;
      return holder;
    }

    template <size_t N>
    static envvar make_new(envvar_holder* holder,  const fixed_str<N>& str) noexcept {
      AGT_assert(holder != nullptr);
      // environment_reset(holder);
      holder->str          = &str;
      holder->strIsDynamic = false;
      return envvar(holder);
    }

    static envvar make_new(envvar_holder*& holder, const dyn_str* str) noexcept {
      AGT_assert(holder != nullptr);
      environment_reset(holder);
      holder->str          = str;
      holder->strIsDynamic = true;
      return envvar(holder);
    }

    static void   destroy(envvar_holder* value) noexcept {
      AGT_assert(value);
      if (value->strIsDynamic)
        ::free((void*)value->str);
      delete value;
    }


    explicit envvar(std::nullptr_t) noexcept
        : m_ptr(nullptr) { }

    explicit envvar(envvar_holder* ptr) noexcept
        : m_ptr(ptr) { }


    static void try_convert_to_dynamic(envvar_holder* holder) noexcept {
      auto str = holder->str;
      if (!holder->strIsDynamic) {
        holder->str = dyn_str::alloc(str->data(), str->size());
        holder->strIsDynamic = true;
      }
    }

    static void environment_reset(envvar_holder*& holder) noexcept {
      auto h = holder;
      if (h->refCount > 1) {
        --h->refCount;
        try_convert_to_dynamic(h);
        holder = alloc_holder();
      }
      else {
        if (h->strIsDynamic)
          ::free((void*)h->str);
      }
      AGT_assert(holder->refCount == 1);
      holder->refCount = 2;
    }

    static void release(envvar_holder* holder) noexcept {
      if (holder && !--holder->refCount)
        destroy(holder);
    }

    void        release() noexcept {
      if (m_ptr && !--m_ptr->refCount)
        destroy(m_ptr);
    }

  public:

    envvar() = default;
    envvar(const envvar& other)     : m_ptr(other.m_ptr) {
      if (m_ptr)
        ++m_ptr->refCount;
    }
    envvar(envvar&& other) noexcept : m_ptr(std::exchange(other.m_ptr, nullptr)) { }

    envvar& operator=(const envvar& other) {
      if (&other == this || m_ptr == other.m_ptr)
        return *this;

      if (other.m_ptr)
        ++other.m_ptr->refCount;
      release();
      m_ptr = other.m_ptr;

      return *this;
    }
    envvar& operator=(envvar&& other) noexcept {
      release();
      m_ptr = std::exchange(other.m_ptr, nullptr);
      return *this;
    }

    ~envvar() {
      release();
    }


    [[nodiscard]] sys_string_view str() const noexcept {
      AGT_assert(m_ptr);
      return { m_ptr->str->data(), m_ptr->str->size() };
    }

    /*[[nodiscard]] std::optional<agt_u32_t> as_bool() const noexcept {}

    [[nodiscard]] std::optional<agt_u32_t> as_u32() const noexcept {}

    [[nodiscard]] std::optional<agt_u64_t> as_u64() const noexcept {
      return this->parse_integer<agt_u64_t>(str());
    }

    [[nodiscard]] std::optional<agt_u32_t> as_version() const noexcept {
      agt_u32_t major = 0, minor = 0, patch = 0;

      constexpr static auto split = [](sys_string_view s) -> std::pair<sys_string_view, sys_string_view> {
        auto pos = s.find('.');
        if (pos == std::string_view::npos)
          return { s, {} };
        return { s.substr(0, pos), s.substr(pos + 1) };
      };

      auto str = this->str();

      if (str.empty())
        return std::nullopt;
      if (str.front() == 'v')
        str.remove_prefix(1);

      sys_string_view tail;

      std::tie(str, tail) = split(str);

      if (auto majorVersion = parse_uint(str))
        major = majorVersion.value();
      else
        return std::nullopt;

      if (!tail.empty()) {
        std::tie(str, tail) = split(tail);

        if (auto minorVersion = parse_uint(str))
          minor = minorVersion.value();
        else
          return std::nullopt;

        if (!tail.empty()) {
          std::tie(str, tail) = split(tail);

          if (auto patchVersion = parse_uint(str))
            patch = patchVersion.value();
          else
            return std::nullopt;
        }
      }

      return AGT_MAKE_VERSION(major, minor, patch);
    }*/

    [[nodiscard]] unique_sys_str  unique_str() const noexcept {
      auto sv = str();
      return make_unique_string(sv.data(), sv.size());
    }


    explicit operator bool() const noexcept {
      return m_ptr != nullptr;
    }
  };

  template <typename To>
  inline std::optional<To> envvar_cast(const envvar& var) noexcept {
    if constexpr (std::same_as<To, bool>)
      return parse_bool(var.str());
    else if constexpr (std::same_as<To, version>)
      return parse_version(var.str());
    else if constexpr (std::integral<To>)
      return parse_integer<To>(var.str());
    else {
      std::unreachable();
    }
  }

  class environment {
    constexpr static DWORD InlineBufferLength = 1000;
    fixed_str<InlineBufferLength> m_envVarBuf;
    fixed_str<1>                  m_emptyString;
    envvar::envvar_holder*        m_holder;
    envvar::envvar_holder*        m_emptyStringHolder;
    /*unique_sys_str  m_dynamicBuffer = nullptr;
    sys_string_view m_value = {};*/
  public:
    environment()
        : m_envVarBuf(),
          m_emptyString(0),
          m_holder(envvar::alloc_holder()),
          m_emptyStringHolder(envvar::alloc_holder()) {
      *m_emptyString.data() = {};
      m_emptyStringHolder->str = &m_emptyString;
      m_emptyStringHolder->strIsDynamic = false;
    }

    [[nodiscard]] envvar operator[](sys_cstring envVarName) noexcept {
      if (DWORD length = GetEnvironmentVariableW(envVarName, nullptr, 0)) {
        if (length <= InlineBufferLength) {
          envvar::environment_reset(m_holder);
          DWORD realLength = GetEnvironmentVariableW(envVarName, m_envVarBuf.data(), length);
          AGT_assert(realLength == length - 1);
          m_envVarBuf.set_length(realLength);
          return envvar::make_new(m_holder, m_envVarBuf);
        }
        else {
          auto dynStr = dyn_str::alloc(length - 1);
          DWORD realLength = GetEnvironmentVariableW(envVarName, dynStr->data(), length);
          AGT_assert(realLength == length - 1);
          return envvar::make_new(m_holder, dynStr);
        }
      }
      else {
        if (GetLastError() == ERROR_ENVVAR_NOT_FOUND)
          return envvar(nullptr);
        ++m_emptyStringHolder->refCount;
        return envvar(m_emptyStringHolder);
      }
    }
  };

}

#endif//AGATE_INTERNAL_ENVIRONMENT_HPP
