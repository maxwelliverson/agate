//
// Created by maxwe on 2022-12-18.
//

#ifndef AGATE_INTERNAL_PARSE_HPP
#define AGATE_INTERNAL_PARSE_HPP

#include "sys_string.hpp"
#include "version.hpp"

namespace agt {

  // A series of flexible, fast, constexpr-safe(?) string parsing functions that don't return any error info,
  // but instead just return an optional value, where a missing failure represents a parsing error

  namespace detail {
    template <typename CharTraits>
    inline static constexpr auto get_char(char c) noexcept {
      return CharTraits::to_char_type(std::char_traits<char>::to_int_type(c));
    }

    enum class parse_integer_option_flags {
      parse_sign             = 0x1,
      parse_prefix           = 0x2,
      parse_digit_separators = 0x4
    };

    template <std::integral I>
    struct parse_int_constants {
      inline constexpr static agt_u64_t Bit             = 0x1;
      inline constexpr static bool      TypeIsSigned    = std::signed_integral<I>;
      inline constexpr static size_t    TypeBits        = std::numeric_limits<I>::digits;
      inline constexpr static size_t    TotalTypeBits   = TypeBits + TypeIsSigned;
      inline constexpr static size_t    DecimalMaxValue = []{
        const auto max      = (std::numeric_limits<I>::max)();
        const auto maxDiv10 = max / 10;
        const auto maxMod10 = ((max % 10) + 1) % 10;
        return maxDiv10 + (maxMod10 > 0 ? 1 : 0);
      }();
    };

    template <typename CharTraits>
    struct parse_char_constants {

      using char_type = typename CharTraits::char_type;

      inline constexpr static char_type MinusChar      = get_char<CharTraits>('-');
      inline constexpr static char_type PlusChar       = get_char<CharTraits>('+');
      inline constexpr static char_type ZeroChar       = get_char<CharTraits>('0');
      inline constexpr static char_type OneChar        = get_char<CharTraits>('1');
      inline constexpr static char_type NineChar       = get_char<CharTraits>('9');
      inline constexpr static char_type SmallXChar     = get_char<CharTraits>('x');
      inline constexpr static char_type BigXChar       = get_char<CharTraits>('X');
      inline constexpr static char_type SmallAChar     = get_char<CharTraits>('a');
      inline constexpr static char_type BigAChar       = get_char<CharTraits>('A');
      inline constexpr static char_type SmallBChar     = get_char<CharTraits>('b');
      inline constexpr static char_type BigBChar       = get_char<CharTraits>('B');
    };

    template <std::integral I, typename CharTraits, size_t ChunkBits_>
    struct parse_constants : parse_int_constants<I>, parse_char_constants<CharTraits> {

    private:
      using C = parse_int_constants<I>;
    public:

      inline constexpr static size_t    ChunkBits      = ChunkBits_;
      inline constexpr static size_t    FullChunkCount = C::TypeBits / ChunkBits;
      inline constexpr static size_t    FinalChunkBits = C::TypeBits % ChunkBits;
      inline constexpr static agt_u64_t ChunkMask      = static_cast<agt_u64_t>(-1) << ChunkBits;
      inline constexpr static agt_u64_t FinalChunkMask = static_cast<agt_u64_t>(-1) << FinalChunkBits;
      inline constexpr static agt_u64_t MaxDigitCount  = FullChunkCount + (FinalChunkBits ? 1 : 0);
      inline constexpr static agt_u64_t ValueMask      = []{
        if constexpr (C::TypeBits == 64)
          return 0;
        else
          return static_cast<agt_u64_t>(-1) << C::TypeBits;
      }();
    };

    template <typename Char, typename CharTraits, size_t N>
    inline static constexpr bool equals_string(std::basic_string_view<Char, CharTraits> str, const char(& other)[N]) noexcept {
      if constexpr (std::same_as<Char, char>) {
        return str == other;
      }
      else {
        if (str.size() != N-1)
          return false;
        for (size_t i = 0; i < N - 1; ++i)
          if (str[i] != get_char<CharTraits>(other[i]))
            return false;
        return true;
      }
    }
  }

  template <typename Char, typename CharTraits>
  static constexpr std::optional<bool> parse_bool(std::basic_string_view<Char, CharTraits> str) noexcept {

    using detail::get_char,
        detail::equals_string;

    if (str.empty())
      return std::nullopt;
    switch(str.front()) {
      case get_char<CharTraits>('0'):
        if (str.size() > 1)
          return std::nullopt;
        else
          return false;
      case get_char<CharTraits>('1'):
        if (str.size() > 1)
          return std::nullopt;
        else
          return true;
      case get_char<CharTraits>('F'):
        if (equals_string(str, "FALSE"))
          return false;
        else
          return std::nullopt;
      case get_char<CharTraits>('N'):
        if (equals_string(str, "NO"))
          return false;
        else
          return std::nullopt;
      case get_char<CharTraits>('T'):
        if (equals_string(str, "TRUE"))
          return true;
        else
          return std::nullopt;
      case get_char<CharTraits>('Y'):
        if (equals_string(str, "YES"))
          return true;
        else
          return std::nullopt;
      case get_char<CharTraits>('f'):
        if (equals_string(str, "false"))
          return false;
        else
          return std::nullopt;
      case get_char<CharTraits>('n'):
        if (equals_string(str, "no"))
          return false;
        else
          return std::nullopt;
      case get_char<CharTraits>('t'):
        if (equals_string(str, "true"))
          return true;
        else
          return std::nullopt;
      case get_char<CharTraits>('y'):
        if (equals_string(str, "yes"))
          return true;
      default:
        return std::nullopt;
    }
  }

  template <typename Char, typename CharTraits = std::char_traits<Char>>
  static constexpr std::optional<bool> parse_bool(const Char* str) noexcept {
    return parse_bool(std::basic_string_view<Char, CharTraits>(str));
  }

  template <std::integral I,
            bool ParseSign = std::signed_integral<I>,
            bool ParsePrefix = true,
            typename Char,
            typename CharTraits>
  static constexpr std::optional<I> parse_hex_integer(std::basic_string_view<Char, CharTraits> sv) noexcept {

    using namespace detail;

    using C = parse_constants<I, CharTraits, 4>;

    agt_u64_t value = 0;
    agt_u64_t oNibble = 0;
    bool isSigned = false;

    if (sv.empty())
      return std::nullopt;

    if constexpr (ParseSign) {
      if (sv.front() == C::MinusChar || sv.front() == C::PlusChar) {
        isSigned = sv.front() == C::MinusChar;
        sv.remove_prefix(1);
        if (sv.empty())
          return std::nullopt;
      }
    }

    if constexpr (ParsePrefix) {
      if (sv.front() == C::ZeroChar) {
        if (sv.size() == 1)
          return static_cast<I>(0);
        else {
          auto c = sv[1];
          if (c == C::SmallXChar || c == C::BigXChar) {
            sv.remove_prefix(2);
            if (sv.empty())
              return std::nullopt;
            // Leading zeroes are okay when having specifically called parse_hex_integer, given that we are not dispatching based on prefix
          }
        }
      }
    }

    if (sv.size() > C::MaxDigitCount)
      return std::nullopt;
    else if constexpr (C::FinalChunkBits > 0) {

      // Actually an assertion that at this point in the code path,
      // FinalChunkBits is known to be 3. The zero clause has to be
      // added in because of the weird defect whereby static assertions
      // are evaluated in unevaluated constexpr branches, which I
      // believe has been fixed in C++23.
      static_assert(C::FinalChunkBits == 0 || C::FinalChunkBits == 3);

      if (sv.size() == C::MaxDigitCount) {
        // auto c = sv.front();
        value = sv.front() - C::ZeroChar;
        if (value & C::FinalChunkMask) // Out of range OR invalid character, depending...
          return std::nullopt;
        sv.remove_prefix(1);
      }
    }



    for (size_t i = 0; i < C::FullChunkCount; ++i) {
      auto c = sv.front();
      switch (c) {
        case get_char<CharTraits>('0'):
        case get_char<CharTraits>('1'):
        case get_char<CharTraits>('2'):
        case get_char<CharTraits>('3'):
        case get_char<CharTraits>('4'):
        case get_char<CharTraits>('5'):
        case get_char<CharTraits>('6'):
        case get_char<CharTraits>('7'):
        case get_char<CharTraits>('8'):
        case get_char<CharTraits>('9'):
          oNibble = c - C::ZeroChar;
          break;
        case get_char<CharTraits>('a'):
        case get_char<CharTraits>('b'):
        case get_char<CharTraits>('c'):
        case get_char<CharTraits>('d'):
        case get_char<CharTraits>('e'):
        case get_char<CharTraits>('f'):
          oNibble = 10 + (c - C::SmallAChar);
          break;
        case get_char<CharTraits>('A'):
        case get_char<CharTraits>('B'):
        case get_char<CharTraits>('C'):
        case get_char<CharTraits>('D'):
        case get_char<CharTraits>('E'):
        case get_char<CharTraits>('F'):
          oNibble = 10 + (c - C::BigAChar);
          break;
        default:
          return std::nullopt;
      }
      value = (value << C::ChunkBits) | oNibble;
      sv.remove_prefix(1);
      if (sv.empty())
        break;
    }
    if (!sv.empty())       // Range error OR invalid trailing characters
      return std::nullopt;
    if (value & C::ValueMask) // Range error, though I actually don't think this code is reachable??
      return std::nullopt;

    I realValue = static_cast<I>(value);

    if constexpr (ParseSign) {
      if (isSigned)
        realValue = -realValue;
    }

    return realValue;
  }

  template <std::integral I,
            bool ParseSign = std::signed_integral<I>,
            bool ParsePrefix = true,
            typename Char,
            typename CharTraits = std::char_traits<Char>>
  static constexpr std::optional<I> parse_hex_integer(const Char* str) noexcept {
    return parse_hex_integer<I, ParseSign, ParsePrefix>(std::basic_string_view<Char, CharTraits>(str));
  }

  template <std::integral I,
            bool ParseSign = std::signed_integral<I>,
            typename Char,
            typename CharTraits>
  static constexpr std::optional<I> parse_decimal_integer(std::basic_string_view<Char, CharTraits> sv) noexcept {


    if !consteval {

      I val;

#if AGT_system_windows
      constexpr static size_t MaxDigits = 23;
      constexpr static agt_u16_t AsciiMaxValue = 127;
      char digitBuffer[MaxDigits + 1];

      if (sv.size() > MaxDigits)
        return std::nullopt;
      for (size_t i = 0; i < sv.size(); ++i) {
        const auto wc = sv[i];
        auto charVal = static_cast<agt_u16_t>(wc);
        if (charVal > AsciiMaxValue) [[unlikely]]
          return std::nullopt;
        digitBuffer[i] = static_cast<char>(wc);
      }
      const char* first = digitBuffer;
#else
      auto first = sv.data();
#endif
      auto last  = first + sv.size();
      auto [end, err] = std::from_chars(first, last, val);
      if (end != last || err == std::errc::result_out_of_range)
        return std::nullopt;
      return val;
    }

    if consteval {
      using namespace detail;

      using C = parse_char_constants<CharTraits>;
      using V = parse_int_constants<I>;

      agt_u64_t value = 0;
      agt_u64_t oNibble = 0;
      bool      isSigned = false;

      if (sv.empty())
        return std::nullopt;

      if constexpr (ParseSign) {
        if (sv.front() == C::MinusChar || sv.front() == C::PlusChar) {
          isSigned = sv.front() == C::MinusChar;
          sv.remove_prefix(1);
          if (sv.empty())
            return std::nullopt;
        }
      }

      for (auto c : sv) {
        if (value > V::DecimalMaxValue)
          return std::nullopt;
        if (c < C::ZeroChar || C::NineChar < c)
          return std::nullopt;
        oNibble = c - C::ZeroChar;
        value *= 10;
        value += oNibble;
      }

      I realValue = static_cast<I>(value);

      if constexpr (ParseSign) {
        if (isSigned)
          realValue = -realValue;
      }

      return realValue;
    }
  }

  template <std::integral I,
           bool ParseSign = std::signed_integral<I>,
           typename Char,
           typename CharTraits = std::char_traits<Char>>
  static constexpr std::optional<I> parse_decimal_integer(const Char* str) noexcept {
    return parse_decimal_integer<I, ParseSign>(std::basic_string_view<Char, CharTraits>(str));
  }

  template <std::integral I,
            bool ParseSign = std::signed_integral<I>,
            typename Char,
            typename CharTraits>
  static constexpr std::optional<I> parse_octal_integer(std::basic_string_view<Char, CharTraits> sv) noexcept {

    using namespace detail;

    using C = parse_constants<I, CharTraits, 3>;

    agt_u64_t value = 0;
    agt_u64_t oNibble = 0;
    bool isSigned = false;

    if (sv.empty())
      return std::nullopt;

    if constexpr (ParseSign) {
      if (sv.front() == C::MinusChar || sv.front() == C::PlusChar) {
        isSigned = sv.front() == C::MinusChar;
        sv.remove_prefix(1);
        if (sv.empty())
          return std::nullopt;
      }
    }

    // No need for a specific parse prefix clause; the octal prefix is simply a leading zero, which will otherwise come out in the wash here

    if (sv.size() > C::MaxDigitCount)
      return std::nullopt;
    else if constexpr (C::FinalChunkBits > 0) {
      if (sv.size() == C::MaxDigitCount) {
        value = sv.front() - C::ZeroChar;
        if (value & C::FinalChunkMask) // Out of range OR invalid character, depending...
          return std::nullopt;
        sv.remove_prefix(1);
      }
    }



    for (size_t i = 0; i < C::FullChunkCount; ++i) {
      oNibble = sv.front() - C::ZeroChar;
      if (oNibble & C::ChunkMask)
        return std::nullopt;
      value = (value << C::ChunkBits) | oNibble;
      sv.remove_prefix(1);
      if (sv.empty())
        break;
    }
    if (!sv.empty())       // Range error OR invalid trailing characters
      return std::nullopt;
    if (value & C::ValueMask) // Range error, though I actually don't think this code is reachable??
      return std::nullopt;

    I realValue = static_cast<I>(value);

    if constexpr (ParseSign) {
      if (isSigned)
        realValue = -realValue;
    }

    return realValue;
  }

  template <std::integral I,
            bool ParseSign = std::signed_integral<I>,
            typename Char,
            typename CharTraits = std::char_traits<Char>>
  static constexpr std::optional<I> parse_octal_integer(const Char* str) noexcept {
    return parse_octal_integer<I, ParseSign>(std::basic_string_view<Char, CharTraits>(str));
  }

  template <std::integral I,
            bool ParsePrefix = true,
            typename Char,
            typename CharTraits>
  static constexpr std::optional<I> parse_binary_integer(std::basic_string_view<Char, CharTraits> sv) noexcept {

    using namespace detail;

    using C = parse_char_constants<CharTraits>;
    using V = parse_int_constants<I>;

    if constexpr (ParsePrefix) {
      if (sv.front() == C::ZeroChar) {
        if (sv.size() == 1)
          return static_cast<I>(0);
        else {
          auto c = sv[1];
          if (c == C::SmallBChar || c == C::BigBChar) {
            sv.remove_prefix(2);
            if (sv.empty())
              return std::nullopt;
            // Leading zeroes are okay when having specifically called parse_hex_integer, given that we are not dispatching based on prefix
          }
        }
      }
    }

    agt_u64_t value = 0;

    if (sv.empty() || sv.size() > V::TotalTypeBits)
      return std::nullopt;

    for (auto c : sv) {
      if (c == C::ZeroChar)
        value <<= 1;
      else if (c == C::OneChar)
        value = (value << 1) | V::Bit;
      else
        return std::nullopt;
    }

    return static_cast<I>(value);
  }

  template <std::integral I,
           bool ParsePrefix = true,
           typename Char,
           typename CharTraits = std::char_traits<Char>>
  static constexpr std::optional<I> parse_binary_integer(const Char* str) noexcept {
    return parse_binary_integer<I, ParsePrefix>(std::basic_string_view<Char, CharTraits>(str));
  }

  template <std::integral I,
            bool ParseSign = std::signed_integral<I>,
            typename Char,
            typename CharTraits>
  static constexpr std::optional<I> parse_integer(std::basic_string_view<Char, CharTraits> sv) noexcept {

    using namespace detail;

    using C = parse_char_constants<CharTraits>;

    bool isSigned = false;



    if (sv.empty())
      return std::nullopt;

    if constexpr (ParseSign) {
      if (sv.front() == C::MinusChar) {
        isSigned = true;
        sv.remove_prefix(1);
      }
      else if (sv.front() == C::PlusChar) {
        sv.remove_prefix(1);
      }
    }

    if (sv.front() == C::ZeroChar) {
      if (sv.size() == 1)
        return static_cast<I>(0);
      auto secondChar = sv[1];
      switch (secondChar) {
        case get_char<CharTraits>('x'):
        case get_char<CharTraits>('X'): {
          sv.remove_prefix(2);
          if constexpr (!ParseSign)
            return parse_hex_integer<I, false, false>(sv);
          else {
            if (const auto tmpValue = parse_hex_integer<I, false, false>(sv)) {
              const auto v = tmpValue.value();
              return isSigned ? -v : v;
            }
            return std::nullopt;
          }
        }
        case get_char<CharTraits>('b'):
        case get_char<CharTraits>('B'): {
          sv.remove_prefix(2);
          if constexpr (!ParseSign)
            return parse_binary_integer<I>(sv);
          else
            return isSigned ? std::nullopt : parse_binary_integer<I, false>(sv);
        }
        case get_char<CharTraits>('0'):
        case get_char<CharTraits>('1'):
        case get_char<CharTraits>('2'):
        case get_char<CharTraits>('3'):
        case get_char<CharTraits>('4'):
        case get_char<CharTraits>('5'):
        case get_char<CharTraits>('6'):
        case get_char<CharTraits>('7'):
          sv.remove_prefix(1);
          if constexpr (!ParseSign)
            return parse_octal_integer<I, false>(sv);
          else {
            if (const auto tmpValue = parse_octal_integer<I, false>(sv)) {
              const auto v = tmpValue.value();
              return isSigned ? -v : v;
            }
            return std::nullopt;
          }
        default:
          return std::nullopt;
      }
    }

    if constexpr (!ParseSign)
      return parse_decimal_integer<I, false>(sv);
    else {
      if (const auto tmpValue = parse_decimal_integer<I, false>(sv)) {
        const auto v = tmpValue.value();
        return isSigned ? -v : v;
      }
      return std::nullopt;
    }
  }

  template <std::integral I,
           bool ParseSign = std::signed_integral<I>,
           typename Char,
           typename CharTraits = std::char_traits<Char>>
  static constexpr std::optional<I> parse_integer(const Char* str) noexcept {
    return parse_integer<I, ParseSign>(std::basic_string_view<Char, CharTraits>(str));
  }

  template <typename Char, typename CharTraits>
  static constexpr std::optional<version> parse_version(std::basic_string_view<Char, CharTraits> sv) noexcept {
    agt_u32_t major = 0, minor = 0, patch = 0;

    using string_view = std::basic_string_view<Char, CharTraits>;

    using detail::get_char;

    if (sv.empty())
      return std::nullopt;
    if (sv.front() == get_char<CharTraits>('v'))
      sv.remove_prefix(1);

    string_view tail;

    std::tie(sv, tail) = split(sv, get_char<CharTraits>('.'));

    if (auto majorVersion = parse_decimal_integer<agt_u16_t>(sv)) {
      major = majorVersion.value();
    }
    else
      return std::nullopt;

    if (!tail.empty()) {
      std::tie(sv, tail) = split(tail, get_char<CharTraits>('.'));

      if (auto minorVersion = parse_decimal_integer<agt_u16_t>(sv)) {
        minor = minorVersion.value();
      }
      else
        return std::nullopt;

      if (!tail.empty()) {
        std::tie(sv, tail) = split(tail, get_char<CharTraits>('.'));

        if (auto patchVersion = parse_decimal_integer<agt_u16_t>(sv))
          patch = patchVersion.value();
        else
          return std::nullopt;
      }
    }

    return version(major, minor, patch);
  }

  template <typename Char, typename CharTraits = std::char_traits<Char>>
  static constexpr std::optional<version> parse_version(const Char* str) noexcept {
    return parse_version(std::basic_string_view<Char, CharTraits>(str));
  }

  static_assert(    parse_integer<agt_i32_t>("-23").value() == -23);
  static_assert(not parse_integer<agt_i16_t>("0x44832").has_value());
  static_assert(parse_integer<agt_u64_t>(L"0b1101101").value() == 109);

  static_assert(parse_version(L"v10.1").value() == version(10, 1));

  static_assert(parse_bool("true").value() == true);
  static_assert(parse_bool("NO").value() == false);
  static_assert(parse_bool(L"NO").value() == false);
}

#endif//AGATE_INTERNAL_PARSE_HPP
