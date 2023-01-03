//
// Created by maxwe on 2022-12-17.
//

#ifndef AGATE_INTERNAL_SYS_STRING_HPP
#define AGATE_INTERNAL_SYS_STRING_HPP

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


#include <cstring>
#include <string_view>
#include <optional>
#include <charconv>
#include <algorithm>
#include <memory>
#include <bit>


#if AGT_system_windows
#define AGT_sys_string(str) L##str
#else
#define AGT_sys_string(str) str
#endif

namespace agt {

#if AGT_system_windows
  using sys_char = wchar_t;
#else
  using sys_char = char;
#endif

  using sys_string_view = std::basic_string_view<sys_char>;
  using sys_cstring     = const sys_char*;


  template <typename Char>
  struct unique_str_deleter {
    void operator()(Char* str) const noexcept {
      ::free(str);
    }
  };

  using unique_cstr     = std::unique_ptr<char[], unique_str_deleter<char>>;
  using unique_wstr     = std::unique_ptr<wchar_t[], unique_str_deleter<wchar_t>>;

  using unique_sys_str  = std::unique_ptr<sys_char[], unique_str_deleter<sys_char>>;



  template <typename Char, typename CharTraits>
  [[nodiscard]] constexpr auto split(std::basic_string_view<Char, CharTraits> sv,
                                     std::type_identity_t<Char> c) -> decltype(std::make_pair(sv, sv)) {
    using string_view = std::basic_string_view<Char, CharTraits>;

    auto pos = sv.find(c);
    if (pos == string_view::npos)
      return { sv, {} };
    return { sv.substr(0, pos), sv.substr(pos + 1) };
  }


  template <typename Char, typename CharTraits>
  [[nodiscard]] constexpr auto split(std::basic_string_view<Char, CharTraits> sv,
                                     std::basic_string_view<std::type_identity_t<Char>, std::type_identity_t<CharTraits>> pattern) -> decltype(std::make_pair(sv, sv)) {
    using string_view = std::basic_string_view<Char, CharTraits>;

    auto pos = sv.find(pattern);
    if (pos == string_view::npos)
      return { sv, {} };
    return { sv.substr(0, pos), sv.substr(pos + pattern.size()) };
  }

  template <typename Char, typename CharTraits>
  [[nodiscard]] constexpr auto split_on_first_of(std::basic_string_view<Char, CharTraits> sv,
                                                 std::basic_string_view<std::type_identity_t<Char>, std::type_identity_t<CharTraits>> charSet) -> decltype(std::make_pair(sv, sv)) {
    using string_view = std::basic_string_view<Char, CharTraits>;

    auto pos = sv.find_first_of(charSet);
    if (pos == string_view::npos)
      return { sv, {} };
    return { sv.substr(0, pos), sv.substr(pos + 1) };
  }






  [[nodiscard]] size_t         sys_strlen(sys_cstring str) noexcept {
#if AGT_system_windows
    return ::wcslen(str);
#else
    return ::strlen(str);
#endif
  }

  [[nodiscard]] size_t         sys_strnlen(sys_cstring str, size_t maxLength) noexcept {
#if AGT_system_windows
    return ::wcsnlen_s(str, maxLength);
#else
    return ::strnlen(str, maxLength);
#endif
  }





  [[nodiscard]] sys_cstring    copy_sys_string(sys_char* buf, size_t bufSize, sys_char** dynPtr, const char* cstr, size_t length = static_cast<size_t>(-1)) noexcept {
#if AGT_system_windows
    if (length == static_cast<size_t>(-1))
      length = ::strlen(cstr);

    wchar_t* ptr;

    if (length < bufSize) {
      ptr = buf;
      *dynPtr = nullptr;
    }
    else {
      ptr = (wchar_t*)::malloc((length + 1) * sizeof(wchar_t));
      *dynPtr = ptr;
    }

    if (length > 0) [[likely]] {
      for (size_t i = 0; i < length; ++i)
        ptr[i] = static_cast<wchar_t>(cstr[i]);
    }
    ptr[length] = L'\0';
    return ptr;
#else
#endif
  }

  [[nodiscard]] sys_cstring    copy_sys_string(sys_char* buf, size_t bufSize, sys_char** dynPtr, const wchar_t* wstr, size_t length = static_cast<size_t>(-1)) noexcept {
#if AGT_system_windows
    if (length == static_cast<size_t>(-1))
      length = ::wcslen(wstr);

    wchar_t* ptr;

    if (length < bufSize) {
      ptr = buf;
      *dynPtr = nullptr;
    }
    else {
      ptr = (wchar_t*)::malloc((length + 1) * sizeof(wchar_t));
      *dynPtr = ptr;
    }

    if (length > 0) [[likely]]
      ::wmemcpy(ptr, wstr, length);
    ptr[length] = L'\0';
    return ptr;

#else
#endif
  }

  template <size_t N>
  [[nodiscard]] sys_cstring    copy_sys_string(sys_char(& buf)[N], sys_char** dynPtr, const char* cstr, size_t length = static_cast<size_t>(-1)) noexcept {
    return copy_sys_string(buf, N, dynPtr, cstr, length);
  }

  template <size_t N>
  [[nodiscard]] sys_cstring    copy_sys_string(sys_char(& buf)[N], sys_char** dynPtr, const wchar_t* wstr, size_t length = static_cast<size_t>(-1)) noexcept {
    return copy_sys_string(buf, N, dynPtr, wstr, length);
  }


  [[nodiscard]] unique_sys_str make_unique_string(const char* cstr, size_t length = static_cast<size_t>(-1)) noexcept {
#if AGT_system_windows
    if (length == static_cast<size_t>(-1))
      length = ::strlen(cstr);
    auto buf      = (wchar_t*)::malloc((length + 1) * sizeof(wchar_t));
    for (size_t i = 0; i < length; ++i)
      buf[i] = static_cast<wchar_t>(cstr[i]);
    buf[length] = L'\0';
#else
    return unique_sys_str(::strdup(cstr));
#endif
  }

  [[nodiscard]] unique_sys_str make_unique_string(const wchar_t* wstr, size_t length = static_cast<size_t>(-1)) noexcept {
#if AGT_system_windows
    if (length == static_cast<size_t>(-1))
      return unique_sys_str(_wcsdup(wstr));
    else {
      auto ptr = (wchar_t*)::malloc((length + 1) * sizeof(wchar_t));
      ::wmemcpy(ptr, wstr, length);
      ptr[length] = L'\0';
      return unique_sys_str(ptr);
    }
#else
#endif
  }

  [[nodiscard]] unique_sys_str make_unique_string(unique_cstr&& str) noexcept {
#if AGT_system_windows
    // TODO: Maybe write utf8 to wide string conversion routine??
    /*size_t length = ::strlen(str.get());
    auto buf      = (wchar_t*)::malloc((length + 1) * sizeof(wchar_t));
    const char* ptr = str.get();
    for (size_t i = 0; i < length; ++i)
      buf[i] = static_cast<wchar_t>(ptr[i]);
    buf[length] = L'\0';
    str.release();
    return unique_sys_str(buf);*/
    auto result = make_unique_string(str.get());
    str.release();
    return std::move(result);
#else
    return std::move(str);
#endif
  }

  [[nodiscard]] unique_sys_str make_unique_string(unique_wstr&& str) noexcept {
#if AGT_system_windows
    return std::move(str);
#else
    auto result = make_unique_string(str.get());
    str.release();
    return std::move(result);
#endif
  }

  template <typename Char, typename CharTraits>
  [[nodiscard]] unique_sys_str make_unique_string(std::basic_string_view<Char, CharTraits> str) noexcept {
    return make_unique_string(str.data(), str.size());
  }


  template <typename Char, std::invocable<Char*, size_t&> Fn, size_t N> requires std::is_invocable_r_v<bool, Fn, Char*, size_t&>
  [[nodiscard]] const Char* get_string(Char(& buffer)[N], std::unique_ptr<Char[], unique_str_deleter<Char>>& dyn, Fn&& fn) noexcept {
    size_t writtenBytes = N;
    if (fn(buffer, writtenBytes))
      return buffer;
    if (writtenBytes <= N)
      return nullptr;

    auto dynBuf = (Char*)::malloc(writtenBytes * sizeof(Char));
    dyn.reset(dynBuf);
    return fn(dynBuf, writtenBytes) ? dynBuf : nullptr;
  }
}

#endif//AGATE_INTERNAL_SYS_STRING_HPP
