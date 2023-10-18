//
// Created by maxwe on 2022-12-18.
//

#ifndef AGATE_INTERNAL_VERSION_HPP
#define AGATE_INTERNAL_VERSION_HPP

#include "agate.h"

#include <compare>

namespace agt {

  class version {
    agt_u32_t m_value = 0;

    inline constexpr static size_t MajorBits  = 10;
    inline constexpr static size_t MinorBits  = 10;
    inline constexpr static size_t PatchBits  = 12;

    inline constexpr static size_t MinorShift = PatchBits;
    inline constexpr static size_t MajorShift = MinorShift + MinorBits;

    inline constexpr static agt_u32_t MinorMask = (0x1 << MinorBits) - 1;
    inline constexpr static agt_u32_t PatchMask = (0x1 << PatchBits) - 1;

  public:

    constexpr version() = default;

    constexpr explicit version(agt_u32_t major, agt_u32_t minor = 0, agt_u32_t patch = 0) noexcept
        : m_value(AGT_MAKE_VERSION(major, minor, patch)) { }

    [[nodiscard]] constexpr agt_u32_t major() const noexcept {
      return m_value >> MajorShift;
    }

    [[nodiscard]] constexpr agt_u32_t minor() const noexcept {
      return (m_value >> MinorShift) & MinorMask;
    }

    [[nodiscard]] constexpr agt_u32_t patch() const noexcept {
      return m_value & PatchMask;
    }

    [[nodiscard]] constexpr agt_u32_t to_u32() const noexcept {
      return m_value;
    }

    [[nodiscard]] constexpr agt_i32_t to_i32() const noexcept {
      return m_value;
    }

    constexpr static version from_integer(agt_u32_t value) noexcept {
      version v;
      v.m_value = value;
      return v;
    }

    constexpr friend bool                 operator==(version a, version b) noexcept {
      return a.m_value == b.m_value;
    }
    constexpr friend std::strong_ordering operator<=>(version a, version b) noexcept {
      return a.m_value <=> b.m_value;
    }
  };


}

#endif//AGATE_INTERNAL_VERSION_HPP
