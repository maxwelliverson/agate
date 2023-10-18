//
// Created by maxwe on 2021-12-07.
//

#ifndef JEMSYS_AGATE2_INTERNAL_FLAGS_HPP
#define JEMSYS_AGATE2_INTERNAL_FLAGS_HPP

#define AGT_BITFLAG_ENUM(EnumType, UnderlyingType) \
  enum class EnumType : UnderlyingType;            \
  template <> struct ::agt::impl::is_bitflag_enum_st<EnumType>{}; \
  AGT_forceinline constexpr EnumType operator~(EnumType a) noexcept { \
    return static_cast<EnumType>(~static_cast<UnderlyingType>(a)); \
  }                                                  \
  AGT_forceinline constexpr EnumType operator&(EnumType a, EnumType b) noexcept { \
    return static_cast<EnumType>(static_cast<UnderlyingType>(a) & static_cast<UnderlyingType>(b)); \
  }                                                  \
  AGT_forceinline constexpr EnumType operator|(EnumType a, EnumType b) noexcept { \
    return static_cast<EnumType>(static_cast<UnderlyingType>(a) | static_cast<UnderlyingType>(b)); \
  }                                                  \
  AGT_forceinline constexpr EnumType operator^(EnumType a, EnumType b) noexcept { \
    return static_cast<EnumType>(static_cast<UnderlyingType>(a) ^ static_cast<UnderlyingType>(b)); \
  }                                                  \
  AGT_forceinline constexpr EnumType& operator&=(EnumType& a, EnumType b) noexcept { \
    return (a = a & b); \
  }                                                  \
  AGT_forceinline constexpr EnumType& operator|=(EnumType& a, EnumType b) noexcept { \
    return (a = a | b); \
  }                                                  \
  AGT_forceinline constexpr EnumType& operator^=(EnumType& a, EnumType b) noexcept { \
    return (a = a ^ b); \
  }                                                 \
  enum class EnumType : UnderlyingType


#include <concepts>


namespace agt {
  namespace impl {
    template <typename T>
    struct is_bitflag_enum_st;
    template <typename T>
    concept bitflag_enum = requires {
      sizeof(is_bitflag_enum_st<T>);
    };
    template <typename T>
    concept cv_bitflag_enum = bitflag_enum<std::remove_cv_t<T>>;
    template <typename T>
    concept cvref_bitflag_enum = bitflag_enum<std::remove_cvref_t<T>>;
    struct empty_flags {
      template <bitflag_enum T>
      constexpr operator T() const noexcept {
        return static_cast<T>(0);
      }
    };
  }
  
  inline constexpr static impl::empty_flags flags_not_set = {};

  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static bool test(const E value) noexcept {
    return static_cast<bool>(value);
  }

  /* TODO: Figure out why the following overloads weren't working??
   *       std::type_identity should work in theory, allowing for conversions to the bitflag type and such...
   *
   * template <impl::bitflag_enum E>
   * AGT_forceinline constexpr static bool test(const E value, const std::type_identity_t<E> testFlags) noexcept
   */

  // Annoying as shit workaround with two separate overloads while std::type_identity isn't working >:(
  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static bool test(const E value, const E testFlags) noexcept {
    return static_cast<bool>(value & testFlags);
  }
  template <impl::bitflag_enum E, std::convertible_to<E> F>
  AGT_forceinline constexpr static bool test(const E value, F&& testFlags) noexcept {
    return test(value, static_cast<E>(std::forward<F>(testFlags)));
  }

  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static bool test_any(const E value, const E testFlags) noexcept {
    return test(value, testFlags);
  }
  template <impl::bitflag_enum E, std::convertible_to<E> F>
  AGT_forceinline constexpr static bool test_any(const E value, F&& testFlags) noexcept {
    return test(value, std::forward<F>(testFlags));
  }

  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static bool test_all(const E value, const E testFlags) noexcept {
    return (value & testFlags) == testFlags;
  }
  template <impl::bitflag_enum E, std::convertible_to<E> F>
  AGT_forceinline constexpr static bool test_all(const E value, F&& testFlags) noexcept {
    return test_all(value, static_cast<E>(std::forward<F>(testFlags)));
  }

  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static void set_flags(E& value, const E setFlags) noexcept {
    value = value | setFlags;
  }
  template <impl::bitflag_enum E, std::convertible_to<E> F>
  AGT_forceinline constexpr static void set_flags(E& value, F&& setFlags) noexcept {
    set(value, static_cast<E>(std::forward<F>(setFlags)));
  }
  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static void reset(E& value, const E resetFlags) noexcept {
    value = value & ~resetFlags;
  }
  template <impl::bitflag_enum E, std::convertible_to<E> F>
  AGT_forceinline constexpr static void reset(E& value, F&& resetFlags) noexcept {
    reset(value, static_cast<E>(std::forward<F>(resetFlags)));
  }



  /*
   * std::type_identity_t overloads
   *
  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static bool test(const E value, const std::type_identity_t<E> testFlags) noexcept {
    return test(value & testFlags);
  }
  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static bool test_any(const E value, const std::type_identity_t<E> testFlags) noexcept {
    return test(value, testFlags);
  }
  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static bool test_all(const E value, const std::type_identity_t<E> testFlags) noexcept {
    return (value & testFlags) == testFlags;
  }

  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static void set(E& value, const std::type_identity_t<E> setFlags) noexcept {
    value = value | setFlags;
  }
  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static void reset(E& value, const std::type_identity_t<E> resetFlags) noexcept {
    value = value & ~resetFlags;
  }
   */

  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static auto to_integer(const E value) noexcept {
    return static_cast<std::underlying_type_t<E>>(value);
  }

  template <impl::bitflag_enum E>
  AGT_forceinline constexpr static auto bitflag_cast(const std::underlying_type_t<E> value) noexcept {
    return static_cast<E>(value);
  }

  template <impl::bitflag_enum E, std::convertible_to<std::underlying_type_t<E>> I>
  AGT_forceinline constexpr static auto bitflag_cast(const I value) noexcept {
    return bitflag_cast<E>(static_cast<std::underlying_type_t<E>>(value));
  }
}

#endif //JEMSYS_AGATE2_INTERNAL_FLAGS_HPP
