//
// Created by Maxwell on 2023-12-27.
//

#ifndef AGATE_KEY_INFO_HPP
#define AGATE_KEY_INFO_HPP

#include "config.hpp"

namespace agt {
  template <typename T>
  struct nonnull;

  template <typename T>
  struct key_info;
  //static inline T get_empty_key();
  //static inline T get_tombstone_key();
  //static unsigned get_hash_value(const T &Val);
  //static bool     is_equal(const T &LHS, const T &RHS);

  template <>
  struct key_info<int> {
    static inline int get_empty_key() {
      return INT32_MIN;
    }
    static inline int get_tombstone_key() {
      return INT32_MIN + 1;
    }
    static unsigned get_hash_value(int val) noexcept {
      return val * 37;
    }
    static bool     is_equal(int a, int b) noexcept {
      return a == b;
    }
  };

  template <typename E> requires std::is_enum_v<E>
  struct key_info<E> {

    using int_key_info = key_info<std::underlying_type_t<E>>;

    static inline E get_empty_key() {
      return static_cast<E>(int_key_info::get_empty_key());
    }
    static inline E get_tombstone_key() {
      return static_cast<E>(int_key_info::get_tombstone_key());
    }
    static unsigned get_hash_value(E val) noexcept {
      return int_key_info::get_hash_value(std::to_underlying(val));
    }
    static bool     is_equal(E a, E b) noexcept {
      return a == b;
    }
  };

  template <typename T>
  struct key_info<T*> {

    inline static constexpr uintptr_t MaxAlignBits = 12;
    inline static constexpr uintptr_t MaxAlign     = (0x1ULL << MaxAlignBits);

    using key_type = T*;

    static inline key_type get_empty_key() noexcept {
      return reinterpret_cast<T*>(static_cast<uintptr_t>(-1) << MaxAlignBits);
    }
    static inline key_type get_tombstone_key() noexcept {
      return reinterpret_cast<T*>(static_cast<uintptr_t>(-2) << MaxAlignBits);
    }
    static unsigned        get_hash_value(const T* val) noexcept {
      return (unsigned((uintptr_t)val) >> 4) ^
             (unsigned((uintptr_t)val) >> 9);
    }
    static bool            is_equal(const T* a, const T* b) noexcept {
      return a == b;
    }
  };

  template <typename T>
  struct key_info<nonnull<T*>> {

    using key_type = T*;

    static inline T* get_empty_key()     noexcept {
      return nullptr;
    }
    static inline T* get_tombstone_key() noexcept {
      return reinterpret_cast<T*>(static_cast<uintptr_t>(-1));
    }
    static unsigned  get_hash_value(const T* val) noexcept {
      return key_info<T*>::get_hash_value(val);
    }
    static bool      is_equal(const T* a, const T* b) noexcept {
      return a == b;
    }
  };

  namespace impl {
    struct no_value{};
  }
}

#endif //AGATE_KEY_INFO_HPP
