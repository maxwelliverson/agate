//
// Created by maxwe on 2021-12-07.
//

#ifndef JEMSYS_AGATE2_INTERNAL_FLAGS_HPP
#define JEMSYS_AGATE2_INTERNAL_FLAGS_HPP

#define AGT_BITFLAG_ENUM(EnumType, UnderlyingType) \
  enum class EnumType : UnderlyingType;            \
  template <> struct Agt::Impl::IsBitflagEnum_st<EnumType>{}; \
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

namespace Agt {
  namespace Impl {
    template <typename T>
    struct IsBitflagEnum_st;
    template <typename T>
    concept BitflagEnum = requires {
      sizeof(IsBitflagEnum_st<T>);
    };
    struct EmptyFlags {
      template <BitflagEnum T>
      constexpr operator T() const noexcept {
        return static_cast<T>(0);
      }
    };
  }
  inline constexpr static Impl::EmptyFlags FlagsNotSet = {};

  template <Impl::BitflagEnum E>
  AGT_forceinline bool test(E value) noexcept {
    return static_cast<bool>(value);
  }

  template <Impl::BitflagEnum E>
  AGT_forceinline bool testAny(E value, std::type_identity_t<E> testFlags) noexcept {
    return static_cast<bool>(value & testFlags);
  }
  template <Impl::BitflagEnum E>
  AGT_forceinline bool testAll(E value, std::type_identity_t<E> testFlags) noexcept {
    return (value & testFlags) == testFlags;
  }

  template <Impl::BitflagEnum E>
  AGT_forceinline void set(E& value, std::type_identity_t<E> setFlags) noexcept {
    value = value | setFlags;
  }
  template <Impl::BitflagEnum E>
  AGT_forceinline void reset(E& value, std::type_identity_t<E> resetFlags) noexcept {
    value = value & ~resetFlags;
  }
}

#endif //JEMSYS_AGATE2_INTERNAL_FLAGS_HPP
