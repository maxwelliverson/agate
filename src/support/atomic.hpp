//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_ATOMIC_HPP
#define JEMSYS_AGATE2_ATOMIC_HPP

#include "agate2.h"

#include "support/atomicutils.hpp"



#define AGT_LONG_TIMEOUT_THRESHOLD 20000

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



    AGT_forceinline static deadline fromTimeout(agt_timeout_t timeoutUs) noexcept {
      return { getCurrentTimestamp() + (timeoutUs * (Microseconds / NativePeriod)) };
    }

    AGT_forceinline agt_u32_t toTimeoutMs() const noexcept {
      return getTimeoutNative() / (Milliseconds / NativePeriod);
    }

    AGT_forceinline agt_timeout_t toTimeoutUs() const noexcept {
      return getTimeoutNative() / (Microseconds / NativePeriod);
    }

    AGT_forceinline bool hasPassed() const noexcept {
      return getCurrentTimestamp() >= timestamp;
    }
    AGT_forceinline bool hasNotPassed() const noexcept {
      return getCurrentTimestamp() < timestamp;
    }

    AGT_forceinline bool isLong() const noexcept {
      return getTimeoutNative() >= (AGT_LONG_TIMEOUT_THRESHOLD * (Microseconds / NativePeriod));
    }

  private:

    AGT_forceinline agt_u64_t getTimeoutNative() const noexcept {
      return timestamp - getCurrentTimestamp();
    }

    static agt_u64_t getCurrentTimestamp() noexcept;

    agt_u64_t timestamp;
  };
  
  namespace impl {

    void      atomicStore(agt_u8_t& value,  agt_u8_t newValue) noexcept;
    void      atomicStore(agt_u16_t& value, agt_u16_t newValue) noexcept;
    void      atomicStore(agt_u32_t& value, agt_u32_t newValue) noexcept;
    void      atomicStore(agt_u64_t& value, agt_u64_t newValue) noexcept;
    agt_u8_t  atomicLoad(const agt_u8_t& value) noexcept;
    agt_u16_t atomicLoad(const agt_u16_t& value) noexcept;
    agt_u32_t atomicLoad(const agt_u32_t& value) noexcept;
    agt_u64_t atomicLoad(const agt_u64_t& value) noexcept;

    void      atomicRelaxedStore(agt_u8_t& value,  agt_u8_t newValue) noexcept;
    void      atomicRelaxedStore(agt_u16_t& value, agt_u16_t newValue) noexcept;
    void      atomicRelaxedStore(agt_u32_t& value, agt_u32_t newValue) noexcept;
    void      atomicRelaxedStore(agt_u64_t& value, agt_u64_t newValue) noexcept;
    agt_u8_t  atomicRelaxedLoad(const agt_u8_t& value) noexcept;
    agt_u16_t atomicRelaxedLoad(const agt_u16_t& value) noexcept;
    agt_u32_t atomicRelaxedLoad(const agt_u32_t& value) noexcept;
    agt_u64_t atomicRelaxedLoad(const agt_u64_t& value) noexcept;

    agt_u8_t  atomicExchange(agt_u8_t& value,  agt_u8_t newValue) noexcept;
    agt_u16_t atomicExchange(agt_u16_t& value, agt_u16_t newValue) noexcept;
    agt_u32_t atomicExchange(agt_u32_t& value, agt_u32_t newValue) noexcept;
    agt_u64_t atomicExchange(agt_u64_t& value, agt_u64_t newValue) noexcept;

    bool      atomicCompareExchange(agt_u8_t& value,  agt_u8_t&  compare, agt_u8_t newValue) noexcept;
    bool      atomicCompareExchange(agt_u16_t& value, agt_u16_t& compare, agt_u16_t newValue) noexcept;
    bool      atomicCompareExchange(agt_u32_t& value, agt_u32_t& compare, agt_u32_t newValue) noexcept;
    bool      atomicCompareExchange(agt_u64_t& value, agt_u64_t& compare, agt_u64_t newValue) noexcept;
    bool      atomicCompareExchangeWeak(agt_u8_t& value,  agt_u8_t&  compare, agt_u8_t newValue) noexcept;
    bool      atomicCompareExchangeWeak(agt_u16_t& value, agt_u16_t& compare, agt_u16_t newValue) noexcept;
    bool      atomicCompareExchangeWeak(agt_u32_t& value, agt_u32_t& compare, agt_u32_t newValue) noexcept;
    bool      atomicCompareExchangeWeak(agt_u64_t& value, agt_u64_t& compare, agt_u64_t newValue) noexcept;

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
        DUFFS_MACHINE_EX(backoff, atomicDeepWaitRaw(std::addressof(value), std::addressof(waitValue), sizeof(T)); );
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
          return atomicLoad(value) != waitValue;
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
      impl::atomicRelaxedStore(val_, 1);
      impl::atomicNotifyOne(val_);
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
      auto current = impl::atomicRelaxedLoad(val_);
      for (;;) {
        while ( current == 0 ) {
          impl::atomicWait(val_, 0);
          current = impl::atomicRelaxedLoad(val_);
        }

        if ( impl::atomicCompareExchangeWeak(val_, current, current - 1) )
          return;
      }
    }
    bool tryAcquire() noexcept {
      if ( auto current = impl::atomicRelaxedLoad(val_) )
        return impl::atomicCompareExchange(val_, current, current - 1);
      return false;
    }
    bool try_acquire_for(agt_timeout_t timeout) noexcept {
      return try_acquire_until(deadline::fromTimeout(timeout));
    }
    bool try_acquire_until(deadline deadline) noexcept {
      auto current  = impl::atomicRelaxedLoad(val_);
      for (;;) {
        while ( current == 0 ) {
          if ( !priv_wait_until(deadline) )
            return false;
          current = impl::atomicRelaxedLoad(val_);
        }

        if ( impl::atomicCompareExchangeWeak(val_, current, current - 1) )
          return true;
      }
    }
    void release() noexcept {
      impl::atomicIncrement(val_);
      impl::atomicNotifyOne(val_);
    }

  private:

    void priv_wait() const noexcept {

    }
    bool priv_wait_until(deadline deadline) const noexcept {
      if (auto current = impl::atomicLoad(val_))
        return true;
      return impl::atomicWaitUntil(val_, deadline, 0);
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


  class semaphore {
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
      /*agt_ptrdiff_t current = value.load();
      if ( current == 0 )
        WaitOnAddress(&value, &current, sizeof(current), INFINITE);*/
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
  };




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
      return static_cast<bool>(impl::atomicLoad(this->bits) & toInt(flags));
    }
    AGT_nodiscard AGT_forceinline bool testAll(EnumType flags) const noexcept {
      return (impl::atomicLoad(this->bits) & toInt(flags)) == toInt(flags);
    }
    AGT_nodiscard AGT_forceinline bool testAny() const noexcept {
      return static_cast<bool>(impl::atomicLoad(this->bits));
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
      return toEnum(impl::atomicLoad(this->bits));
    }
    AGT_nodiscard AGT_forceinline EnumType fetch(EnumType flags) const noexcept {
      return toEnum(impl::atomicLoad(this->bits) & toInt(flags));
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
      return toEnum(impl::atomicExchange(this->bits, static_cast<IntType>(0)));
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
      impl::atomicStore(this->bits, toInt(flags));
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
      return impl::atomicRelaxedLoad(value_);
    }


    agt_u32_t acquire() noexcept {
      return impl::atomicIncrement(value_);
    }
    agt_u32_t acquire(agt_u32_t n) noexcept {
      return impl::atomicExchangeAdd(value_, n) + n;
    }

    agt_u32_t release() noexcept {
      return impl::atomicDecrement(value_);
    }
    agt_u32_t release(agt_u32_t n) noexcept {
      return impl::atomicExchangeAdd(value_, 0 - n) - n;
    }

    agt_u32_t operator++()    noexcept {
      return this->acquire();
    }
    agt_u32_t operator++(int) noexcept {
      return impl::atomicExchangeAdd(value_, 1);
    }

    agt_u32_t operator--()    noexcept {
      return this->release();
    }
    agt_u32_t operator--(int) noexcept {
      return impl::atomicExchangeAdd(value_, -1);
    }

  private:

    agt_u32_t value_ = 0;
  };

  class atomic_monotonic_counter {
  public:
    atomic_monotonic_counter() = default;


    agt_u32_t getValue() const noexcept {
      return impl::atomicRelaxedLoad(value_);
      // return __iso_volatile_load32(&reinterpret_cast<const int&>(value_));
    }

    void reset() noexcept {
      impl::atomicStore(value_, 0);
    }



    agt_u32_t operator++()    noexcept {
      agt_u32_t result = impl::atomicIncrement(value_);
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
      const agt_u32_t waiters = impl::atomicRelaxedLoad(deepSleepers_);

      if ( waiters == 0 ) [[likely]] {

      } else if ( waiters == 1 ) {
        impl::atomicNotifyOne(value_);
      } else {
        impl::atomicNotifyAll(value_);
      }
    }

    AGT_forceinline agt_u32_t orderedLoad() const noexcept {
      return impl::atomicLoad(value_);
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
                         impl::atomicIncrement(deepSleepers_);
                         impl::atomicDeepWaitRaw(&value_, &capturedValue, sizeof(agt_u32_t));
                         impl::atomicDecrement(deepSleepers_);
                         );
      }
    }

    AGT_noinline    bool      deepWaitUntil(agt_u32_t expectedValue, deadline deadline) const noexcept {
      agt_u32_t capturedValue;
      agt_u32_t backoff = 0;

      while ( (capturedValue = impl::atomicLoad(value_)) < expectedValue ) {
        if ( deadline.hasPassed() )
          return false;
        DUFFS_MACHINE_EX(backoff,
          impl::atomicIncrement(deepSleepers_);
          bool result = impl::atomicDeepWaitRaw(&value_, &capturedValue, sizeof(agt_u32_t), deadline.toTimeoutMs());
          impl::atomicDecrement(deepSleepers_);
          if (!result) return false;
          );
      }

      return true;
    }


    agt_u32_t value_ = 0;
    mutable agt_u32_t deepSleepers_ = 0;
  };

}



#endif//JEMSYS_AGATE2_ATOMIC_HPP
