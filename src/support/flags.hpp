//
// Created by maxwe on 2021-12-07.
//

#ifndef JEMSYS_AGATE2_INTERNAL_FLAGS_HPP
#define JEMSYS_AGATE2_INTERNAL_FLAGS_HPP

#define AGT_BITFLAG_ENUM(EnumType, UnderlyingType) \
  enum class EnumType : UnderlyingType;            \
  template <> struct agt::impl::is_bitflag_enum_st<EnumType>{}; \
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
  enum class EnumType : UnderlyingType 

namespace agt {
  namespace impl {
    template <typename T>
    struct is_bitflag_enum_st;
    template <typename T>
    concept bitflag_enum = requires {
      sizeof(is_bitflag_enum_st<T>);
    };
    struct empty_flags {
      template <bitflag_enum T>
      constexpr operator T() const noexcept {
        return static_cast<T>(0);
      }
    };
  }
  
  inline constexpr static impl::empty_flags flags_not_set = {};

  template <impl::bitflag_enum E>
  AGT_forceinline bool test(E value) noexcept {
    return static_cast<bool>(value);
  }

  template <impl::bitflag_enum E>
  AGT_forceinline bool testAny(E value, std::type_identity_t<E> testFlags) noexcept {
    return static_cast<bool>(value & testFlags);
  }
  template <impl::bitflag_enum E>
  AGT_forceinline bool testAll(E value, std::type_identity_t<E> testFlags) noexcept {
    return (value & testFlags) == testFlags;
  }

  template <impl::bitflag_enum E>
  AGT_forceinline void set(E& value, std::type_identity_t<E> setFlags) noexcept {
    value = value | setFlags;
  }
  template <impl::bitflag_enum E>
  AGT_forceinline void reset(E& value, std::type_identity_t<E> resetFlags) noexcept {
    value = value & ~resetFlags;
  }
}

#endif //JEMSYS_AGATE2_INTERNAL_FLAGS_HPP
