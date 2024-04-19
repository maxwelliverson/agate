//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_ATOMIC_HPP
#define JEMSYS_AGATE2_ATOMIC_HPP

#include "agate.h"

#include "align.hpp"
#include "atomicutils.hpp"


#include <bit>
#include <optional>



#define AGT_LONG_TIMEOUT_THRESHOLD AGT_TIMEOUT_MS(20)


#define REPEAT_STMT_x2(stmt) stmt stmt
#define REPEAT_STMT_x4(stmt) REPEAT_STMT_x2(REPEAT_STMT_x2(stmt))
#define REPEAT_STMT_x8(stmt) REPEAT_STMT_x2(REPEAT_STMT_x4(stmt))
#define REPEAT_STMT_x16(stmt) REPEAT_STMT_x4(REPEAT_STMT_x4(stmt))
#define REPEAT_STMT_x32(stmt) REPEAT_STMT_x2(REPEAT_STMT_x16(stmt))

#define PAUSE_x1()  _mm_pause()
#define PAUSE_x2()  do { REPEAT_STMT_x2(PAUSE_x1();) } while(false)
#define PAUSE_x4()  do { REPEAT_STMT_x4(PAUSE_x1();) } while(false)
#define PAUSE_x8()  do { REPEAT_STMT_x8(PAUSE_x1();) } while(false)
#define PAUSE_x16() do { REPEAT_STMT_x16(PAUSE_x1();) } while(false)
#define PAUSE_x32() do { REPEAT_STMT_x32(PAUSE_x1();) } while(false)

#define DUFFS_MACHINE_EX(backoff, ...) switch (backoff) { \
   default:                                        \
   { __VA_ARGS__ }                                 \
   break;                                          \
 case 5:                                           \
   PAUSE_x16();                                    \
   [[fallthrough]];                                \
 case 4:                                           \
   PAUSE_x8();                                     \
   [[fallthrough]];                                \
 case 3:                                           \
   PAUSE_x4();                                     \
   [[fallthrough]];                                \
 case 2:                                           \
   PAUSE_x2();                                     \
   [[fallthrough]];                                \
 case 1:                                           \
   PAUSE_x1();                                     \
   [[fallthrough]];                                \
 case 0:                                           \
   PAUSE_x1();                                     \
   }                                               \
   ++backoff

#define DUFFS_MACHINE(backoff) switch (backoff) { \
default:                                          \
  PAUSE_x32();                                    \
  [[fallthrough]];                                \
case 5:                                           \
  PAUSE_x16();                                    \
  [[fallthrough]];                                \
case 4:                                           \
  PAUSE_x8();                                     \
  [[fallthrough]];                                \
case 3:                                           \
  PAUSE_x4();                                     \
  [[fallthrough]];                                \
case 2:                                           \
  PAUSE_x2();                                     \
  [[fallthrough]];                                \
case 1:                                           \
  PAUSE_x1();                                     \
  [[fallthrough]];                                \
case 0:                                           \
  PAUSE_x1();                                     \
  }                                               \
  ++backoff

namespace agt {

  namespace impl {
    template <size_t N>
    struct unsigned_size;
    template <>
    struct unsigned_size<1> { using type = agt_u8_t; };
    template <>
    struct unsigned_size<2> { using type = agt_u16_t; };
    template <>
    struct unsigned_size<4> { using type = agt_u32_t; };
    template <>
    struct unsigned_size<8> { using type = agt_u64_t; };

    template <typename T> requires (std::is_trivial_v<T>)
    using atomic_type_t = typename unsigned_size<sizeof(T)>::type;

    template <typename T>
    concept atomic_capable = std::is_trivial_v<T> && requires
    {
      typename unsigned_size<sizeof(T)>::type;
    };
    template <typename T>
    concept atomic_cas_capable = atomic_capable<T> || (std::is_trivial_v<T> && (sizeof(T) == 16));
  }

  class deadline {


    inline constexpr static agt_u64_t Nanoseconds  = 1;
    inline constexpr static agt_u64_t NativePeriod = 100;
    inline constexpr static agt_u64_t Microseconds = 1'000;
    inline constexpr static agt_u64_t Milliseconds = 1'000'000;



    deadline(agt_u64_t timestamp) noexcept : timestamp(timestamp) { }

  public:

    /*[[nodiscard]] AGT_forceinline static deadline fromTimeoutOrNull(agt_timeout_t timeout) noexcept {
      if ()
    }*/

    [[nodiscard]] AGT_forceinline static deadline fromTimeout(agt_timeout_t timeout) noexcept {
      return { getCurrentTimestamp() + timeout };
    }

    [[nodiscard]] AGT_forceinline agt_u32_t     toTimeoutMs() const noexcept {
      return toTimeout() / (Milliseconds / NativePeriod);
    }

    [[nodiscard]] AGT_forceinline agt_timeout_t toTimeout() const noexcept {
      return timestamp - getCurrentTimestamp();
    }

    [[nodiscard]] AGT_forceinline bool hasPassed() const noexcept {
      return getCurrentTimestamp() >= timestamp;
    }
    [[nodiscard]] AGT_forceinline bool hasNotPassed() const noexcept {
      return getCurrentTimestamp() < timestamp;
    }

    [[nodiscard]] AGT_forceinline bool isLong() const noexcept {
      return toTimeout() >= (AGT_LONG_TIMEOUT_THRESHOLD * (Microseconds / NativePeriod));
    }

  private:

    static agt_u64_t getCurrentTimestamp() noexcept;

    agt_u64_t timestamp;
  };


  /**
   *
   * Memory Semantics of Atomic Operations
   *
   * atomic_store --------------- Release
   * atomic_load  --------------- Acquire
   * atomic_relaxed_store ------- Relaxed
   * atomic_relaxed_load -------- Relaxed
   * atomic_store_if_equals ----- Acquire (essentially a CAS operation that does not need the actual stored value in case of failure)
   * atomic_cas ----------------- Acquire, Release
   * atomic_strict_cas ---------- Acquire, Release
   * atomic_exchange ------------ Acquire, Release
   * atomic_exchange_add -------- Acquire, Release
   * atomic_exchange_sub -------- Acquire, Release
   * atomic_exchange_and -------- Acquire, Release
   * atomic_exchange_nand ------- Acquire, Release
   * atomic_exchange_or --------- Acquire, Release
   * atomic_exchange_xor -------- Acquire, Release
   * atomic_add ----------------- Release
   * atomic_sub ----------------- Release
   * atomic_and ----------------- Release
   * atomic_nand ---------------- Release
   * atomic_or ------------------ Release
   * atomic_xor ----------------- Release
   * atomic_and ----------------- Release
   * atomic_increment ----------- Acquire, Release
   * atomic_relaxed_increment --- Relaxed (Special case for implementing reference counting).
   * atomic_decrement ----------- Acquire, Release
   *
   *
   *
   *
   * */



  void        atomic_store(agt_u8_t& value,  agt_u8_t newValue) noexcept;
  void        atomic_store(agt_u16_t& value, agt_u16_t newValue) noexcept;
  void        atomic_store(agt_u32_t& value, agt_u32_t newValue) noexcept;
  void        atomic_store(agt_u64_t& value, agt_u64_t newValue) noexcept;
  template <impl::atomic_capable T>
  inline void atomic_store(T& val, std::type_identity_t<T> newValue) noexcept {
    atomic_store(*reinterpret_cast<impl::atomic_type_t<T>*>(&val), std::bit_cast<impl::atomic_type_t<T>>(newValue));
  }
  template <typename T>
  inline void atomic_store(T*& value, std::type_identity_t<T>* newValue) noexcept {
    atomic_store(reinterpret_cast<agt_u64_t&>(value), reinterpret_cast<agt_u64_t>(newValue));
  }

  agt_u8_t  atomic_load(const agt_u8_t& value) noexcept;
  agt_u16_t atomic_load(const agt_u16_t& value) noexcept;
  agt_u32_t atomic_load(const agt_u32_t& value) noexcept;
  agt_u64_t atomic_load(const agt_u64_t& value) noexcept;
  template <impl::atomic_capable T>
  inline T  atomic_load(const T& val) noexcept {
    return std::bit_cast<T>(atomic_load(*reinterpret_cast<const impl::atomic_type_t<T>*>(&val)));
  }
  template <typename T>
  inline T* atomic_load(T* const & value) noexcept {
    return reinterpret_cast<T*>(atomic_load(reinterpret_cast<const agt_u64_t&>(value)));
  }

  void       atomic_relaxed_store(agt_u8_t& value,  agt_u8_t newValue) noexcept;
  void       atomic_relaxed_store(agt_u16_t& value, agt_u16_t newValue) noexcept;
  void       atomic_relaxed_store(agt_u32_t& value, agt_u32_t newValue) noexcept;
  void       atomic_relaxed_store(agt_u64_t& value, agt_u64_t newValue) noexcept;
  template <impl::atomic_capable T>
  inline void atomic_relaxed_store(T& val, std::type_identity_t<T> newValue) noexcept {
    atomic_relaxed_store(*reinterpret_cast<impl::atomic_type_t<T>*>(&val), std::bit_cast<impl::atomic_type_t<T>>(newValue));
  }
  template <typename T>
  inline void atomic_relaxed_store(T*& value, std::type_identity_t<T>* newValue) noexcept {
    atomic_relaxed_store(reinterpret_cast<agt_u64_t&>(value), reinterpret_cast<agt_u64_t>(newValue));
  }

  agt_u8_t  atomic_relaxed_load(const agt_u8_t& value) noexcept;
  agt_u16_t atomic_relaxed_load(const agt_u16_t& value) noexcept;
  agt_u32_t atomic_relaxed_load(const agt_u32_t& value) noexcept;
  agt_u64_t atomic_relaxed_load(const agt_u64_t& value) noexcept;
  template <impl::atomic_capable T>
  inline T  atomic_relaxed_load(const T& val) noexcept {
    return std::bit_cast<T>(atomic_relaxed_load(*reinterpret_cast<const impl::atomic_type_t<T>*>(&val)));
  }
  template <typename T>
  inline T* atomic_relaxed_load(T* const & value) noexcept {
    return reinterpret_cast<T*>(atomic_relaxed_load(reinterpret_cast<const agt_u64_t&>(value)));
  }

  agt_u8_t  atomic_exchange(agt_u8_t& value,  agt_u8_t newValue) noexcept;
  agt_u16_t atomic_exchange(agt_u16_t& value, agt_u16_t newValue) noexcept;
  agt_u32_t atomic_exchange(agt_u32_t& value, agt_u32_t newValue) noexcept;
  agt_u64_t atomic_exchange(agt_u64_t& value, agt_u64_t newValue) noexcept;
  template <impl::atomic_capable T>
  inline T  atomic_exchange(T& value, std::type_identity_t<T> newValue) noexcept {
    using type_t = impl::atomic_type_t<T>;
    return std::bit_cast<T>(atomic_exchange(reinterpret_cast<type_t&>(value), std::bit_cast<type_t>(newValue)));
  }
  template <typename T>
  inline T* atomic_exchange(T*& value, std::type_identity_t<T>* newValue) noexcept {
    return reinterpret_cast<T*>(atomic_exchange(reinterpret_cast<agt_u64_t&>(value), reinterpret_cast<agt_u64_t>(newValue)));
  }

  bool        atomic_try_replace(agt_u8_t& value,  agt_u8_t  compare, agt_u8_t newValue) noexcept;
  bool        atomic_try_replace(agt_u16_t& value, agt_u16_t compare, agt_u16_t newValue) noexcept;
  bool        atomic_try_replace(agt_u32_t& value, agt_u32_t compare, agt_u32_t newValue) noexcept;
  bool        atomic_try_replace(agt_u64_t& value, agt_u64_t compare, agt_u64_t newValue) noexcept;
  bool        atomic_try_replace_16bytes(void* value, void* compare, const void* newValue) noexcept;
  template <impl::atomic_cas_capable T>
  inline bool atomic_try_replace(T& value, std::type_identity_t<T> compare, std::type_identity_t<T> newValue) noexcept {
    if constexpr (sizeof(T) == 16) {
      return atomic_try_replace_16bytes(&value, &compare, &newValue);
    }
    else {
      using type_t = impl::atomic_type_t<T>;
      return atomic_try_replace(reinterpret_cast<type_t&>(value), std::bit_cast<type_t>(compare), std::bit_cast<type_t>(newValue));
    }

  }
  template <typename T>
  inline bool atomic_try_replace(T*& value, T*& compare, std::type_identity_t<T>* newValue) noexcept {
    return atomic_try_replace(reinterpret_cast<agt_u64_t&>(value), reinterpret_cast<agt_u64_t&>(compare), reinterpret_cast<agt_u64_t>(newValue));
  }

  bool        atomic_cas(agt_u8_t& value,  agt_u8_t&  compare, agt_u8_t newValue) noexcept;
  bool        atomic_cas(agt_u16_t& value, agt_u16_t& compare, agt_u16_t newValue) noexcept;
  bool        atomic_cas(agt_u32_t& value, agt_u32_t& compare, agt_u32_t newValue) noexcept;
  bool        atomic_cas(agt_u64_t& value, agt_u64_t& compare, agt_u64_t newValue) noexcept;
  bool        atomic_cas_16bytes(void* value, void* compare, const void* newValue) noexcept;
  // value must have 4 bytes of padding at the end of it;
  template <impl::atomic_cas_capable T>
  inline bool atomic_cas(T& value, std::type_identity_t<T>& compare, std::type_identity_t<T> newValue) noexcept {
    if constexpr (sizeof(T) == 16) {
      return atomic_cas_16bytes(&value, &compare, &newValue);
    }
    else {
      using type_t = impl::atomic_type_t<T>;
      return atomic_cas(reinterpret_cast<type_t&>(value), reinterpret_cast<type_t&>(compare), std::bit_cast<type_t>(newValue));
    }

  }
  template <typename T>
  inline bool atomic_cas(T*& value, T*& compare, std::type_identity_t<T>* newValue) noexcept {
    return atomic_cas(reinterpret_cast<agt_u64_t&>(value), reinterpret_cast<agt_u64_t&>(compare), reinterpret_cast<agt_u64_t>(newValue));
  }

  agt_u8_t  atomic_increment(agt_u8_t& value) noexcept;
  agt_u16_t atomic_increment(agt_u16_t& value) noexcept;
  agt_u32_t atomic_increment(agt_u32_t& value) noexcept;
  agt_u64_t atomic_increment(agt_u64_t& value) noexcept;

  template <impl::atomic_capable T>
  inline static T atomic_increment(T& value) noexcept {
    return atomic_increment(*reinterpret_cast<impl::atomic_type_t<T>*>(&value));
  }

  agt_u8_t  atomic_relaxed_increment(agt_u8_t& value) noexcept;
  agt_u16_t atomic_relaxed_increment(agt_u16_t& value) noexcept;
  agt_u32_t atomic_relaxed_increment(agt_u32_t& value) noexcept;
  agt_u64_t atomic_relaxed_increment(agt_u64_t& value) noexcept;

  agt_u8_t  atomic_decrement(agt_u8_t& value) noexcept;
  agt_u16_t atomic_decrement(agt_u16_t& value) noexcept;
  agt_u32_t atomic_decrement(agt_u32_t& value) noexcept;
  agt_u64_t atomic_decrement(agt_u64_t& value) noexcept;

  template <impl::atomic_capable T>
  inline static T atomic_decrement(T& value) noexcept {
    return atomic_decrement(*reinterpret_cast<impl::atomic_type_t<T>*>(&value));
  }

  agt_u8_t  atomic_exchange_add(agt_u8_t&  value, agt_u8_t  newValue) noexcept;
  agt_u16_t atomic_exchange_add(agt_u16_t& value, agt_u16_t newValue) noexcept;
  agt_u32_t atomic_exchange_add(agt_u32_t& value, agt_u32_t newValue) noexcept;
  agt_u64_t atomic_exchange_add(agt_u64_t& value, agt_u64_t newValue) noexcept;

  template <impl::atomic_capable T>
  inline static T atomic_exchange_add(T& value, std::type_identity_t<T> secondValue) noexcept {
    return std::bit_cast<T>(atomic_exchange_add(*reinterpret_cast<impl::atomic_type_t<T>*>(&value), std::bit_cast<impl::atomic_type_t<T>>(secondValue)));
  }

  void      atomic_add(agt_u8_t&  value, agt_u8_t  newValue) noexcept;
  void      atomic_add(agt_u16_t& value, agt_u16_t newValue) noexcept;
  void      atomic_add(agt_u32_t& value, agt_u32_t newValue) noexcept;
  void      atomic_add(agt_u64_t& value, agt_u64_t newValue) noexcept;

  template <impl::atomic_capable T>
  inline static void atomic_add(T& value, std::type_identity_t<T> secondValue) noexcept {
    atomic_add(*reinterpret_cast<impl::atomic_type_t<T>*>(&value), std::bit_cast<impl::atomic_type_t<T>>(secondValue));
  }

  agt_u8_t  atomic_exchange_sub(agt_u8_t&  value, agt_u8_t  newValue) noexcept;
  agt_u16_t atomic_exchange_sub(agt_u16_t& value, agt_u16_t newValue) noexcept;
  agt_u32_t atomic_exchange_sub(agt_u32_t& value, agt_u32_t newValue) noexcept;
  agt_u64_t atomic_exchange_sub(agt_u64_t& value, agt_u64_t newValue) noexcept;

  template <impl::atomic_capable T>
  inline static T atomic_exchange_sub(T& value, std::type_identity_t<T> secondValue) noexcept {
    return std::bit_cast<T>(atomic_exchange_sub(*reinterpret_cast<impl::atomic_type_t<T>*>(&value), std::bit_cast<impl::atomic_type_t<T>>(secondValue)));
  }

  void      atomic_sub(agt_u8_t&  value, agt_u8_t  newValue) noexcept;
  void      atomic_sub(agt_u16_t& value, agt_u16_t newValue) noexcept;
  void      atomic_sub(agt_u32_t& value, agt_u32_t newValue) noexcept;
  void      atomic_sub(agt_u64_t& value, agt_u64_t newValue) noexcept;

  template <impl::atomic_capable T>
  inline static void atomic_sub(T& value, std::type_identity_t<T> secondValue) noexcept {
    atomic_sub(*reinterpret_cast<impl::atomic_type_t<T>*>(&value), std::bit_cast<impl::atomic_type_t<T>>(secondValue));
  }

  agt_u8_t  atomic_exchange_and(agt_u8_t&  value, agt_u8_t  newValue) noexcept;
  agt_u16_t atomic_exchange_and(agt_u16_t& value, agt_u16_t newValue) noexcept;
  agt_u32_t atomic_exchange_and(agt_u32_t& value, agt_u32_t newValue) noexcept;
  agt_u64_t atomic_exchange_and(agt_u64_t& value, agt_u64_t newValue) noexcept;

  void      atomic_and(agt_u8_t& value, agt_u8_t newValue)   noexcept;
  void      atomic_and(agt_u16_t& value, agt_u16_t newValue) noexcept;
  void      atomic_and(agt_u32_t& value, agt_u32_t newValue) noexcept;
  void      atomic_and(agt_u64_t& value, agt_u64_t newValue) noexcept;

  agt_u8_t  atomic_exchange_or(agt_u8_t&  value, agt_u8_t newValue) noexcept;
  agt_u16_t atomic_exchange_or(agt_u16_t& value, agt_u16_t newValue) noexcept;
  agt_u32_t atomic_exchange_or(agt_u32_t& value, agt_u32_t newValue) noexcept;
  agt_u64_t atomic_exchange_or(agt_u64_t& value, agt_u64_t newValue) noexcept;

  void      atomic_or(agt_u8_t&  value, agt_u8_t newValue) noexcept;
  void      atomic_or(agt_u16_t& value, agt_u16_t newValue) noexcept;
  void      atomic_or(agt_u32_t& value, agt_u32_t newValue) noexcept;
  void      atomic_or(agt_u64_t& value, agt_u64_t newValue) noexcept;

  agt_u8_t  atomic_exchange_xor(agt_u8_t&  value, agt_u8_t newValue) noexcept;
  agt_u16_t atomic_exchange_xor(agt_u16_t& value, agt_u16_t newValue) noexcept;
  agt_u32_t atomic_exchange_xor(agt_u32_t& value, agt_u32_t newValue) noexcept;
  agt_u64_t atomic_exchange_xor(agt_u64_t& value, agt_u64_t newValue) noexcept;

  void      atomic_xor(agt_u8_t&  value, agt_u8_t newValue) noexcept;
  void      atomic_xor(agt_u16_t& value, agt_u16_t newValue) noexcept;
  void      atomic_xor(agt_u32_t& value, agt_u32_t newValue) noexcept;
  void      atomic_xor(agt_u64_t& value, agt_u64_t newValue) noexcept;

  agt_u8_t  atomic_exchange_nand(agt_u8_t&  value, agt_u8_t  newValue) noexcept;
  agt_u16_t atomic_exchange_nand(agt_u16_t& value, agt_u16_t newValue) noexcept;
  agt_u32_t atomic_exchange_nand(agt_u32_t& value, agt_u32_t newValue) noexcept;
  agt_u64_t atomic_exchange_nand(agt_u64_t& value, agt_u64_t newValue) noexcept;

  void      atomic_nand(agt_u8_t&  value, agt_u8_t newValue) noexcept;
  void      atomic_nand(agt_u16_t& value, agt_u16_t newValue) noexcept;
  void      atomic_nand(agt_u32_t& value, agt_u32_t newValue) noexcept;
  void      atomic_nand(agt_u64_t& value, agt_u64_t newValue) noexcept;

  
  namespace impl {
    template <typename IntType_>
    class generic_atomic_flags {
    public:
      
      using IntType = IntType_;
    protected:

      generic_atomic_flags() = default;
      generic_atomic_flags(IntType flags) noexcept : bits(flags){}


      AGT_nodiscard AGT_forceinline bool test(IntType flags) const noexcept {
        return this->testAny(flags);
      }

      AGT_nodiscard AGT_forceinline bool testAny(IntType flags) const noexcept {
        return static_cast<bool>(atomic_load(bits) & flags);
      }
      AGT_nodiscard AGT_forceinline bool testAll(IntType flags) const noexcept {
        return (atomic_load(bits) & flags) == flags;
      }
      AGT_nodiscard AGT_forceinline bool testAny() const noexcept {
        return static_cast<bool>(atomic_load(bits));
      }

      AGT_nodiscard AGT_forceinline bool testAndSet(IntType flags) noexcept {
        return testAnyAndSet(flags);
      }
      AGT_nodiscard AGT_forceinline bool testAnyAndSet(IntType flags) noexcept {
        return static_cast<bool>(atomic_exchange_or(bits, flags) & flags);
      }
      AGT_nodiscard AGT_forceinline bool testAllAndSet(IntType flags) noexcept {
        return (atomic_exchange_or(bits, flags) & flags) == flags;
      }

      AGT_nodiscard AGT_forceinline bool testAndReset(IntType flags) noexcept {
        return testAnyAndReset(flags);
      }
      AGT_nodiscard AGT_forceinline bool testAnyAndReset(IntType flags) noexcept {
        return static_cast<bool>(atomic_exchange_and(bits, ~flags) & flags);
      }
      AGT_nodiscard AGT_forceinline bool testAllAndReset(IntType flags) noexcept {
        return (atomic_exchange_and(bits, ~flags) & flags) == flags;
      }

      AGT_nodiscard AGT_forceinline bool testAndFlip(IntType flags) noexcept {
        return testAnyAndFlip(flags);
      }
      AGT_nodiscard AGT_forceinline bool testAnyAndFlip(IntType flags) noexcept {
        return static_cast<bool>(atomic_exchange_xor(bits, flags) & flags);
      }
      AGT_nodiscard AGT_forceinline bool testAllAndFlip(IntType flags) noexcept {
        return (atomic_exchange_xor(bits, flags) & flags) == flags;
      }

      AGT_nodiscard AGT_forceinline IntType fetch() const noexcept {
        return atomic_load(bits);
      }
      AGT_nodiscard AGT_forceinline IntType fetch(IntType flags) const noexcept {
        return atomic_load(bits) & flags;
      }

      AGT_nodiscard AGT_forceinline IntType fetchAndSet(IntType flags) noexcept {
        return atomic_exchange_or(bits, flags);
      }
      AGT_nodiscard AGT_forceinline IntType fetchAndReset(IntType flags) noexcept {
        return atomic_exchange_and(bits, ~flags);
      }
      AGT_nodiscard AGT_forceinline IntType fetchAndFlip(IntType flags) noexcept {
        return atomic_exchange_xor(bits, flags);
      }

      AGT_nodiscard AGT_forceinline IntType fetchAndClear() noexcept {
        return atomic_exchange(bits, static_cast<IntType>(0));
      }

      AGT_forceinline void set(IntType flags) noexcept {
        atomic_exchange_or(bits, flags);
      }
      AGT_forceinline void reset(IntType flags) noexcept {
        atomic_exchange_and(bits, ~flags);
      }
      AGT_forceinline void flip(IntType flags) noexcept {
        atomic_exchange_xor(bits, flags);
      }

      AGT_forceinline void clear() noexcept {
        reset();
      }
      AGT_forceinline void clearAndSet(IntType flags) noexcept {
        atomic_store(bits, flags);
      }

      AGT_forceinline void reset() noexcept {
        atomic_store(bits, static_cast<IntType>(0));
      }


      AGT_forceinline void waitExact(IntType flags) const noexcept {
        IntType capturedFlags;
        capturedFlags = bits.load(std::memory_order_relaxed);
        while( capturedFlags != flags ) {
          WaitOnAddress((volatile void*)&bits, &capturedFlags, sizeof(IntType), INFINITE);
          capturedFlags = bits.load(std::memory_order_relaxed);
        }
      }
      AGT_forceinline void waitAny(IntType flags) const noexcept {
        IntType capturedFlags;
        capturedFlags = bits.load(std::memory_order_relaxed);
        while( (capturedFlags & flags) == 0 ) {
          WaitOnAddress((volatile void*)&bits, &capturedFlags, sizeof(IntType), INFINITE);
          capturedFlags = bits.load(std::memory_order_relaxed);
        }
      }
      AGT_forceinline void waitAll(IntType flags) const noexcept {
        IntType capturedFlags;
        capturedFlags = bits.load(std::memory_order_relaxed);
        while( (capturedFlags & flags) != flags ) {
          WaitOnAddress((volatile void*)&bits, &capturedFlags, sizeof(IntType), INFINITE);
          capturedFlags = bits.load(std::memory_order_relaxed);
        }
      }

      AGT_forceinline bool waitExactUntil(IntType flags, deadline deadline) const noexcept {
        return atomicWaitUntil(bits, deadline, [flags](IntType a) noexcept { return a == flags; });
      }
      AGT_forceinline bool waitAnyUntil(IntType flags, deadline deadline) const noexcept {
        return atomicWaitUntil(bits, deadline, [flags](IntType a) noexcept { return (a & flags) != 0; });
      }
      AGT_forceinline bool waitAllUntil(IntType flags, deadline deadline) const noexcept {
        return atomicWaitUntil(bits, deadline, [flags](IntType a) noexcept { return (a & flags) == flags; });
      }

      AGT_forceinline void notifyOne() noexcept {
        atomicNotifyOne(bits);
      }
      AGT_forceinline void notifyAll() noexcept {
        atomicNotifyAll(bits);
      }

      IntType bits = 0;
    };
  }


  /*class local_binary_semaphore {
    agt_u32_t val_;
  public:

    explicit local_binary_semaphore(agt_u32_t val) : val_(val) { }

    void acquire() noexcept;
    bool tryAcquire() noexcept;
    bool tryAcquireFor(agt_timeout_t timeout) noexcept;
    bool tryAcquireUntil(deadline deadline) noexcept;
    void release() noexcept {
      atomic_relaxed_store(val_, 1);
      atomicNotifyOne(val_);
    }

  };

  class shared_binary_semaphore {
    agt_i32_t val_;
  public:

    explicit shared_binary_semaphore(agt_i32_t val) : val_(val) { }

    void acquire() noexcept;
    bool tryAcquire() noexcept;
    bool tryAcquireFor(agt_timeout_t timeout) noexcept;
    bool tryAcquireUntil(deadline deadline) noexcept;
    void release() noexcept;

  };

  class local_semaphore {
    agt_u32_t val_;
  public:

    constexpr explicit local_semaphore(agt_u32_t val) : val_(val) { }

    void acquire() noexcept {
      auto current = atomic_relaxed_load(val_);
      for (;;) {
        while ( current == 0 ) {
          atomicWait(val_, 0);
          current = atomic_relaxed_load(val_);
        }

        if ( atomic_cas(val_, current, current - 1) )
          return;
      }
    }
    bool tryAcquire() noexcept {
      if ( auto current = atomic_relaxed_load(val_) )
        return atomic_try_replace(val_, current, current - 1);
      return false;
    }
    bool try_acquire_for(agt_timeout_t timeout) noexcept {
      return try_acquire_until(deadline::fromTimeout(timeout));
    }
    bool try_acquire_until(deadline deadline) noexcept {
      auto current  = atomic_relaxed_load(val_);
      for (;;) {
        while ( current == 0 ) {
          if ( !priv_wait_until(deadline) )
            return false;
          current = atomic_relaxed_load(val_);
        }

        if ( atomic_cas(val_, current, current - 1) )
          return true;
      }
    }
    void release() noexcept {
      atomic_increment(val_);
      atomicNotifyOne(val_);
    }

  private:

    void priv_wait() const noexcept {

    }
    bool priv_wait_until(deadline deadline) const noexcept {
      if (auto current = atomic_load(val_))
        return true;
      return atomicWaitUntil(val_, deadline, 0);
    }
  };

  class shared_semaphore {
    agt_u32_t  val_;
  public:

    explicit shared_semaphore(agt_u32_t val) : val_(val) { }

    void acquire() noexcept;
    bool tryAcquire() noexcept;
    bool tryAcquireFor(agt_timeout_t timeout) noexcept;
    bool tryAcquireUntil(deadline deadline) noexcept;
    void release() noexcept;

  };

  class local_multi_semaphore {
  public:
    void acquire(agt_u32_t n) noexcept;
    bool tryAcquire(agt_u32_t n) noexcept;
    bool tryAcquireFor(agt_u32_t n, agt_timeout_t timeout) noexcept;
    bool tryAcquireUntil(agt_u32_t n, deadline deadline) noexcept;
    void release(agt_u32_t n) noexcept;
  };

  class shared_multi_semaphore {
  public:
    void acquire(agt_u32_t n) noexcept;
    bool tryAcquire(agt_u32_t n) noexcept;
    bool tryAcquireFor(agt_u32_t n, agt_timeout_t timeout) noexcept;
    bool tryAcquireUntil(agt_u32_t n, deadline deadline) noexcept;
    void release(agt_u32_t n) noexcept;
  };*/


  /*class semaphore {
    inline constexpr static agt_ptrdiff_t LeastMaxValue = (std::numeric_limits<agt_ptrdiff_t>::max)();
  public:
    AGT_nodiscard static constexpr agt_ptrdiff_t(max)() noexcept {
      return LeastMaxValue;
    }

    constexpr explicit semaphore(const agt_ptrdiff_t desired) noexcept
        : value(desired) { }

    semaphore(const semaphore&) = delete;
    semaphore& operator=(const semaphore&) = delete;

    void release(agt_ptrdiff_t update = 1) noexcept {
      if (update == 0) {
        return;
      }

      value.fetch_add(update);
      value.notify_one();
    }



    void acquire() noexcept {
      ptrdiff_t current = value.load(std::memory_order_relaxed);
      for (;;) {
        while ( current == 0 ) {
          priv_wait();
          current = value.load(std::memory_order_relaxed);
        }
        assert(current > 0 && current <= LeastMaxValue);

        if ( value.compare_exchange_weak(current, current - 1) )
          return;
      }
    }
    void acquire(agt_ptrdiff_t n) noexcept {
      for (agt_ptrdiff_t acquired = 0; acquired < n; ++acquired )
        acquire();
    }

    AGT_nodiscard bool try_acquire() noexcept {
      agt_ptrdiff_t current = value.load();
      if (current == 0)
        return false;

      assert(current > 0 && current <= LeastMaxValue);

      return value.compare_exchange_weak(current, current - 1);
    }
    AGT_nodiscard bool try_acquire(agt_ptrdiff_t n) noexcept {
      for (agt_ptrdiff_t acquired = 0; acquired < n; ++acquired ) {
        if ( !try_acquire() ) {
          release(acquired);
          return false;
        }
      }
      return true;
    }

    AGT_nodiscard bool try_acquire_for(agt_u64_t timeout_us) noexcept {
      return try_acquire_until(deadline_t::from_timeout_us(timeout_us));
    }
    AGT_nodiscard bool try_acquire_for(agt_ptrdiff_t n, agt_u64_t timeout_us) noexcept {
      return try_acquire_until(n, deadline_t::from_timeout_us(timeout_us));
    }
    AGT_nodiscard bool try_acquire_until(deadline_t deadline) noexcept {
      agt_ptrdiff_t current  = value.load(std::memory_order_relaxed);
      for (;;) {
        while ( current == 0 ) {
          if ( !priv_wait_until(deadline) )
            return false;
          current = value.load(std::memory_order_relaxed);
        }
        assert(current > 0 && current <= LeastMaxValue);
        if ( value.compare_exchange_weak(current, current - 1) )
          return true;
      }
    }
    AGT_nodiscard bool try_acquire_until(agt_ptrdiff_t n, deadline_t deadline) noexcept {
      for (agt_ptrdiff_t acquired = 0; acquired < n; ++acquired ) {
        if ( !try_acquire_until(deadline) ) {
          release(acquired);
          return false;
        }
      }
      return true;
    }

  private:

    bool priv_wait_until(deadline_t deadline) noexcept {
      agt_ptrdiff_t current = value.load();
      if ( current == 0 )
        return atomic_wait(value, 0, deadline);
      return true;
    }
    void priv_wait() noexcept {
      *//*agt_ptrdiff_t current = value.load();
      if ( current == 0 )
        WaitOnAddress(&value, &current, sizeof(current), INFINITE);*//*
      value.wait(0);
    }

    std::atomic<agt_ptrdiff_t> value;
  };
  class shared_semaphore {

    inline constexpr static agt_ptrdiff_t LeastMaxValue = (std::numeric_limits<agt_ptrdiff_t>::max)();
  public:
    AGT_nodiscard static constexpr agt_ptrdiff_t(max)() noexcept {
      return LeastMaxValue;
    }

    constexpr explicit shared_semaphore(const agt_ptrdiff_t desired) noexcept
        : value(desired) { }

    shared_semaphore(const shared_semaphore&) = delete;
    shared_semaphore& operator=(const shared_semaphore&) = delete;

    void release(agt_ptrdiff_t update = 1) noexcept {
      if (update == 0)
        return;

      value.fetch_add(update);

      const agt_ptrdiff_t waitersUpperBound = waiters.load();

      if ( waitersUpperBound == 0 ) {

      }
      else if ( waitersUpperBound <= update ) {
        value.notify_all();
      }
      else {
        for (; update != 0; --update)
          value.notify_one();
      }
    }

    void acquire() noexcept {
      ptrdiff_t current = value.load(std::memory_order_relaxed);
      for (;;) {
        while ( current == 0 ) {
          priv_wait();
          current = value.load(std::memory_order_relaxed);
        }
        assert(current > 0 && current <= LeastMaxValue);

        if ( value.compare_exchange_weak(current, current - 1) )
          return;
      }
    }
    void acquire(agt_ptrdiff_t n) noexcept {
      for (agt_ptrdiff_t acquired = 0; acquired < n; ++acquired )
        acquire();
    }

    AGT_nodiscard bool try_acquire() noexcept {
      agt_ptrdiff_t current = value.load();
      if (current == 0)
        return false;

      assert(current > 0 && current <= LeastMaxValue);

      return value.compare_exchange_weak(current, current - 1);
    }
    AGT_nodiscard bool try_acquire(agt_ptrdiff_t n) noexcept {
      for (agt_ptrdiff_t acquired = 0; acquired < n; ++acquired ) {
        if ( !try_acquire() ) {
          release(acquired);
          return false;
        }
      }
      return true;
    }

    AGT_nodiscard bool try_acquire_for(agt_u64_t timeout_us) noexcept {
      return try_acquire_until(deadline_t::from_timeout_us(timeout_us));
    }
    AGT_nodiscard bool try_acquire_for(agt_ptrdiff_t n, agt_u64_t timeout_us) noexcept {
      return try_acquire_until(n, deadline_t::from_timeout_us(timeout_us));
    }
    AGT_nodiscard bool try_acquire_until(deadline_t deadline) noexcept {
      agt_ptrdiff_t current  = value.load(std::memory_order_relaxed);
      for (;;) {
        while ( current == 0 ) {
          if ( !priv_wait_until(deadline) )
            return false;
          current = value.load(std::memory_order_relaxed);
        }
        assert(current > 0 && current <= LeastMaxValue);
        if ( value.compare_exchange_weak(current, current - 1) )
          return true;
      }
    }
    AGT_nodiscard bool try_acquire_until(agt_ptrdiff_t n, deadline_t deadline) noexcept {
      for (agt_ptrdiff_t acquired = 0; acquired < n; ++acquired ) {
        if ( !try_acquire_until(deadline) ) {
          release(acquired);
          return false;
        }
      }
      return true;
    }

  private:

    bool priv_wait_until(deadline_t deadline) noexcept {
      waiters.fetch_add(1);
      agt_ptrdiff_t current = value.load();
      bool retVal = true;
      if ( current == 0 )
        retVal = atomic_wait(value, 0, deadline);
      waiters.fetch_sub(1, std::memory_order_relaxed);
      return retVal;
    }
    void priv_wait() noexcept {
      waiters.fetch_add(1);
      agt_ptrdiff_t current = value.load();
      if ( current == 0 )
        WaitOnAddress(&value, &current, sizeof(current), INFINITE);
      waiters.fetch_sub(1, std::memory_order_relaxed);
    }

    std::atomic<agt_ptrdiff_t> value;
    std::atomic<agt_ptrdiff_t> waiters;
  };
  class binary_semaphore {
    inline constexpr static agt_ptrdiff_t LeastMaxValue = 1;
  public:
    AGT_nodiscard static constexpr agt_ptrdiff_t(max)() noexcept {
      return 1;
    }

    constexpr explicit binary_semaphore(const agt_ptrdiff_t desired) noexcept
        : value(desired) { }

    binary_semaphore(const binary_semaphore&) = delete;
    binary_semaphore& operator=(const binary_semaphore&) = delete;

    void release(const agt_ptrdiff_t update = 1) noexcept {
      if (update == 0)
        return;
      assert(update == 1);
      // TRANSITION, GH-1133: should be memory_order_release
      value.store(1, std::memory_order_release);
      value.notify_one();
    }

    void acquire() noexcept {
      for (;;) {
        agt_u8_t previous = value.exchange(0, std::memory_order_acquire);
        if (previous == 1)
          break;
        assert(previous == 0);
        value.wait(0, std::memory_order_relaxed);
      }
    }

    AGT_nodiscard bool try_acquire() noexcept {
      // TRANSITION, GH-1133: should be memory_order_acquire
      agt_u8_t previous = value.exchange(0);
      assert((previous & ~1) == 0);
      return reinterpret_cast<const bool&>(previous);
    }

    AGT_nodiscard bool try_acquire_for(agt_u64_t timeout_us) {
      return try_acquire_until(deadline_t::from_timeout_us(timeout_us));
    }

    AGT_nodiscard bool try_acquire_until(deadline_t deadline) {
      for (;;) {
        agt_u8_t previous = value.exchange(0, std::memory_order_acquire);
        if (previous == 1)
          return true;
        assert(previous == 0);
        if ( !atomic_wait(value, 0, deadline) )
          return false;
      }
    }

  private:
    std::atomic<agt_u8_t> value;
  };*/

  enum class futex_key_t : agt_u16_t;

  /*class futex {

    inline constexpr static agt_u16_t MaxIndex  = 16;
    inline constexpr static agt_u16_t IndexMask = MaxIndex - 1;

    using epoch_type = agt_u16_t;

    enum lock_state : agt_u16_t {
      NOT_ACQUIRED,
      ACQUIRED_BY_TAKE,
      ACQUIRED_BY_DROP_TRANSFER,
      ACQUIRED_BY_WAIT_TRANSFER
    };

    union lock_type {
      struct {
        epoch_type   epoch;
        lock_state   state;
        epoch_type   waiterEpoch;
        agt_u16_t    waiters;
      };
      agt_u64_t bits;
    };

    static_assert(sizeof(lock_type) == sizeof(agt_u64_t));

    [[nodiscard]] AGT_forceinline static epoch_type  _key_cast(futex_key_t key) noexcept {
      return static_cast<epoch_type>(key);
    }
    [[nodiscard]] AGT_forceinline static futex_key_t _key_cast(epoch_type epoch) noexcept {
      return static_cast<futex_key_t>(epoch);
    }

  public:

    futex() = default;

    // Locks the futex; returned key must be retained to subsequently unlock
    // Increments
    [[nodiscard]] futex_key_t take() noexcept {
      lock_type oldLock, newLock;
      oldLock.bits  = atomic_relaxed_load(m_lock.bits);
      newLock.state = ACQUIRED_BY_TAKE;

      do {
        if (oldLock.state != NOT_ACQUIRED)
          return _wait_to_take(oldLock);

        newLock.epoch       = oldLock.epoch;
        newLock.waiterEpoch = oldLock.waiterEpoch;
        newLock.waiters     = oldLock.waiters;

      } while(!atomic_cas(m_lock.bits, oldLock.bits, newLock.bits));

      return _key_cast(newLock.epoch);
    }

    [[nodiscard]] std::optional<futex_key_t> try_take() noexcept {

      lock_type oldLock, newLock;
      oldLock.bits  = atomic_relaxed_load(m_lock.bits);
      newLock.state = ACQUIRED_BY_TAKE;

      do {
        if (oldLock.state != NOT_ACQUIRED)
          return std::nullopt;
        newLock.epoch       = oldLock.epoch;
        newLock.waiterEpoch = oldLock.waiterEpoch;
        newLock.waiters     = oldLock.waiters;

      } while(!atomic_cas(m_lock.bits, oldLock.bits, newLock.bits));

      return _key_cast(newLock.epoch);
    }

    void drop(futex_key_t key) noexcept {
      lock_type oldLock, newLock;
      oldLock.bits = atomic_relaxed_load(m_lock.bits);

      bool transferOwnership;
      do {
        transferOwnership = oldLock.waiters     != 0             &&
                            oldLock.waiterEpoch != oldLock.epoch;
        newLock.waiterEpoch = oldLock.waiterEpoch;

        if (transferOwnership) {
          newLock.waiters = oldLock.waiters - 1;
          newLock.state   = ACQUIRED_BY_DROP_TRANSFER;
        }
        else {
          newLock.waiters = 0;
          newLock.state   = NOT_ACQUIRED;
        }
      } while(!atomic_cas(m_lock.bits, oldLock.bits, newLock.bits));
    }

    //
    [[nodiscard]] futex_key_t wait(futex_key_t key) noexcept {
      lock_type oldLock, newLock;
      oldLock.bits = atomic_relaxed_load(m_lock.bits);
      newLock.epoch = _key_cast(key) + 1;

      do {
        bool transferOwnership = oldLock.waiters != 0;

        if (transferOwnership) {
          newLock.waiters  = oldLock.waiters;
          newLock.spinLock = LockedValue;
        }
        else {
          newLock.waiters  = 1;
          newLock.spinLock = UnlockedValue;
        }

        if (atomic_cas(m_lock.bits, oldLock.bits, newLock.bits)) {
          if (transferOwnership) {

          }
        }

      } while(true);
    }


  private:

    [[nodiscard]] futex_key_t _wait_to_take(lock_type oldLock) noexcept {

    }

    void _transfer_to_waiter() noexcept {

    }

    void _wake_waiter() noexcept {

    }

    lock_type m_lock;
  };*/



  namespace impl {

    consteval static size_t select_align(size_t align, size_t defaultAlign) noexcept {
      if (!is_pow2(align) || !is_pow2(defaultAlign))
        return 0;
      return std::max(align, defaultAlign);
    }

    template <size_t N, size_t Align>
    class atomic_storage;

    template <size_t Size>
    struct unsigned_integer_type;
    template <>
    struct unsigned_integer_type<1>{ using type = agt_u8_t; };
    template <>
    struct unsigned_integer_type<2>{ using type = agt_u16_t; };
    template <>
    struct unsigned_integer_type<4>{ using type = agt_u32_t; };
    template <>
    struct unsigned_integer_type<8>{ using type = agt_u64_t; };

    template <size_t Size, size_t Align>
    class lockfree_atomic_storage {
      inline constexpr static size_t alignment = select_align(Align, Size);
      using storage_type = typename unsigned_integer_type<Size>::type;
    protected:
      AGT_forceinline lockfree_atomic_storage() = default;

      AGT_forceinline void _unsafe_store(const void* src) noexcept {
        std::memcpy(&m_bits, src, Size);
      }

      AGT_forceinline void _load(void* dst) const noexcept {
        *static_cast<storage_type*>(dst) = atomic_load(m_bits);
      }
      AGT_forceinline void _relaxed_load(void* dst) const noexcept {
        *static_cast<storage_type*>(dst) = atomic_relaxed_load(m_bits);
      }

      AGT_forceinline void _store(const void* src) noexcept {
        atomic_store(m_bits, *static_cast<const storage_type*>(src));
      }
      AGT_forceinline void _relaxed_store(const void* src) noexcept {
        atomic_relaxed_store(m_bits, *static_cast<const storage_type*>(src));
      }

      AGT_forceinline void _exchange(void* dst, const void* src) noexcept {
        *static_cast<storage_type*>(dst) = atomic_exchange(m_bits, *static_cast<const storage_type*>(src));
      }

      AGT_forceinline void _increment(void* dst) noexcept {
        *static_cast<storage_type*>(dst) = atomic_increment(m_bits);
      }
      AGT_forceinline void _relaxed_increment(void* dst) noexcept {
        *static_cast<storage_type*>(dst) = atomic_relaxed_increment(m_bits);
      }
      AGT_forceinline void _decrement(void* dst) noexcept {
        *static_cast<storage_type*>(dst) = atomic_decrement(m_bits);
      }

      AGT_forceinline void _exchange_add(void* dst, const void* src) noexcept {
        *static_cast<storage_type*>(dst) = atomic_exchange_add(m_bits, *static_cast<const storage_type*>(src));
      }
      AGT_forceinline void _exchange_and(void* dst, const void* src) noexcept {
        *static_cast<storage_type*>(dst) = atomic_exchange_and(m_bits, *static_cast<const storage_type*>(src));
      }
      AGT_forceinline void _exchange_or(void* dst, const void* src) noexcept {
        *static_cast<storage_type*>(dst) = atomic_exchange_or(m_bits, *static_cast<const storage_type*>(src));
      }
      AGT_forceinline void _exchange_xor(void* dst, const void* src) noexcept {
        *static_cast<storage_type*>(dst) = atomic_exchange_xor(m_bits, *static_cast<const storage_type*>(src));
      }

      AGT_forceinline bool _compare_exchange(void* compare, const void* value) noexcept {
        return atomic_try_replace(m_bits, *static_cast<storage_type*>(compare), *static_cast<const storage_type*>(value));
      }
      AGT_forceinline bool _compare_exchange_weak(void* compare, const void* value) noexcept {
        return atomic_cas(m_bits, *static_cast<storage_type*>(compare), *static_cast<const storage_type*>(value));
      }

      [[nodiscard]] AGT_forceinline void* _address() noexcept {
        return &m_bits;
      }
      [[nodiscard]] AGT_forceinline const void* _address() const noexcept {
        return &m_bits;
      }

    private:
      alignas(alignment) storage_type m_bits;
    };

    template <size_t Align>
    class lockfree_atomic_storage<16, Align> {
      inline constexpr static size_t Size = 16;
      inline constexpr static size_t alignment = select_align(Align, 16);

      inline constexpr static bool   TypeIsAligned = Align == alignment;

      struct dummy_type {
        alignas(alignment)
        agt_u64_t low;
        agt_u64_t high;
      };

      AGT_forceinline static bool      _is_aligned(const void* addr) noexcept {
        return (reinterpret_cast<uintptr_t>(addr) & static_cast<uintptr_t>(Size - 1)) == 0;
      }

      AGT_forceinline static agt_u64_t _read_low(const void* src) noexcept {
        agt_u64_t bits;
        std::memcpy(&bits, src, Size / 2);
        return bits;
      }
      AGT_forceinline static agt_u64_t _read_high(const void* src) noexcept {
        agt_u64_t bits;
        std::memcpy(&bits, ((const char*)src) + (Size / 2), Size / 2);
        return bits;
      }

      AGT_forceinline static agt_u64_t _add_bits(void* out, const void* srcA, const void* srcB) noexcept {
        const agt_u64_t lowSrcB   = static_cast<const agt_u64_t*>(srcB)[0];
        const agt_u64_t lowResult = static_cast<const agt_u64_t*>(srcA)[0] + lowSrcB;
        agt_u64_t       highSrcB  = static_cast<const agt_u64_t*>(srcB)[1];
        if (lowResult < lowSrcB)
          highSrcB += 1;
        static_cast<agt_u64_t*>(out)[0] = lowResult;
        static_cast<agt_u64_t*>(out)[1] = static_cast<const agt_u64_t*>(srcA)[1] + highSrcB;
      }

      AGT_forceinline static agt_u64_t _and_bits(void* out, const void* srcA, const void* srcB) noexcept {
        static_cast<agt_u64_t*>(out)[0] = static_cast<const agt_u64_t*>(srcA)[0] & static_cast<const agt_u64_t*>(srcB)[0];
        static_cast<agt_u64_t*>(out)[1] = static_cast<const agt_u64_t*>(srcA)[1] & static_cast<const agt_u64_t*>(srcB)[1];
      }
      AGT_forceinline static agt_u64_t _or_bits(void* out, const void* srcA, const void* srcB) noexcept {
        static_cast<agt_u64_t*>(out)[0] = static_cast<const agt_u64_t*>(srcA)[0] | static_cast<const agt_u64_t*>(srcB)[0];
        static_cast<agt_u64_t*>(out)[1] = static_cast<const agt_u64_t*>(srcA)[1] | static_cast<const agt_u64_t*>(srcB)[1];
      }
      AGT_forceinline static agt_u64_t _xor_bits(void* out, const void* srcA, const void* srcB) noexcept {
        static_cast<agt_u64_t*>(out)[0] = static_cast<const agt_u64_t*>(srcA)[0] ^ static_cast<const agt_u64_t*>(srcB)[0];
        static_cast<agt_u64_t*>(out)[1] = static_cast<const agt_u64_t*>(srcA)[1] ^ static_cast<const agt_u64_t*>(srcB)[1];
      }
    protected:

      AGT_forceinline lockfree_atomic_storage() = default;

      AGT_forceinline void _unsafe_store(const void* src) noexcept {
        std::memcpy(&m_lowBits, src, Size);
      }

      AGT_forceinline void _load(void* dst) const noexcept {
        if constexpr (!TypeIsAligned) {
          dummy_type dummy = {};
          const_cast<lockfree_atomic_storage*>(this)->_aligned_compare_exchange_weak(&dummy, &dummy);
          std::memcpy(dst, &dummy, Size);
        }
        else {
          const_cast<lockfree_atomic_storage*>(this)->_compare_exchange_weak(dst, dst);
        }
      }
      AGT_forceinline void _relaxed_load(void* dst) const noexcept {
        _load();
      }

      AGT_forceinline void _store(const void* src) noexcept {
        if constexpr (!TypeIsAligned) {
          dummy_type dummy = {};
          dummy_type dummySrc;
          // TODO: Benchmark; it might be faster to simply always copy to the aligned type rather than introducing a branch
          const bool isAligned = _is_aligned(src);
          const void* trueSrc;
          if (!isAligned) {
            std::memcpy(&dummySrc, src, Size);
            trueSrc = &dummySrc;
          }
          else {
            trueSrc = src;
          }
          while (!_aligned_compare_exchange_weak(&dummy, trueSrc));
        }
        else {
          dummy_type dummy = {};
          while (!_aligned_compare_exchange_weak(&dummy, src));
        }

      }
      AGT_forceinline void _relaxed_store(const void* src) noexcept {
        _store(src);
      }

      AGT_forceinline void _exchange(void* dst, const void* src) noexcept {
        if constexpr (!TypeIsAligned) {
          dummy_type dummySrc;
          dummy_type dummyDst;
          std::memcpy(&dummySrc, src, Size);
          const bool isAligned = _is_aligned(dst);
          void* trueDst = isAligned ? dst : &dummyDst;
          while(!_aligned_compare_exchange_weak(trueDst, &dummySrc));
          if (!isAligned)
            std::memcpy(dst, &dummyDst, Size);
        }
        else {
          while(!_aligned_compare_exchange_weak(dst, src));
        }
      }

      AGT_forceinline void _increment(void* dst) noexcept {
        if constexpr (!TypeIsAligned) {
          dummy_type dummy;
          dummy_type dstDummy;
          std::memcpy(&dummy, dst, Size);

          bool isAligned = _is_aligned(dst);
          void* trueDst = isAligned ? dst : &dstDummy;
          if (!isAligned)
            std::memcpy(&dstDummy, dst, Size);
          do {
            if (++dummy.low == 0) [[unlikely]]
              ++dummy.high;
          } while(!_aligned_compare_exchange_weak(trueDst, &dummy));
          if (!isAligned)
            std::memcpy(dst, &dstDummy, Size);
        }
        else {
          dummy_type dummy;
          std::memcpy(&dummy, dst, Size);
          do {
            if (++dummy.low == 0) [[unlikely]]
              ++dummy.high;
          } while(!_aligned_compare_exchange_weak(dst, &dummy));
        }
      }
      AGT_forceinline void _relaxed_increment(void* dst) noexcept {
        _increment(dst);
      }
      AGT_forceinline void _decrement(void* dst) noexcept {
        if constexpr (!TypeIsAligned) {
          dummy_type dummy;
          dummy_type dstDummy;
          std::memcpy(&dummy, dst, Size);

          bool isAligned = _is_aligned(dst);
          void* trueDst = isAligned ? dst : &dstDummy;
          if (!isAligned)
            std::memcpy(&dstDummy, dst, Size);
          do {
            if (dummy.low-- == 0) [[unlikely]]
              --dummy.high;
          } while(!_aligned_compare_exchange_weak(trueDst, &dummy));
          if (!isAligned)
            std::memcpy(dst, &dstDummy, Size);
        }
        else {
          dummy_type dummy;
          std::memcpy(&dummy, dst, Size);
          do {
            if (dummy.low-- == 0) [[unlikely]]
              --dummy.high;
          } while(!_aligned_compare_exchange_weak(dst, &dummy));
        }
      }

      AGT_forceinline void _exchange_add(void* dst, const void* src) noexcept {
        if constexpr (!TypeIsAligned) {
          const bool       dstIsAligned = _is_aligned(dst);
          const dummy_type dummySrc = *static_cast<const dummy_type*>(src);
          dummy_type       result;
          dummy_type       dummyDst;
          void*            trueDst = dstIsAligned ? dst : &dummyDst;

          do {
            _add_bits(&result, trueDst, src);
          } while(!_compare_exchange_weak(trueDst, &result));

          if (dstIsAligned)
            std::memcpy(dst, &dummyDst, Size);
        }
        else {
          dummy_type result;
          do {
            _add_bits(&result, dst, src);
          } while(!_compare_exchange_weak(dst, &result));
        }

      }
      AGT_forceinline void _exchange_and(void* dst, const void* src) noexcept {
        if constexpr(!TypeIsAligned) {
          const bool       dstIsAligned = _is_aligned(dst);
          dummy_type       result;
          dummy_type       dummyDst;
          void*            trueDst = dstIsAligned ? dst : &dummyDst;

          do {
            _and_bits(&result, trueDst, src);
          } while(!_aligned_compare_exchange_weak(trueDst, &result));

          if (dstIsAligned)
            std::memcpy(dst, &dummyDst, Size);
        }
        else {
          dummy_type result;
          do {
            _and_bits(&result, dst, src);
          } while(!_aligned_compare_exchange_weak(dst, &result));
        }
      }
      AGT_forceinline void _exchange_or(void* dst, const void* src) noexcept {
        if constexpr (!TypeIsAligned) {
          const bool       dstIsAligned = _is_aligned(dst);
          dummy_type       result;
          dummy_type       dummyDst;
          void*            trueDst = dstIsAligned ? dst : &dummyDst;

          do {
            _or_bits(&result, trueDst, src);
          } while(!_aligned_compare_exchange_weak(trueDst, &result));

          if (dstIsAligned)
            std::memcpy(dst, &dummyDst, Size);
        }
        else {
          dummy_type result;
          do {
            _or_bits(&result, dst, src);
          } while(!_aligned_compare_exchange_weak(dst, &result));
        }
      }
      AGT_forceinline void _exchange_xor(void* dst, const void* src) noexcept {
        if constexpr (!TypeIsAligned) {
          const bool       dstIsAligned = _is_aligned(dst);
          dummy_type       result;
          dummy_type       dummyDst;
          void*            trueDst = dstIsAligned ? dst : &dummyDst;

          do {
            _xor_bits(&result, trueDst, src);
          } while(!_aligned_compare_exchange_weak(trueDst, &result));

          if (dstIsAligned)
            std::memcpy(dst, &dummyDst, Size);
        }
        else {
          dummy_type result;
          do {
            _xor_bits(&result, dst, src);
          } while(!_aligned_compare_exchange_weak(dst, &result));
        }
      }

      AGT_forceinline bool _compare_exchange(void* compare, const void* value) noexcept {
        if constexpr (!TypeIsAligned) {
          const bool isAligned = _is_aligned(compare);
          const dummy_type dummyValue;
          dummy_type dummyCompare;
          std::memcpy(&dummyValue, value, Size);
          if (!isAligned) {
            std::memcpy(&dummyCompare, compare, Size);
            bool result = _aligned_compare_exchange(&dummyCompare, &dummyValue);
            if (!result)
              std::memcpy(compare, &dummyCompare, Size);
            return result;
          }
          else
            return _aligned_compare_exchange(compare, &dummyValue);
        }
        else {
          return _aligned_compare_exchange(compare, value);
        }
      }
      AGT_forceinline bool _compare_exchange_weak(void* compare, const void* value) noexcept {
        if constexpr (!TypeIsAligned) {
          const bool isAligned = _is_aligned(compare);
          const dummy_type dummyValue;
          dummy_type dummyCompare;
          std::memcpy(&dummyValue, value, Size);
          if (!isAligned) {
            std::memcpy(&dummyCompare, compare, Size);
            bool result = _aligned_compare_exchange_weak(&dummyCompare, &dummyValue);
            if (!result)
              std::memcpy(compare, &dummyCompare, Size);
            return result;
          }
          else
            return _aligned_compare_exchange_weak(compare, &dummyValue);
        }
        else {
          return _aligned_compare_exchange_weak(compare, value);
        }
      }

      [[nodiscard]] AGT_forceinline void*       _address() noexcept {
        return &m_lowBits;
      }
      [[nodiscard]] AGT_forceinline const void* _address() const noexcept {
        return &m_lowBits;
      }

    private:

      AGT_forceinline bool _aligned_compare_exchange(void* compare, const void* value) noexcept {
        return atomic_try_replace_16bytes(this, compare, value);
      }

      AGT_forceinline bool _aligned_compare_exchange_weak(void* compare, const void* value) noexcept {
        return atomic_cas_16bytes(this, compare, value);
      }

      alignas(alignment)
      agt_u64_t m_lowBits;
      agt_u64_t m_highBits;
    };

    template <size_t N, size_t Align>
    class locked_atomic_storage;


    template <size_t Size, size_t Align>
    class storage_and_spinlock;
    template <size_t Size, size_t Align> requires( Size > 16 && Align <= 8)
    class storage_and_spinlock<Size, Align>{
    protected:

      storage_and_spinlock() = default;

      inline constexpr static size_t alignment = 8;

      void _lock() noexcept {

      }
      void _unlock() noexcept {

      }
      [[nodiscard]] void*       _address() noexcept {
        return &m_bits;
      }
      [[nodiscard]] const void* _address() const noexcept {
        return &m_bits;
      }

    private:
      union alignas(alignment) {
        struct {
          agt_u32_t m_spinlock;
          agt_u32_t m_epoch;
        };
        agt_u64_t m_lockBits;
      };
      agt_u8_t  m_bits[Size];
    };
    template <size_t Size, size_t Align> requires( Size > 16 && Align > 8)
    class storage_and_spinlock<Size, Align>{
    protected:

      storage_and_spinlock() = default;

      inline constexpr static size_t alignment = select_align(Align, 8); // Will be Align, but this ensures Align is a power of 2

      void _lock() noexcept {

      }
      void _unlock() noexcept {

      }
      [[nodiscard]] void*       _address() noexcept {
        return &m_bits;
      }
      [[nodiscard]] const void* _address() const noexcept {
        return &m_bits;
      }

    private:
      alignas(alignment)
      agt_u8_t  m_bits[Size];
      union {
        struct {
          agt_u32_t m_spinlock;
          agt_u32_t m_epoch;
        };
        agt_u64_t m_lockBits;
      };

    };



    template <size_t N, size_t Align> requires( N > 16 && Align <= 4)
    class locked_atomic_storage<N, Align> {
      inline constexpr static size_t Alignment = 4;
    protected:
      locked_atomic_storage() = default;

    private:
      alignas(Alignment)
      agt_u32_t m_spinlock;
      agt_u8_t  m_bits[N];
    };

    template <size_t N, size_t Align> requires( N > 16 && Align > 4)
    class locked_atomic_storage<N, Align> {
      inline constexpr static size_t Alignment = select_align(Align, 4); // Will be Align, but this ensures Align is a power of 2
    protected:
      locked_atomic_storage() = default;

    private:
      alignas(Alignment)
      agt_u8_t  m_bits[N];
      agt_u32_t m_spinlock;
    };

  }



  template <typename E>
  class atomic_flags : public impl::generic_atomic_flags<std::underlying_type_t<E>> {

    using UnderlyingType = std::underlying_type_t<E>;
    using Base = impl::generic_atomic_flags<std::underlying_type_t<E>>;
    using EnumType = E;
    using IntType = std::underlying_type_t<EnumType>;


    AGT_forceinline static IntType  toInt(EnumType e) noexcept {
      return static_cast<IntType>(e);
    }
    AGT_forceinline static EnumType toEnum(IntType i) noexcept {
      return static_cast<EnumType>(i);
    }


  public:

    atomic_flags() = default;
    atomic_flags(EnumType value) noexcept : Base() {}


    AGT_nodiscard AGT_forceinline bool test(EnumType flags) const noexcept {
      return this->testAny(flags);
    }

    AGT_nodiscard AGT_forceinline bool testAny(EnumType flags) const noexcept {
      return static_cast<bool>(atomic_load(this->bits) & toInt(flags));
    }
    AGT_nodiscard AGT_forceinline bool testAll(EnumType flags) const noexcept {
      return (atomic_load(this->bits) & toInt(flags)) == toInt(flags);
    }
    AGT_nodiscard AGT_forceinline bool testAny() const noexcept {
      return static_cast<bool>(atomic_load(this->bits));
    }

    AGT_nodiscard AGT_forceinline bool testAndSet(EnumType flags) noexcept {
      return testAnyAndSet(flags);
    }
    AGT_nodiscard AGT_forceinline bool testAnyAndSet(EnumType flags) noexcept {
      return this->testAnyAndSet(toInt(flags));
      // return static_cast<bool>(atomic_exchange_or(this->bits, flags) & flags);
    }
    AGT_nodiscard AGT_forceinline bool testAllAndSet(EnumType flags) noexcept {
      return this->testAllAndSet(toInt(flags));
      // return (atomic_exchange_or(bits, flags) & flags) == flags;
    }

    AGT_nodiscard AGT_forceinline bool testAndReset(EnumType flags) noexcept {
      return testAnyAndReset(flags);
    }
    AGT_nodiscard AGT_forceinline bool testAnyAndReset(EnumType flags) noexcept {
      return this->testAnyAndReset(toInt(flags));
      // return static_cast<bool>(atomic_exchange_and(bits, ~flags) & flags);
    }
    AGT_nodiscard AGT_forceinline bool testAllAndReset(EnumType flags) noexcept {
      return this->testAllAndReset(toInt(flags));
      // return (atomic_exchange_and(bits, ~flags) & flags) == flags;
    }

    AGT_nodiscard AGT_forceinline bool testAndFlip(EnumType flags) noexcept {
      return testAnyAndFlip(flags);
    }
    AGT_nodiscard AGT_forceinline bool testAnyAndFlip(EnumType flags) noexcept {
      return this->testAnyAndFlip(toInt(flags));
      // return static_cast<bool>(atomic_exchange_xor(bits, flags) & flags);
    }
    AGT_nodiscard AGT_forceinline bool testAllAndFlip(EnumType flags) noexcept {
      return this->testAllAndFlip(toInt(flags));
      // return (atomic_exchange_xor(bits, flags) & flags) == flags;
    }

    AGT_nodiscard AGT_forceinline EnumType fetch() const noexcept {
      return toEnum(atomic_load(this->bits));
    }
    AGT_nodiscard AGT_forceinline EnumType fetch(EnumType flags) const noexcept {
      return toEnum(atomic_load(this->bits) & toInt(flags));
    }

    AGT_nodiscard AGT_forceinline EnumType fetchAndSet(EnumType flags) noexcept {
      return toEnum(this->fetchAndSet(toInt(flags)));
      // return atomic_exchange_or(bits, flags);
    }
    AGT_nodiscard AGT_forceinline EnumType fetchAndReset(EnumType flags) noexcept {
      return toEnum(this->fetchAndReset(toInt(flags)));
    }
    AGT_nodiscard AGT_forceinline EnumType fetchAndFlip(EnumType flags) noexcept {
      return toEnum(this->fetchAndFlip(toInt(flags)));
    }

    AGT_nodiscard AGT_forceinline EnumType fetchAndClear() noexcept {
      return toEnum(atomic_exchange(this->bits, static_cast<IntType>(0)));
    }

    AGT_forceinline void set(EnumType flags) noexcept {
      this->set(toInt(flags));
    }
    AGT_forceinline void reset(EnumType flags) noexcept {
      this->reset(toInt(flags));
    }
    AGT_forceinline void flip(EnumType flags) noexcept {
      this->flip(toInt(flags));
    }

    AGT_forceinline void clear() noexcept {
      reset();
    }
    AGT_forceinline void clearAndSet(EnumType flags) noexcept {
      atomic_store(this->bits, toInt(flags));
    }

    AGT_forceinline void reset() noexcept {
      atomic_store(this->bits, static_cast<IntType>(0));
    }


    AGT_forceinline void waitExact(EnumType flags) const noexcept {
      this->waitExact(toInt(flags));
    }
    AGT_forceinline void waitAny(EnumType flags) const noexcept {
      this->waitAny(toInt(flags));
    }
    AGT_forceinline void waitAll(EnumType flags) const noexcept {
      this->waitAll(toInt(flags));
    }

    AGT_forceinline bool waitExactUntil(EnumType flags, deadline deadline) const noexcept {
      return this->waitExactUntil(toInt(flags), deadline);
    }
    AGT_forceinline bool waitAnyUntil(EnumType flags, deadline deadline) const noexcept {
      return this->waitAnyUntil(toInt(flags), deadline);
    }
    AGT_forceinline bool waitAllUntil(EnumType flags, deadline deadline) const noexcept {
      return this->waitAllUntil(toInt(flags), deadline);
    }

    AGT_forceinline void notifyOne() noexcept {
      atomicNotifyOne(this->bits);
    }
    AGT_forceinline void notifyAll() noexcept {
      atomicNotifyAll(this->bits);
    }

  };


  class ref_count {
  public:
    ref_count() = default;
    explicit ref_count(agt_u32_t initialCount) noexcept : value_(initialCount) { }


    AGT_nodiscard agt_u32_t get() const noexcept {
      return atomic_relaxed_load(value_);
    }


    agt_u32_t acquire() noexcept {
      return atomic_increment(value_);
    }
    agt_u32_t acquire(agt_u32_t n) noexcept {
      return atomic_exchange_add(value_, n) + n;
    }

    agt_u32_t release() noexcept {
      return atomic_decrement(value_);
    }
    agt_u32_t release(agt_u32_t n) noexcept {
      return atomic_exchange_add(value_, 0 - n) - n;
    }

    agt_u32_t operator++()    noexcept {
      return this->acquire();
    }
    agt_u32_t operator++(int) noexcept {
      return atomic_exchange_add(value_, 1);
    }

    agt_u32_t operator--()    noexcept {
      return this->release();
    }
    agt_u32_t operator--(int) noexcept {
      return atomic_exchange_add(value_, -1);
    }

  private:

    agt_u32_t value_ = 0;
  };

  class read_write_mutex {
    inline constexpr static agt_u32_t WriteLockConstant = static_cast<agt_u32_t>(-1);

    template <typename T>
    class atomic_field;

    template <typename T> requires (sizeof(T) == sizeof(agt_u64_t))
    class atomic_field<T> {
    public:



    private:
      agt_u64_t m_bits;
    };

    union first_half {
      struct {
        agt_u32_t readerCount;
        agt_u16_t writeLockWaiting;
        agt_u16_t isWriteLocked;
      };
      agt_u64_t bits;
    };
    union second_half {
      struct {
        agt_u32_t epoch;
        agt_u32_t waiterCount;
      };
      agt_u64_t bits;
    };

    union owner_info {
      struct {
        agt_u32_t ayo;
      };
      agt_u64_t bits;
    };

  public:

    void read_lock(agt_u32_t count) noexcept {
      /*first_half tmp{
          .bits = atomic_relaxed_load(m_first.bits)
      };
      do {
        if (tmp.isWriteLocked || tmp.writeLockWaiting)

      } while();*/
    }

    bool try_read_lock(agt_u32_t count) noexcept {

    }

    bool try_read_lock_for(agt_u32_t count, agt_timeout_t timeout) noexcept {

    }

    bool try_read_lock_until(agt_u32_t count, deadline dl) noexcept {

    }

    void read_unlock(agt_u32_t count) noexcept {

    }


    void write_lock() noexcept {

    }

    bool try_write_lock() noexcept {

    }

    bool try_write_lock_for(agt_timeout_t timeout) noexcept {

    }

    bool try_write_lock_until(deadline dl) noexcept {

    }

    void write_unlock() noexcept {

    }


    void upgrade_lock(agt_u32_t count) noexcept {

    }

    bool try_upgrade_lock(agt_u32_t count) noexcept {

    }

    bool try_upgrade_lock_for(agt_u32_t count, agt_timeout_t timeout) noexcept {

    }

    bool try_upgrade_lock_until(agt_u32_t count, deadline dl) noexcept {

    }



    void downgrade_lock(agt_u32_t count) noexcept {

    }


  private:

    first_half  m_first;
    second_half m_second;
    uintptr_t   m_ownerId;
    uint32_t    m_ownerCount;
  };



  class write_lock;


  namespace impl {
    struct try_once_t {};

    struct try_for_t {
      agt_timeout_t timeout;
    };

    struct try_until_t {
      deadline dl;
    };
  }

  inline constexpr static impl::try_once_t try_once{};

  inline static impl::try_for_t   try_for(agt_timeout_t timeout) noexcept {
    return impl::try_for_t{ timeout };
  }

  inline static impl::try_until_t try_until(deadline dl) noexcept {
    return impl::try_until_t{ dl };
  }



  class read_lock {
    friend class write_lock;

    static read_write_mutex* do_lock(read_write_mutex& mut, agt_u32_t count) noexcept {
      mut.read_lock(count);
      return &mut;
    }

  public:

    explicit read_lock(read_write_mutex& mut) noexcept
        : read_lock(mut, 1) { }
    explicit read_lock(read_write_mutex& mut, impl::try_once_t) noexcept
        : read_lock(mut, 1, try_once) { }
    explicit read_lock(read_write_mutex& mut, impl::try_for_t tryFor) noexcept
        : read_lock(mut, 1, tryFor) { }
    explicit read_lock(read_write_mutex& mut, impl::try_until_t tryUntil) noexcept
        : read_lock(mut, 1, tryUntil) { }
    explicit read_lock(read_write_mutex& mut, agt_u32_t count) noexcept
        : m_mut(do_lock(mut, count)), m_count(count) { }
    explicit read_lock(read_write_mutex& mut, agt_u32_t count, impl::try_once_t) noexcept
        : m_mut(mut.try_read_lock(count) ? &mut : nullptr), m_count(count) { }
    explicit read_lock(read_write_mutex& mut, agt_u32_t count, impl::try_for_t tryFor) noexcept
        : m_mut(mut.try_read_lock_for(count, tryFor.timeout) ? &mut : nullptr), m_count(count) { }
    explicit read_lock(read_write_mutex& mut, agt_u32_t count, impl::try_until_t tryUntil) noexcept
        : m_mut(mut.try_read_lock_until(count, tryUntil.dl) ? &mut : nullptr), m_count(count) { }

    ~read_lock() {
      unlock();
    }

    void release() noexcept {
      unlock();
    }

    void unlock() noexcept {
      if (*this)
        m_mut->read_unlock(m_count);
      m_mut = nullptr;
    }

    explicit operator bool() const noexcept {
      return m_mut != nullptr;
    }

  private:
    read_write_mutex* m_mut;
    agt_u32_t         m_count;
  };

  class write_lock {

  public:

    write_lock() = default;
    write_lock(const write_lock&) = delete;
    write_lock(write_lock&& other) noexcept
        : m_mut(other.m_mut), m_readLock(other.m_readLock) {
      other.m_mut      = nullptr;
      other.m_readLock = nullptr;
    }

    explicit write_lock(read_write_mutex& mut) noexcept : m_readLock(nullptr) {
      mut.write_lock();
      m_mut = &mut;
    }
    explicit write_lock(read_write_mutex& mut, impl::try_once_t) noexcept
        : m_mut(mut.try_write_lock() ? &mut : nullptr), m_readLock(nullptr) { }
    explicit write_lock(read_write_mutex& mut, impl::try_for_t tryFor) noexcept
        : m_mut(mut.try_write_lock_for(tryFor.timeout) ? &mut : nullptr), m_readLock(nullptr) { }
    explicit write_lock(read_write_mutex& mut, impl::try_until_t tryUntil) noexcept
        : m_mut(mut.try_write_lock_until(tryUntil.dl) ? &mut : nullptr), m_readLock(nullptr) { }
    explicit write_lock(read_lock& lock) noexcept : m_mut(nullptr), m_readLock(&lock) {
      if (lock) {
        auto mut = lock.m_mut;
        mut->upgrade_lock(lock.m_count);
        m_mut = mut;
      }
    }
    explicit write_lock(read_lock& lock, impl::try_once_t) noexcept : m_mut(nullptr), m_readLock(&lock)  {
      if (lock && lock.m_mut->try_upgrade_lock(lock.m_count))
        m_mut = lock.m_mut;
    }
    explicit write_lock(read_lock& lock, impl::try_for_t tryFor) noexcept : m_mut(nullptr), m_readLock(&lock)  {
      if (lock && lock.m_mut->try_upgrade_lock_for(lock.m_count, tryFor.timeout))
        m_mut = lock.m_mut;
    }
    explicit write_lock(read_lock& lock, impl::try_until_t tryUntil) noexcept : m_mut(nullptr), m_readLock(&lock)  {
      if (lock && lock.m_mut->try_upgrade_lock_until(lock.m_count, tryUntil.dl))
        m_mut = lock.m_mut;
    }

    write_lock& operator=(const write_lock&) = delete;
    write_lock& operator=(write_lock&& lock) noexcept {
      release();
      new (this) write_lock(std::move(lock));
      return *this;
    }


    ~write_lock() {
      release();
    }




    void release() noexcept {
      if (*this) {
        if (m_readLock)
          m_mut->downgrade_lock(m_readLock->m_count);
        else
          m_mut->write_unlock();
        m_mut = nullptr;
      }
    }

    void unlock() noexcept {
      if (m_mut)
        m_mut->write_unlock();
      detach_read_lock();
      m_mut = nullptr;
    }

    void detach_read_lock() noexcept {
      if (m_readLock) {
        m_readLock->m_mut = nullptr;
        m_readLock = nullptr;
      }
    }

    explicit operator bool() const noexcept {
      return m_mut != nullptr;
    }

  private:
    read_write_mutex* m_mut;
    read_lock*        m_readLock;
  };

}


#include "atomic.cpp"

#endif//JEMSYS_AGATE2_ATOMIC_HPP
