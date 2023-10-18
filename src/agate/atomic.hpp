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

  void spin_sleep_until(deadline deadline) noexcept {
    agt_u32_t backoff = 0;
    while ( deadline.hasNotPassed() ) {
      DUFFS_MACHINE(backoff);
    }
  }

  void sleep(agt_u64_t ms) noexcept {
    if ( ms > 20 )
      Sleep((DWORD)ms);
    else
      spin_sleep_until(deadline::fromTimeout(AGT_TIMEOUT_MS(ms)));
  }
  void usleep(agt_u64_t us) noexcept {
    spin_sleep_until(deadline::fromTimeout(AGT_TIMEOUT_US(us)));
  }
  void nanosleep(agt_u64_t ns) noexcept {
    spin_sleep_until(deadline::fromTimeout(AGT_TIMEOUT_NS(ns)));
  }

  void        atomicStore(agt_u8_t& value,  agt_u8_t newValue) noexcept;
  void        atomicStore(agt_u16_t& value, agt_u16_t newValue) noexcept;
  void        atomicStore(agt_u32_t& value, agt_u32_t newValue) noexcept;
  void        atomicStore(agt_u64_t& value, agt_u64_t newValue) noexcept;
  template <typename T>
  inline void atomicStore(T*& value, std::type_identity_t<T>* newValue) noexcept {
    atomicStore(reinterpret_cast<agt_u64_t&>(value), reinterpret_cast<agt_u64_t>(newValue));
  }

  agt_u8_t  atomicLoad(const agt_u8_t& value) noexcept;
  agt_u16_t atomicLoad(const agt_u16_t& value) noexcept;
  agt_u32_t atomicLoad(const agt_u32_t& value) noexcept;
  agt_u64_t atomicLoad(const agt_u64_t& value) noexcept;
  template <typename T>
  inline T* atomicLoad(T* const & value) noexcept {
    return reinterpret_cast<T*>(atomicLoad(reinterpret_cast<const agt_u64_t&>(value)));
  }

  void       atomicRelaxedStore(agt_u8_t& value,  agt_u8_t newValue) noexcept;
  void       atomicRelaxedStore(agt_u16_t& value, agt_u16_t newValue) noexcept;
  void       atomicRelaxedStore(agt_u32_t& value, agt_u32_t newValue) noexcept;
  void       atomicRelaxedStore(agt_u64_t& value, agt_u64_t newValue) noexcept;
  template <typename T>
  inline void atomicRelaxedStore(T*& value, std::type_identity_t<T>* newValue) noexcept {
    atomicRelaxedStore(reinterpret_cast<agt_u64_t&>(value), reinterpret_cast<agt_u64_t>(newValue));
  }

  agt_u8_t  atomicRelaxedLoad(const agt_u8_t& value) noexcept;
  agt_u16_t atomicRelaxedLoad(const agt_u16_t& value) noexcept;
  agt_u32_t atomicRelaxedLoad(const agt_u32_t& value) noexcept;
  agt_u64_t atomicRelaxedLoad(const agt_u64_t& value) noexcept;
  template <typename T>
  inline T* atomicRelaxedLoad(T* const & value) noexcept {
    return reinterpret_cast<T*>(atomicRelaxedLoad(reinterpret_cast<const agt_u64_t&>(value)));
  }

  agt_u8_t  atomicExchange(agt_u8_t& value,  agt_u8_t newValue) noexcept;
  agt_u16_t atomicExchange(agt_u16_t& value, agt_u16_t newValue) noexcept;
  agt_u32_t atomicExchange(agt_u32_t& value, agt_u32_t newValue) noexcept;
  agt_u64_t atomicExchange(agt_u64_t& value, agt_u64_t newValue) noexcept;
  template <typename T>
  inline T* atomicExchange(T*& value, std::type_identity_t<T>* newValue) noexcept {
    return reinterpret_cast<T*>(atomicExchange(reinterpret_cast<agt_u64_t&>(value), reinterpret_cast<agt_u64_t>(newValue)));
  }

  bool        atomicCompareExchange(agt_u8_t& value,  agt_u8_t&  compare, agt_u8_t newValue) noexcept;
  bool        atomicCompareExchange(agt_u16_t& value, agt_u16_t& compare, agt_u16_t newValue) noexcept;
  bool        atomicCompareExchange(agt_u32_t& value, agt_u32_t& compare, agt_u32_t newValue) noexcept;
  bool        atomicCompareExchange(agt_u64_t& value, agt_u64_t& compare, agt_u64_t newValue) noexcept;
  bool        atomicCompareExchange16(void* value, void* compare, const void* newValue) noexcept;
  inline bool atomicCompareExchange12(void* value, void* compare, const void* newValue) noexcept {
    agt_u32_t tmp_compare_buffer[4];
    agt_u32_t tmp_new_val_buffer[4];

    std::memcpy(tmp_compare_buffer, compare, 12);
    std::memcpy(&tmp_compare_buffer[3], static_cast<agt_u32_t*>(value) + 3, sizeof(agt_u32_t));
    std::memcpy(tmp_new_val_buffer, newValue, 12);
    tmp_new_val_buffer[3] = tmp_compare_buffer[3];

    if (!atomicCompareExchange16(value, tmp_compare_buffer, tmp_new_val_buffer)) {
      std::memcpy(compare, tmp_compare_buffer, sizeof(agt_u32_t[3]));
      return false;
    }
    return true;
  }
  template <typename T>
  inline bool atomicCompareExchange(T*& value, T*& compare, std::type_identity_t<T>* newValue) noexcept {
    return atomicCompareExchange(reinterpret_cast<agt_u64_t&>(value), reinterpret_cast<agt_u64_t&>(compare), reinterpret_cast<agt_u64_t>(newValue));
  }

  bool        atomicCompareExchangeWeak(agt_u8_t& value,  agt_u8_t&  compare, agt_u8_t newValue) noexcept;
  bool        atomicCompareExchangeWeak(agt_u16_t& value, agt_u16_t& compare, agt_u16_t newValue) noexcept;
  bool        atomicCompareExchangeWeak(agt_u32_t& value, agt_u32_t& compare, agt_u32_t newValue) noexcept;
  bool        atomicCompareExchangeWeak(agt_u64_t& value, agt_u64_t& compare, agt_u64_t newValue) noexcept;
  bool        atomicCompareExchangeWeak16(void* value, void* compare, const void* newValue) noexcept;
  // value must have 4 bytes of padding at the end of it;
  inline bool atomicCompareExchangeWeak12(void* value, void* compare, const void* newValue) noexcept {

    agt_u32_t tmp_compare_buffer[4];
    agt_u32_t tmp_new_val_buffer[4];

    std::memcpy(tmp_compare_buffer, compare, 12);
    std::memcpy(&tmp_compare_buffer[3], static_cast<agt_u32_t*>(value) + 3, sizeof(agt_u32_t));
    std::memcpy(tmp_new_val_buffer, newValue, 12);
    tmp_new_val_buffer[3] = tmp_compare_buffer[3];

    if (!atomicCompareExchangeWeak16(value, tmp_compare_buffer, tmp_new_val_buffer)) {
      std::memcpy(compare, tmp_compare_buffer, sizeof(agt_u32_t[3]));
      return false;
    }
    return true;
  }
  template <typename T>
  inline bool atomicCompareExchangeWeak(T*& value, T*& compare, std::type_identity_t<T>* newValue) noexcept {
    return atomicCompareExchangeWeak(reinterpret_cast<agt_u64_t&>(value), reinterpret_cast<agt_u64_t&>(compare), reinterpret_cast<agt_u64_t>(newValue));
  }

  agt_u8_t  atomicIncrement(agt_u8_t& value) noexcept;
  agt_u16_t atomicIncrement(agt_u16_t& value) noexcept;
  agt_u32_t atomicIncrement(agt_u32_t& value) noexcept;
  agt_u64_t atomicIncrement(agt_u64_t& value) noexcept;
  agt_u8_t  atomicRelaxedIncrement(agt_u8_t& value) noexcept;
  agt_u16_t atomicRelaxedIncrement(agt_u16_t& value) noexcept;
  agt_u32_t atomicRelaxedIncrement(agt_u32_t& value) noexcept;
  agt_u64_t atomicRelaxedIncrement(agt_u64_t& value) noexcept;

  agt_u8_t  atomicDecrement(agt_u8_t& value) noexcept;
  agt_u16_t atomicDecrement(agt_u16_t& value) noexcept;
  agt_u32_t atomicDecrement(agt_u32_t& value) noexcept;
  agt_u64_t atomicDecrement(agt_u64_t& value) noexcept;

  agt_u8_t  atomicExchangeAdd(agt_u8_t&  value, agt_u8_t  newValue) noexcept;
  agt_u16_t atomicExchangeAdd(agt_u16_t& value, agt_u16_t newValue) noexcept;
  agt_u32_t atomicExchangeAdd(agt_u32_t& value, agt_u32_t newValue) noexcept;
  agt_u64_t atomicExchangeAdd(agt_u64_t& value, agt_u64_t newValue) noexcept;
  agt_u8_t  atomicExchangeAnd(agt_u8_t&  value, agt_u8_t  newValue) noexcept;
  agt_u16_t atomicExchangeAnd(agt_u16_t& value, agt_u16_t newValue) noexcept;
  agt_u32_t atomicExchangeAnd(agt_u32_t& value, agt_u32_t newValue) noexcept;
  agt_u64_t atomicExchangeAnd(agt_u64_t& value, agt_u64_t newValue) noexcept;

  agt_u8_t  atomicExchangeOr(agt_u8_t&  value, agt_u8_t newValue) noexcept;
  agt_u16_t atomicExchangeOr(agt_u16_t& value, agt_u16_t newValue) noexcept;
  agt_u32_t atomicExchangeOr(agt_u32_t& value, agt_u32_t newValue) noexcept;
  agt_u64_t atomicExchangeOr(agt_u64_t& value, agt_u64_t newValue) noexcept;

  agt_u8_t  atomicExchangeXor(agt_u8_t&  value, agt_u8_t newValue) noexcept;
  agt_u16_t atomicExchangeXor(agt_u16_t& value, agt_u16_t newValue) noexcept;
  agt_u32_t atomicExchangeXor(agt_u32_t& value, agt_u32_t newValue) noexcept;
  agt_u64_t atomicExchangeXor(agt_u64_t& value, agt_u64_t newValue) noexcept;

  void      atomicNotifyOne(void* value) noexcept;
  void      atomicNotifyAll(void* value) noexcept;
  void      atomicNotifyOneLocal(void* value) noexcept;
  void      atomicNotifyAllLocal(void* value) noexcept;
  void      atomicNotifyOneShared(void* value) noexcept;
  void      atomicNotifyAllShared(void* value) noexcept;

  template <typename T> requires (std::integral<T>)
  inline void      atomicNotifyOne(T& value) noexcept {
    atomicNotifyOne(std::addressof(value));
  }
  template <typename T> requires (std::integral<T>)
  inline void      atomicNotifyOneLocal(T& value) noexcept {
    atomicNotifyOneLocal(std::addressof(value));
  }
  template <typename T> requires (std::integral<T>)
  inline void      atomicNotifyOneShared(T& value) noexcept {
    atomicNotifyOneShared(std::addressof(value));
  }
  template <typename T> requires (std::integral<T>)
  inline void      atomicNotifyAll(T& value) noexcept {
    atomicNotifyAll(std::addressof(value));
  }
  template <typename T> requires (std::integral<T>)
  inline void      atomicNotifyAllLocal(T& value) noexcept {
    atomicNotifyAllLocal(std::addressof(value));
  }
  template <typename T> requires (std::integral<T>)
  inline void      atomicNotifyAllShared(T& value) noexcept {
    atomicNotifyAllShared(std::addressof(value));
  }


  // Deep Wait

  void      atomicDeepWaitRaw(const void* atomicAddress, void* compareAddress, size_t addressSize) noexcept;
  bool      atomicDeepWaitRaw(const void* atomicAddress, void* compareAddress, size_t addressSize, agt_u32_t timeout) noexcept;

  template <typename T>
  inline void atomicDeepWait(const T& value, std::type_identity_t<T> waitValue) noexcept {
    while ( atomicLoad(value) == waitValue ) {
      atomicDeepWaitRaw(std::addressof(value), std::addressof(waitValue), sizeof(T));
    }
  }

  template <typename T>
  inline void atomicDeepWait(const T& value, T& capturedValue, std::type_identity_t<T> waitValue) noexcept {

    T tmpValue;

    while ( (tmpValue = atomicLoad(value)) == waitValue ) {
      atomicDeepWaitRaw(std::addressof(value), std::addressof(waitValue), sizeof(T));
    }

    capturedValue = tmpValue;
  }

  template <typename T, std::predicate<const T&> Fn>
  inline void atomicDeepWait(const T& value, Fn&& functor) noexcept {
    T tmpValue;

    while ( !functor(tmpValue = atomicLoad(value)) ) {
      atomicDeepWaitRaw(std::addressof(value), std::addressof(tmpValue), sizeof(T));
    }
  }

  template <typename T, std::predicate<const T&> Fn>
  inline void atomicDeepWait(const T& value, T& capturedValue, Fn&& functor) noexcept {

    T tmpValue;

    while ( !functor(tmpValue = atomicLoad(value)) ) {
      atomicDeepWaitRaw(std::addressof(value), std::addressof(tmpValue), sizeof(T));
    }

    capturedValue = tmpValue;
  }

  template <typename T>
  inline bool atomicDeepWaitUntil(const T& value, deadline deadline, std::type_identity_t<T> waitValue) noexcept {
    while ( atomicLoad(value) == waitValue ) {
      if (deadline.hasPassed())
        return false;
      atomicDeepWaitRaw(std::addressof(value), std::addressof(waitValue), sizeof(T));
    }
    return true;
  }

  template <typename T>
  inline bool atomicDeepWaitUntil(const T& value, T& capturedValue, deadline deadline, std::type_identity_t<T> waitValue) noexcept {

    T tmpValue;

    while ( (tmpValue = atomicLoad(value)) == waitValue ) {
      if (deadline.hasPassed())
        return false;
      atomicDeepWaitRaw(std::addressof(value), std::addressof(waitValue), sizeof(T));
    }

    capturedValue = tmpValue;
    return true;
  }

  template <typename T, std::predicate<const T&> Fn>
  inline bool atomicDeepWaitUntil(const T& value, deadline deadline, Fn&& functor) noexcept {

    T capturedValue;

    while ( !functor((capturedValue = atomicLoad(value))) ) {
      if (deadline.hasPassed())
        return false;
      atomicDeepWaitRaw(std::addressof(value), std::addressof(capturedValue), sizeof(T));
    }
    return true;
  }

  template <typename T, std::predicate<const T&> Fn>
  inline bool atomicDeepWaitUntil(const T& value, T& capturedValue, deadline deadline, Fn&& functor) noexcept {

    T tmpValue;

    while ( !functor(tmpValue = atomicLoad(value)) ) {
      if (deadline.hasPassed())
        return false;
      atomicDeepWaitRaw(std::addressof(value), std::addressof(tmpValue), sizeof(T));
    }

    capturedValue = tmpValue;
    return true;
  }



  // Wait

  template <typename T>
  inline void atomicWait(const T& value, std::type_identity_t<T> waitValue) noexcept {
    agt_u32_t backoff = 0;

    while ( atomicLoad(value) == waitValue ) {
      DUFFS_MACHINE_EX(backoff, atomicDeepWaitRaw(std::addressof(value), std::addressof(waitValue), sizeof(T)); );
    }
  }

  template <typename T>
  inline void atomicWait(const T& value, T& capturedValue, std::type_identity_t<T> waitValue) noexcept {
    T tmpValue;
    agt_u32_t backoff = 0;

    while ( (tmpValue = atomicLoad(value)) == waitValue ) {
      DUFFS_MACHINE_EX(backoff,
                       atomicDeepWaitRaw(std::addressof(value), std::addressof(waitValue), sizeof(T)); );
    }

    capturedValue = tmpValue;
  }

  template <typename T, std::predicate<const T&> Fn>
  inline void atomicWait(const T& value, Fn&& functor) noexcept {
    T capturedValue;
    agt_u32_t backoff = 0;

    while ( !functor((capturedValue = atomicLoad(value))) ) {
      DUFFS_MACHINE_EX(backoff, atomicDeepWaitRaw(std::addressof(value), std::addressof(capturedValue), sizeof(T)); );
    }
  }

  template <typename T, std::predicate<const T&> Fn>
  inline void atomicWait(const T& value, T& capturedValue, Fn&& functor) noexcept {
    T tmpValue;
    agt_u32_t backoff = 0;

    while ( !functor((tmpValue = atomicLoad(value))) ) {
      DUFFS_MACHINE_EX(backoff, atomicDeepWaitRaw(std::addressof(value), std::addressof(tmpValue), sizeof(T)); );
    }

    capturedValue = tmpValue;
  }

  template <typename T>
  inline bool atomicWaitUntil(const T& value, deadline deadline, std::type_identity_t<T> waitValue) noexcept {
    agt_u32_t backoff = 0;

    while ( atomicLoad(value) == waitValue ) {
      if ( deadline.hasPassed() )
        return false;
      DUFFS_MACHINE_EX(backoff, if (!atomicDeepWaitRaw(std::addressof(value), std::addressof(waitValue), sizeof(T), deadline.toTimeoutMs())) {
            return false;
          } );
    }

    return true;
  }

  template <typename T>
  inline bool atomicWaitUntil(const T& value, T& capturedValue, deadline deadline, std::type_identity_t<T> waitValue) noexcept {
    T tmpValue;
    agt_u32_t backoff = 0;

    while ( (tmpValue = atomicLoad(value)) == waitValue ) {
      if ( deadline.hasPassed() )
        return false;
      DUFFS_MACHINE_EX(backoff, if (!atomicDeepWaitRaw(std::addressof(value), std::addressof(waitValue), sizeof(T), deadline.toTimeoutMs())) {
            return false;
          } );
    }

    capturedValue = tmpValue;
    return true;
  }

  template <typename T, std::predicate<const T&> Fn>
  inline bool atomicWaitUntil(const T& value, deadline deadline, Fn&& functor) noexcept {
    T capturedValue;
    agt_u32_t backoff = 0;

    while ( !functor((capturedValue = atomicLoad(value))) ) {
      if ( deadline.hasPassed() )
        return false;
      DUFFS_MACHINE_EX(backoff, if (!atomicDeepWaitRaw(std::addressof(value), std::addressof(capturedValue), sizeof(T), deadline.toTimeoutMs())) {
            return false;
          } );
    }

    return true;
  }

  template <typename T, std::predicate<const T&> Fn>
  inline bool atomicWaitUntil(const T& value, T& capturedValue, deadline deadline, Fn&& functor) noexcept {
    T tmpValue;
    agt_u32_t backoff = 0;

    while ( !functor((tmpValue = atomicLoad(value))) ) {
      if ( deadline.hasPassed() )
        return false;
      DUFFS_MACHINE_EX(backoff, if (!atomicDeepWaitRaw(std::addressof(value), std::addressof(tmpValue), sizeof(T), deadline.toTimeoutMs())) {
            return false;
          } );
    }
    capturedValue = tmpValue;
    return true;
  }

  template <typename T>
  inline bool atomicWaitFor(const T& value, agt_timeout_t timeout, std::type_identity_t<T> waitValue) noexcept {
    switch (timeout) {
      case AGT_WAIT:
        atomicWait(value, waitValue);
        return true;
      case AGT_DO_NOT_WAIT:
        return atomicLoad(value) != waitValue;
      default:
        return atomicWaitUntil(value, deadline::fromTimeout(timeout), waitValue);
    }
  }

  template <typename T>
  inline bool atomicWaitFor(const T& value, T& capturedValue, agt_timeout_t timeout, std::type_identity_t<T> waitValue) noexcept {
    switch (timeout) {
      case AGT_WAIT:
        atomicWait(value, capturedValue, waitValue);
        return true;
      case AGT_DO_NOT_WAIT: {
        T tmpValue;
        if ((tmpValue = atomicLoad(value)) != waitValue) {
          capturedValue = tmpValue;
          return true;
        }
        return false;
      }
      default:
        return atomicWaitUntil(value, capturedValue, deadline::fromTimeout(timeout), waitValue);
    }
  }

  template <typename T, std::predicate<const T&> Fn>
  inline bool atomicWaitFor(const T& value, agt_timeout_t timeout, Fn&& functor) noexcept {
    switch (timeout) {
      case AGT_WAIT:
        atomicWait(value, std::forward<Fn>(functor));
        return true;
      case AGT_DO_NOT_WAIT:
        return functor(atomicLoad(value));
      default:
        return atomicWaitUntil(value, deadline::fromTimeout(timeout), std::forward<Fn>(functor));
    }
  }

  template <typename T, std::predicate<const T&> Fn>
  inline bool atomicWaitFor(const T& value, T& capturedValue, agt_timeout_t timeout, Fn&& functor) noexcept {
    switch (timeout) {
      case AGT_WAIT:
        atomicWait(value, capturedValue, std::forward<Fn>(functor));
        return true;
      case AGT_DO_NOT_WAIT:
        return functor((capturedValue = atomicLoad(value)));
      default:
        return atomicWaitUntil(value, capturedValue, deadline::fromTimeout(timeout), std::forward<Fn>(functor));
    }
  }


  
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
        return static_cast<bool>(atomicLoad(bits) & flags);
      }
      AGT_nodiscard AGT_forceinline bool testAll(IntType flags) const noexcept {
        return (atomicLoad(bits) & flags) == flags;
      }
      AGT_nodiscard AGT_forceinline bool testAny() const noexcept {
        return static_cast<bool>(atomicLoad(bits));
      }

      AGT_nodiscard AGT_forceinline bool testAndSet(IntType flags) noexcept {
        return testAnyAndSet(flags);
      }
      AGT_nodiscard AGT_forceinline bool testAnyAndSet(IntType flags) noexcept {
        return static_cast<bool>(atomicExchangeOr(bits, flags) & flags);
      }
      AGT_nodiscard AGT_forceinline bool testAllAndSet(IntType flags) noexcept {
        return (atomicExchangeOr(bits, flags) & flags) == flags;
      }

      AGT_nodiscard AGT_forceinline bool testAndReset(IntType flags) noexcept {
        return testAnyAndReset(flags);
      }
      AGT_nodiscard AGT_forceinline bool testAnyAndReset(IntType flags) noexcept {
        return static_cast<bool>(atomicExchangeAnd(bits, ~flags) & flags);
      }
      AGT_nodiscard AGT_forceinline bool testAllAndReset(IntType flags) noexcept {
        return (atomicExchangeAnd(bits, ~flags) & flags) == flags;
      }

      AGT_nodiscard AGT_forceinline bool testAndFlip(IntType flags) noexcept {
        return testAnyAndFlip(flags);
      }
      AGT_nodiscard AGT_forceinline bool testAnyAndFlip(IntType flags) noexcept {
        return static_cast<bool>(atomicExchangeXor(bits, flags) & flags);
      }
      AGT_nodiscard AGT_forceinline bool testAllAndFlip(IntType flags) noexcept {
        return (atomicExchangeXor(bits, flags) & flags) == flags;
      }

      AGT_nodiscard AGT_forceinline IntType fetch() const noexcept {
        return atomicLoad(bits);
      }
      AGT_nodiscard AGT_forceinline IntType fetch(IntType flags) const noexcept {
        return atomicLoad(bits) & flags;
      }

      AGT_nodiscard AGT_forceinline IntType fetchAndSet(IntType flags) noexcept {
        return atomicExchangeOr(bits, flags);
      }
      AGT_nodiscard AGT_forceinline IntType fetchAndReset(IntType flags) noexcept {
        return atomicExchangeAnd(bits, ~flags);
      }
      AGT_nodiscard AGT_forceinline IntType fetchAndFlip(IntType flags) noexcept {
        return atomicExchangeXor(bits, flags);
      }

      AGT_nodiscard AGT_forceinline IntType fetchAndClear() noexcept {
        return atomicExchange(bits, static_cast<IntType>(0));
      }

      AGT_forceinline void set(IntType flags) noexcept {
        atomicExchangeOr(bits, flags);
      }
      AGT_forceinline void reset(IntType flags) noexcept {
        atomicExchangeAnd(bits, ~flags);
      }
      AGT_forceinline void flip(IntType flags) noexcept {
        atomicExchangeXor(bits, flags);
      }

      AGT_forceinline void clear() noexcept {
        reset();
      }
      AGT_forceinline void clearAndSet(IntType flags) noexcept {
        atomicStore(bits, flags);
      }

      AGT_forceinline void reset() noexcept {
        atomicStore(bits, static_cast<IntType>(0));
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


  class local_binary_semaphore {
    agt_u32_t val_;
  public:

    explicit local_binary_semaphore(agt_u32_t val) : val_(val) { }

    void acquire() noexcept;
    bool tryAcquire() noexcept;
    bool tryAcquireFor(agt_timeout_t timeout) noexcept;
    bool tryAcquireUntil(deadline deadline) noexcept;
    void release() noexcept {
      atomicRelaxedStore(val_, 1);
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
      auto current = atomicRelaxedLoad(val_);
      for (;;) {
        while ( current == 0 ) {
          atomicWait(val_, 0);
          current = atomicRelaxedLoad(val_);
        }

        if ( atomicCompareExchangeWeak(val_, current, current - 1) )
          return;
      }
    }
    bool tryAcquire() noexcept {
      if ( auto current = atomicRelaxedLoad(val_) )
        return atomicCompareExchange(val_, current, current - 1);
      return false;
    }
    bool try_acquire_for(agt_timeout_t timeout) noexcept {
      return try_acquire_until(deadline::fromTimeout(timeout));
    }
    bool try_acquire_until(deadline deadline) noexcept {
      auto current  = atomicRelaxedLoad(val_);
      for (;;) {
        while ( current == 0 ) {
          if ( !priv_wait_until(deadline) )
            return false;
          current = atomicRelaxedLoad(val_);
        }

        if ( atomicCompareExchangeWeak(val_, current, current - 1) )
          return true;
      }
    }
    void release() noexcept {
      atomicIncrement(val_);
      atomicNotifyOne(val_);
    }

  private:

    void priv_wait() const noexcept {

    }
    bool priv_wait_until(deadline deadline) const noexcept {
      if (auto current = atomicLoad(val_))
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
  };


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

  class spinlock {

    inline constexpr static agt_u32_t UnlockedValue = 0;
    inline constexpr static agt_u32_t LockedValue   = static_cast<agt_u32_t>(-1);

  public:

    spinlock() = default;


    void lock() noexcept {
      agt_u32_t oldValue;
      do {
        oldValue = UnlockedValue;
      } while(!atomicCompareExchangeWeak(m_lock, oldValue, LockedValue));
    }

    void unlock() noexcept {
      atomicStore(m_lock, UnlockedValue);
    }

  private:
    agt_u32_t m_lock = UnlockedValue;
  };

  class futex {

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
      oldLock.bits  = atomicRelaxedLoad(m_lock.bits);
      newLock.state = ACQUIRED_BY_TAKE;

      do {
        if (oldLock.state != NOT_ACQUIRED)
          return _wait_to_take(oldLock);

        newLock.epoch       = oldLock.epoch;
        newLock.waiterEpoch = oldLock.waiterEpoch;
        newLock.waiters     = oldLock.waiters;

      } while(!atomicCompareExchangeWeak(m_lock.bits, oldLock.bits, newLock.bits));

      return _key_cast(newLock.epoch);
    }

    [[nodiscard]] std::optional<futex_key_t> try_take() noexcept {

      lock_type oldLock, newLock;
      oldLock.bits  = atomicRelaxedLoad(m_lock.bits);
      newLock.state = ACQUIRED_BY_TAKE;

      do {
        if (oldLock.state != NOT_ACQUIRED)
          return std::nullopt;
        newLock.epoch       = oldLock.epoch;
        newLock.waiterEpoch = oldLock.waiterEpoch;
        newLock.waiters     = oldLock.waiters;

      } while(!atomicCompareExchangeWeak(m_lock.bits, oldLock.bits, newLock.bits));

      return _key_cast(newLock.epoch);
    }

    void drop(futex_key_t key) noexcept {
      lock_type oldLock, newLock;
      oldLock.bits = atomicRelaxedLoad(m_lock.bits);

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
      } while(!atomicCompareExchangeWeak(m_lock.bits, oldLock.bits, newLock.bits));
    }

    //
    [[nodiscard]] futex_key_t wait(futex_key_t key) noexcept {
      lock_type oldLock, newLock;
      oldLock.bits = atomicRelaxedLoad(m_lock.bits);
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

        if (atomicCompareExchangeWeak(m_lock.bits, oldLock.bits, newLock.bits)) {
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
  };



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
        *static_cast<storage_type*>(dst) = atomicLoad(m_bits);
      }
      AGT_forceinline void _relaxed_load(void* dst) const noexcept {
        *static_cast<storage_type*>(dst) = atomicRelaxedLoad(m_bits);
      }

      AGT_forceinline void _store(const void* src) noexcept {
        atomicStore(m_bits, *static_cast<const storage_type*>(src));
      }
      AGT_forceinline void _relaxed_store(const void* src) noexcept {
        atomicRelaxedStore(m_bits, *static_cast<const storage_type*>(src));
      }

      AGT_forceinline void _exchange(void* dst, const void* src) noexcept {
        *static_cast<storage_type*>(dst) = atomicExchange(m_bits, *static_cast<const storage_type*>(src));
      }

      AGT_forceinline void _increment(void* dst) noexcept {
        *static_cast<storage_type*>(dst) = atomicIncrement(m_bits);
      }
      AGT_forceinline void _relaxed_increment(void* dst) noexcept {
        *static_cast<storage_type*>(dst) = atomicRelaxedIncrement(m_bits);
      }
      AGT_forceinline void _decrement(void* dst) noexcept {
        *static_cast<storage_type*>(dst) = atomicDecrement(m_bits);
      }

      AGT_forceinline void _exchange_add(void* dst, const void* src) noexcept {
        *static_cast<storage_type*>(dst) = atomicExchangeAdd(m_bits, *static_cast<const storage_type*>(src));
      }
      AGT_forceinline void _exchange_and(void* dst, const void* src) noexcept {
        *static_cast<storage_type*>(dst) = atomicExchangeAnd(m_bits, *static_cast<const storage_type*>(src));
      }
      AGT_forceinline void _exchange_or(void* dst, const void* src) noexcept {
        *static_cast<storage_type*>(dst) = atomicExchangeOr(m_bits, *static_cast<const storage_type*>(src));
      }
      AGT_forceinline void _exchange_xor(void* dst, const void* src) noexcept {
        *static_cast<storage_type*>(dst) = atomicExchangeXor(m_bits, *static_cast<const storage_type*>(src));
      }

      AGT_forceinline bool _compare_exchange(void* compare, const void* value) noexcept {
        return atomicCompareExchange(m_bits, *static_cast<storage_type*>(compare), *static_cast<const storage_type*>(value));
      }
      AGT_forceinline bool _compare_exchange_weak(void* compare, const void* value) noexcept {
        return atomicCompareExchangeWeak(m_bits, *static_cast<storage_type*>(compare), *static_cast<const storage_type*>(value));
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
        return atomicCompareExchange16(this, compare, value);
      }

      AGT_forceinline bool _aligned_compare_exchange_weak(void* compare, const void* value) noexcept {
        return atomicCompareExchangeWeak16(this, compare, value);
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
      return static_cast<bool>(atomicLoad(this->bits) & toInt(flags));
    }
    AGT_nodiscard AGT_forceinline bool testAll(EnumType flags) const noexcept {
      return (atomicLoad(this->bits) & toInt(flags)) == toInt(flags);
    }
    AGT_nodiscard AGT_forceinline bool testAny() const noexcept {
      return static_cast<bool>(atomicLoad(this->bits));
    }

    AGT_nodiscard AGT_forceinline bool testAndSet(EnumType flags) noexcept {
      return testAnyAndSet(flags);
    }
    AGT_nodiscard AGT_forceinline bool testAnyAndSet(EnumType flags) noexcept {
      return this->testAnyAndSet(toInt(flags));
      // return static_cast<bool>(atomicExchangeOr(this->bits, flags) & flags);
    }
    AGT_nodiscard AGT_forceinline bool testAllAndSet(EnumType flags) noexcept {
      return this->testAllAndSet(toInt(flags));
      // return (atomicExchangeOr(bits, flags) & flags) == flags;
    }

    AGT_nodiscard AGT_forceinline bool testAndReset(EnumType flags) noexcept {
      return testAnyAndReset(flags);
    }
    AGT_nodiscard AGT_forceinline bool testAnyAndReset(EnumType flags) noexcept {
      return this->testAnyAndReset(toInt(flags));
      // return static_cast<bool>(atomicExchangeAnd(bits, ~flags) & flags);
    }
    AGT_nodiscard AGT_forceinline bool testAllAndReset(EnumType flags) noexcept {
      return this->testAllAndReset(toInt(flags));
      // return (atomicExchangeAnd(bits, ~flags) & flags) == flags;
    }

    AGT_nodiscard AGT_forceinline bool testAndFlip(EnumType flags) noexcept {
      return testAnyAndFlip(flags);
    }
    AGT_nodiscard AGT_forceinline bool testAnyAndFlip(EnumType flags) noexcept {
      return this->testAnyAndFlip(toInt(flags));
      // return static_cast<bool>(atomicExchangeXor(bits, flags) & flags);
    }
    AGT_nodiscard AGT_forceinline bool testAllAndFlip(EnumType flags) noexcept {
      return this->testAllAndFlip(toInt(flags));
      // return (atomicExchangeXor(bits, flags) & flags) == flags;
    }

    AGT_nodiscard AGT_forceinline EnumType fetch() const noexcept {
      return toEnum(atomicLoad(this->bits));
    }
    AGT_nodiscard AGT_forceinline EnumType fetch(EnumType flags) const noexcept {
      return toEnum(atomicLoad(this->bits) & toInt(flags));
    }

    AGT_nodiscard AGT_forceinline EnumType fetchAndSet(EnumType flags) noexcept {
      return toEnum(this->fetchAndSet(toInt(flags)));
      // return atomicExchangeOr(bits, flags);
    }
    AGT_nodiscard AGT_forceinline EnumType fetchAndReset(EnumType flags) noexcept {
      return toEnum(this->fetchAndReset(toInt(flags)));
    }
    AGT_nodiscard AGT_forceinline EnumType fetchAndFlip(EnumType flags) noexcept {
      return toEnum(this->fetchAndFlip(toInt(flags)));
    }

    AGT_nodiscard AGT_forceinline EnumType fetchAndClear() noexcept {
      return toEnum(atomicExchange(this->bits, static_cast<IntType>(0)));
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
      atomicStore(this->bits, toInt(flags));
    }

    AGT_forceinline void reset() noexcept {
      atomicStore(this->bits, static_cast<IntType>(0));
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
      return atomicRelaxedLoad(value_);
    }


    agt_u32_t acquire() noexcept {
      return atomicIncrement(value_);
    }
    agt_u32_t acquire(agt_u32_t n) noexcept {
      return atomicExchangeAdd(value_, n) + n;
    }

    agt_u32_t release() noexcept {
      return atomicDecrement(value_);
    }
    agt_u32_t release(agt_u32_t n) noexcept {
      return atomicExchangeAdd(value_, 0 - n) - n;
    }

    agt_u32_t operator++()    noexcept {
      return this->acquire();
    }
    agt_u32_t operator++(int) noexcept {
      return atomicExchangeAdd(value_, 1);
    }

    agt_u32_t operator--()    noexcept {
      return this->release();
    }
    agt_u32_t operator--(int) noexcept {
      return atomicExchangeAdd(value_, -1);
    }

  private:

    agt_u32_t value_ = 0;
  };

  class atomic_monotonic_counter {
  public:
    atomic_monotonic_counter() = default;


    agt_u32_t getValue() const noexcept {
      return atomicRelaxedLoad(value_);
      // return __iso_volatile_load32(&reinterpret_cast<const int&>(value_));
    }

    void reset() noexcept {
      atomicStore(value_, 0);
    }



    agt_u32_t operator++()    noexcept {
      agt_u32_t result = atomicIncrement(value_);
      notifyWaiters();
      return result;
    }
    agt_u32_t operator++(int) noexcept {
      agt_u32_t result = _InterlockedExchangeAdd(&value_, 1) + 1;
      notifyWaiters();
      return result;
    }


    bool      waitFor(agt_u32_t expectedValue, agt_timeout_t timeout) const noexcept {
      switch (timeout) {
        case AGT_WAIT:
          deepWait(expectedValue);
          return true;
        case AGT_DO_NOT_WAIT:
          return isAtLeast(expectedValue);
        default:
          return deepWaitUntil(expectedValue, deadline::fromTimeout(timeout));
      }
    }


  private:

    void notifyWaiters() noexcept {
      const agt_u32_t waiters = atomicRelaxedLoad(deepSleepers_);

      if ( waiters == 0 ) [[likely]] {

      } else if ( waiters == 1 ) {
        atomicNotifyOne(value_);
      } else {
        atomicNotifyAll(value_);
      }
    }

    AGT_forceinline agt_u32_t orderedLoad() const noexcept {
      return atomicLoad(value_);
    }

    AGT_forceinline bool isAtLeast(agt_u32_t value) const noexcept {
      return orderedLoad() >= value;
    }

    AGT_forceinline bool      tryWaitOnce(agt_u32_t expectedValue) const noexcept {
      return orderedLoad() >= expectedValue;
    }

    AGT_noinline    void      deepWait(agt_u32_t expectedValue) const noexcept {
      agt_u32_t capturedValue;
      agt_u32_t backoff = 0;
      while ( (capturedValue = orderedLoad()) < expectedValue ) {
        DUFFS_MACHINE_EX(backoff,
                         atomicIncrement(deepSleepers_);
                         atomicDeepWaitRaw(&value_, &capturedValue, sizeof(agt_u32_t));
                         atomicDecrement(deepSleepers_);
                         );
      }
    }

    AGT_noinline    bool      deepWaitUntil(agt_u32_t expectedValue, deadline deadline) const noexcept {
      agt_u32_t capturedValue;
      agt_u32_t backoff = 0;

      while ( (capturedValue = atomicLoad(value_)) < expectedValue ) {
        if ( deadline.hasPassed() )
          return false;
        DUFFS_MACHINE_EX(backoff,
          atomicIncrement(deepSleepers_);
          bool result = atomicDeepWaitRaw(&value_, &capturedValue, sizeof(agt_u32_t), deadline.toTimeoutMs());
          atomicDecrement(deepSleepers_);
          if (!result) return false;
          );
      }

      return true;
    }


    agt_u32_t value_ = 0;
    mutable agt_u32_t deepSleepers_ = 0;
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
        agt_u32_t
      };
      agt_u64_t bits;
    };

  public:

    void read_lock(agt_u32_t count) noexcept {
      /*first_half tmp{
          .bits = atomicRelaxedLoad(m_first.bits)
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



#endif//JEMSYS_AGATE2_ATOMIC_HPP
