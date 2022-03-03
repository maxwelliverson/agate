//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_ATOMIC_HPP
#define JEMSYS_AGATE2_ATOMIC_HPP

#include "agate.h"

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

namespace Agt {

  class Deadline {


    inline constexpr static AgtUInt64 Nanoseconds  = 1;
    inline constexpr static AgtUInt64 NativePeriod = 100;
    inline constexpr static AgtUInt64 Microseconds = 1'000;
    inline constexpr static AgtUInt64 Milliseconds = 1'000'000;



    Deadline(AgtUInt64 timestamp) noexcept : timestamp(timestamp) { }

  public:



    AGT_forceinline static Deadline fromTimeout(AgtTimeout timeoutUs) noexcept {
      return { getCurrentTimestamp() + (timeoutUs * (Microseconds / NativePeriod)) };
    }

    AGT_forceinline AgtUInt32 toTimeoutMs() const noexcept {
      return getTimeoutNative() / (Milliseconds / NativePeriod);
    }

    AGT_forceinline AgtTimeout toTimeoutUs() const noexcept {
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

    AGT_forceinline AgtUInt64 getTimeoutNative() const noexcept {
      return timestamp - getCurrentTimestamp();
    }

    static AgtUInt64 getCurrentTimestamp() noexcept;

    AgtUInt64 timestamp;
  };
  
  namespace Impl {

    void      atomicStore(AgtUInt8& value,  AgtUInt8 newValue) noexcept;
    void      atomicStore(AgtUInt16& value, AgtUInt16 newValue) noexcept;
    void      atomicStore(AgtUInt32& value, AgtUInt32 newValue) noexcept;
    void      atomicStore(AgtUInt64& value, AgtUInt64 newValue) noexcept;
    AgtUInt8  atomicLoad(const AgtUInt8& value) noexcept;
    AgtUInt16 atomicLoad(const AgtUInt16& value) noexcept;
    AgtUInt32 atomicLoad(const AgtUInt32& value) noexcept;
    AgtUInt64 atomicLoad(const AgtUInt64& value) noexcept;

    void      atomicRelaxedStore(AgtUInt8& value,  AgtUInt8 newValue) noexcept;
    void      atomicRelaxedStore(AgtUInt16& value, AgtUInt16 newValue) noexcept;
    void      atomicRelaxedStore(AgtUInt32& value, AgtUInt32 newValue) noexcept;
    void      atomicRelaxedStore(AgtUInt64& value, AgtUInt64 newValue) noexcept;
    AgtUInt8  atomicRelaxedLoad(const AgtUInt8& value) noexcept;
    AgtUInt16 atomicRelaxedLoad(const AgtUInt16& value) noexcept;
    AgtUInt32 atomicRelaxedLoad(const AgtUInt32& value) noexcept;
    AgtUInt64 atomicRelaxedLoad(const AgtUInt64& value) noexcept;

    AgtUInt8  atomicExchange(AgtUInt8& value,  AgtUInt8 newValue) noexcept;
    AgtUInt16 atomicExchange(AgtUInt16& value, AgtUInt16 newValue) noexcept;
    AgtUInt32 atomicExchange(AgtUInt32& value, AgtUInt32 newValue) noexcept;
    AgtUInt64 atomicExchange(AgtUInt64& value, AgtUInt64 newValue) noexcept;
    bool      atomicCompareExchange(AgtUInt8& value,  AgtUInt8&  compare, AgtUInt8 newValue) noexcept;
    bool      atomicCompareExchange(AgtUInt16& value, AgtUInt16& compare, AgtUInt16 newValue) noexcept;
    bool      atomicCompareExchange(AgtUInt32& value, AgtUInt32& compare, AgtUInt32 newValue) noexcept;
    bool      atomicCompareExchange(AgtUInt64& value, AgtUInt64& compare, AgtUInt64 newValue) noexcept;
    bool      atomicCompareExchangeWeak(AgtUInt8& value,  AgtUInt8&  compare, AgtUInt8 newValue) noexcept;
    bool      atomicCompareExchangeWeak(AgtUInt16& value, AgtUInt16& compare, AgtUInt16 newValue) noexcept;
    bool      atomicCompareExchangeWeak(AgtUInt32& value, AgtUInt32& compare, AgtUInt32 newValue) noexcept;
    bool      atomicCompareExchangeWeak(AgtUInt64& value, AgtUInt64& compare, AgtUInt64 newValue) noexcept;
    AgtUInt8  atomicIncrement(AgtUInt8& value) noexcept;
    AgtUInt16 atomicIncrement(AgtUInt16& value) noexcept;
    AgtUInt32 atomicIncrement(AgtUInt32& value) noexcept;
    AgtUInt64 atomicIncrement(AgtUInt64& value) noexcept;
    AgtUInt8  atomicRelaxedIncrement(AgtUInt8& value) noexcept;
    AgtUInt16 atomicRelaxedIncrement(AgtUInt16& value) noexcept;
    AgtUInt32 atomicRelaxedIncrement(AgtUInt32& value) noexcept;
    AgtUInt64 atomicRelaxedIncrement(AgtUInt64& value) noexcept;
    AgtUInt8  atomicDecrement(AgtUInt8& value) noexcept;
    AgtUInt16 atomicDecrement(AgtUInt16& value) noexcept;
    AgtUInt32 atomicDecrement(AgtUInt32& value) noexcept;
    AgtUInt64 atomicDecrement(AgtUInt64& value) noexcept;
    AgtUInt8  atomicExchangeAdd(AgtUInt8&  value, AgtUInt8  newValue) noexcept;
    AgtUInt16 atomicExchangeAdd(AgtUInt16& value, AgtUInt16 newValue) noexcept;
    AgtUInt32 atomicExchangeAdd(AgtUInt32& value, AgtUInt32 newValue) noexcept;
    AgtUInt64 atomicExchangeAdd(AgtUInt64& value, AgtUInt64 newValue) noexcept;
    AgtUInt8  atomicExchangeAnd(AgtUInt8&  value, AgtUInt8  newValue) noexcept;
    AgtUInt16 atomicExchangeAnd(AgtUInt16& value, AgtUInt16 newValue) noexcept;
    AgtUInt32 atomicExchangeAnd(AgtUInt32& value, AgtUInt32 newValue) noexcept;
    AgtUInt64 atomicExchangeAnd(AgtUInt64& value, AgtUInt64 newValue) noexcept;
    AgtUInt8  atomicExchangeOr(AgtUInt8&  value, AgtUInt8 newValue) noexcept;
    AgtUInt16 atomicExchangeOr(AgtUInt16& value, AgtUInt16 newValue) noexcept;
    AgtUInt32 atomicExchangeOr(AgtUInt32& value, AgtUInt32 newValue) noexcept;
    AgtUInt64 atomicExchangeOr(AgtUInt64& value, AgtUInt64 newValue) noexcept;
    AgtUInt8  atomicExchangeXor(AgtUInt8&  value, AgtUInt8 newValue) noexcept;
    AgtUInt16 atomicExchangeXor(AgtUInt16& value, AgtUInt16 newValue) noexcept;
    AgtUInt32 atomicExchangeXor(AgtUInt32& value, AgtUInt32 newValue) noexcept;
    AgtUInt64 atomicExchangeXor(AgtUInt64& value, AgtUInt64 newValue) noexcept;

    void      atomicNotifyOne(void* value) noexcept;
    void      atomicNotifyAll(void* value) noexcept;
    void      atomicNotifyOneLocal(void* value) noexcept;
    void      atomicNotifyAllLocal(void* value) noexcept;
    void      atomicNotifyOneShared(void* value) noexcept;
    void      atomicNotifyAllShared(void* value) noexcept;

    template <std::integral T>
    inline void      atomicNotifyOne(T& value) noexcept {
      atomicNotifyOne(std::addressof(value));
    }
    template <std::integral T>
    inline void      atomicNotifyOneLocal(T& value) noexcept {
      atomicNotifyOneLocal(std::addressof(value));
    }
    template <std::integral T>
    inline void      atomicNotifyOneShared(T& value) noexcept {
      atomicNotifyOneShared(std::addressof(value));
    }
    template <std::integral T>
    inline void      atomicNotifyAll(T& value) noexcept {
      atomicNotifyAll(std::addressof(value));
    }
    template <std::integral T>
    inline void      atomicNotifyAllLocal(T& value) noexcept {
      atomicNotifyAllLocal(std::addressof(value));
    }
    template <std::integral T>
    inline void      atomicNotifyAllShared(T& value) noexcept {
      atomicNotifyAllShared(std::addressof(value));
    }


    // Deep Wait

    void      atomicDeepWaitRaw(const void* atomicAddress, void* compareAddress, AgtSize addressSize) noexcept;
    bool      atomicDeepWaitRaw(const void* atomicAddress, void* compareAddress, AgtSize addressSize, AgtUInt32 timeout) noexcept;

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
    inline bool atomicDeepWaitUntil(const T& value, Deadline deadline, std::type_identity_t<T> waitValue) noexcept {
      while ( atomicLoad(value) == waitValue ) {
        if (deadline.hasPassed())
          return false;
        atomicDeepWaitRaw(std::addressof(value), std::addressof(waitValue), sizeof(T));
      }
      return true;
    }

    template <typename T>
    inline bool atomicDeepWaitUntil(const T& value, T& capturedValue, Deadline deadline, std::type_identity_t<T> waitValue) noexcept {

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
    inline bool atomicDeepWaitUntil(const T& value, Deadline deadline, Fn&& functor) noexcept {

      T capturedValue;

      while ( !functor((capturedValue = atomicLoad(value))) ) {
        if (deadline.hasPassed())
          return false;
        atomicDeepWaitRaw(std::addressof(value), std::addressof(capturedValue), sizeof(T));
      }
      return true;
    }

    template <typename T, std::predicate<const T&> Fn>
    inline bool atomicDeepWaitUntil(const T& value, T& capturedValue, Deadline deadline, Fn&& functor) noexcept {

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
      AgtUInt32 backoff = 0;

      while ( atomicLoad(value) == waitValue ) {
        DUFFS_MACHINE_EX(backoff, atomicDeepWaitRaw(std::addressof(value), std::addressof(waitValue), sizeof(T)); );
      }
    }

    template <typename T>
    inline void atomicWait(const T& value, T& capturedValue, std::type_identity_t<T> waitValue) noexcept {
      T tmpValue;
      AgtUInt32 backoff = 0;

      while ( (tmpValue = atomicLoad(value)) == waitValue ) {
        DUFFS_MACHINE_EX(backoff, atomicDeepWaitRaw(std::addressof(value), std::addressof(waitValue), sizeof(T)); );
      }

      capturedValue = tmpValue;
    }

    template <typename T, std::predicate<const T&> Fn>
    inline void atomicWait(const T& value, Fn&& functor) noexcept {
      T capturedValue;
      AgtUInt32 backoff = 0;

      while ( !functor((capturedValue = atomicLoad(value))) ) {
        DUFFS_MACHINE_EX(backoff, atomicDeepWaitRaw(std::addressof(value), std::addressof(capturedValue), sizeof(T)); );
      }
    }

    template <typename T, std::predicate<const T&> Fn>
    inline void atomicWait(const T& value, T& capturedValue, Fn&& functor) noexcept {
      T tmpValue;
      AgtUInt32 backoff = 0;

      while ( !functor((tmpValue = atomicLoad(value))) ) {
        DUFFS_MACHINE_EX(backoff, atomicDeepWaitRaw(std::addressof(value), std::addressof(tmpValue), sizeof(T)); );
      }

      capturedValue = tmpValue;
    }

    template <typename T>
    inline bool atomicWaitUntil(const T& value, Deadline deadline, std::type_identity_t<T> waitValue) noexcept {
      AgtUInt32 backoff = 0;

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
    inline bool atomicWaitUntil(const T& value, T& capturedValue, Deadline deadline, std::type_identity_t<T> waitValue) noexcept {
      T tmpValue;
      AgtUInt32 backoff = 0;

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
    inline bool atomicWaitUntil(const T& value, Deadline deadline, Fn&& functor) noexcept {
      T capturedValue;
      AgtUInt32 backoff = 0;

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
    inline bool atomicWaitUntil(const T& value, T& capturedValue, Deadline deadline, Fn&& functor) noexcept {
      T tmpValue;
      AgtUInt32 backoff = 0;

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
    inline bool atomicWaitFor(const T& value, AgtTimeout timeout, std::type_identity_t<T> waitValue) noexcept {
      switch (timeout) {
        case AGT_WAIT:
          atomicWait(value, waitValue);
          return true;
        case AGT_DO_NOT_WAIT:
          return atomicLoad(value) != waitValue;
        default:
          return atomicWaitUntil(value, Deadline::fromTimeout(timeout), waitValue);
      }
    }

    template <typename T>
    inline bool atomicWaitFor(const T& value, T& capturedValue, AgtTimeout timeout, std::type_identity_t<T> waitValue) noexcept {
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
          return atomicWaitUntil(value, capturedValue, Deadline::fromTimeout(timeout), waitValue);
      }
    }

    template <typename T, std::predicate<const T&> Fn>
    inline bool atomicWaitFor(const T& value, AgtTimeout timeout, Fn&& functor) noexcept {
      switch (timeout) {
        case AGT_WAIT:
          atomicWait(value, std::forward<Fn>(functor));
          return true;
        case AGT_DO_NOT_WAIT:
          return functor(atomicLoad(value));
        default:
          return atomicWaitUntil(value, Deadline::fromTimeout(timeout), std::forward<Fn>(functor));
      }
    }

    template <typename T, std::predicate<const T&> Fn>
    inline bool atomicWaitFor(const T& value, T& capturedValue, AgtTimeout timeout, Fn&& functor) noexcept {
      switch (timeout) {
        case AGT_WAIT:
          atomicWait(value, capturedValue, std::forward<Fn>(functor));
          return true;
        case AGT_DO_NOT_WAIT:
          return functor((capturedValue = atomicLoad(value)));
        default:
          return atomicWaitUntil(value, capturedValue, Deadline::fromTimeout(timeout), std::forward<Fn>(functor));
      }
    }




    template <typename IntType_>
    class GenericAtomicFlags {
    public:
      
      using IntType = IntType_;
    protected:

      GenericAtomicFlags() = default;
      GenericAtomicFlags(IntType flags) noexcept : bits(flags){}


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

      AGT_forceinline bool waitExactUntil(IntType flags, Deadline deadline) const noexcept {
        return atomicWaitUntil(bits, deadline, [flags](IntType a) noexcept { return a == flags; });
      }
      AGT_forceinline bool waitAnyUntil(IntType flags, Deadline deadline) const noexcept {
        return atomicWaitUntil(bits, deadline, [flags](IntType a) noexcept { return (a & flags) != 0; });
      }
      AGT_forceinline bool waitAllUntil(IntType flags, Deadline deadline) const noexcept {
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


  class LocalBinarySemaphore {
    AgtInt32 val_;
  public:

    explicit LocalBinarySemaphore(AgtInt32 val) : val_(val) { }

    void acquire() noexcept;
    bool tryAcquire() noexcept;
    bool tryAcquireFor(AgtTimeout timeout) noexcept;
    bool tryAcquireUntil(Deadline deadline) noexcept;
    void release() noexcept;

  };

  class SharedBinarySemaphore {
    AgtInt32 val_;
  public:

    explicit SharedBinarySemaphore(AgtInt32 val) : val_(val) { }

    void acquire() noexcept;
    bool tryAcquire() noexcept;
    bool tryAcquireFor(AgtTimeout timeout) noexcept;
    bool tryAcquireUntil(Deadline deadline) noexcept;
    void release() noexcept;

  };

  class LocalSemaphore {
    AgtInt32 val_;
  public:

    explicit LocalSemaphore(AgtInt32 val) : val_(val) { }

    void acquire() noexcept;
    bool tryAcquire() noexcept;
    bool tryAcquireFor(AgtTimeout timeout) noexcept;
    bool tryAcquireUntil(Deadline deadline) noexcept;
    void release() noexcept;

  };

  class SharedSemaphore {
    AgtInt32  val_;
  public:

    explicit SharedSemaphore(AgtInt32 val) : val_(val) { }

    void acquire() noexcept;
    bool tryAcquire() noexcept;
    bool tryAcquireFor(AgtTimeout timeout) noexcept;
    bool tryAcquireUntil(Deadline deadline) noexcept;
    void release() noexcept;

  };




  template <typename E>
  class AtomicFlags : public Impl::GenericAtomicFlags<std::underlying_type_t<E>> {

    using UnderlyingType = std::underlying_type_t<E>;
    using Base = Impl::GenericAtomicFlags<std::underlying_type_t<E>>;
    using EnumType = E;
    using IntType = std::underlying_type_t<EnumType>;


    AGT_forceinline static IntType  toInt(EnumType e) noexcept {
      return static_cast<IntType>(e);
    }
    AGT_forceinline static EnumType toEnum(IntType i) noexcept {
      return static_cast<EnumType>(i);
    }


  public:

    AtomicFlags() = default;
    AtomicFlags(EnumType value) noexcept : Base() {}


    AGT_nodiscard AGT_forceinline bool test(EnumType flags) const noexcept {
      return this->testAny(flags);
    }

    AGT_nodiscard AGT_forceinline bool testAny(EnumType flags) const noexcept {
      return static_cast<bool>(Impl::atomicLoad(this->bits) & toInt(flags));
    }
    AGT_nodiscard AGT_forceinline bool testAll(EnumType flags) const noexcept {
      return (Impl::atomicLoad(this->bits) & toInt(flags)) == toInt(flags);
    }
    AGT_nodiscard AGT_forceinline bool testAny() const noexcept {
      return static_cast<bool>(Impl::atomicLoad(this->bits));
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
      return toEnum(Impl::atomicLoad(this->bits));
    }
    AGT_nodiscard AGT_forceinline EnumType fetch(EnumType flags) const noexcept {
      return toEnum(Impl::atomicLoad(this->bits) & toInt(flags));
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
      return toEnum(Impl::atomicExchange(this->bits, static_cast<IntType>(0)));
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
      Impl::atomicStore(this->bits, toInt(flags));
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

    AGT_forceinline bool waitExactUntil(EnumType flags, Deadline deadline) const noexcept {
      return this->waitExactUntil(toInt(flags), deadline);
    }
    AGT_forceinline bool waitAnyUntil(EnumType flags, Deadline deadline) const noexcept {
      return this->waitAnyUntil(toInt(flags), deadline);
    }
    AGT_forceinline bool waitAllUntil(EnumType flags, Deadline deadline) const noexcept {
      return this->waitAllUntil(toInt(flags), deadline);
    }

    AGT_forceinline void notifyOne() noexcept {
      atomicNotifyOne(this->bits);
    }
    AGT_forceinline void notifyAll() noexcept {
      atomicNotifyAll(this->bits);
    }

  };


  class ReferenceCount {
  public:

    ReferenceCount() = default;
    explicit ReferenceCount(AgtUInt32 initialCount) noexcept : value_(initialCount) { }


    AGT_nodiscard AgtUInt32 get() const noexcept {
      return Impl::atomicRelaxedLoad(value_);
    }


    AgtUInt32 acquire() noexcept {
      return Impl::atomicIncrement(value_);
    }
    AgtUInt32 acquire(AgtUInt32 n) noexcept {
      return Impl::atomicExchangeAdd(value_, n) + n;
    }

    AgtUInt32 release() noexcept {
      return Impl::atomicDecrement(value_);
    }
    AgtUInt32 release(AgtUInt32 n) noexcept {
      return Impl::atomicExchangeAdd(value_, 0 - n) - n;
    }

    AgtUInt32 operator++()    noexcept {
      return this->acquire();
    }
    AgtUInt32 operator++(int) noexcept {
      return Impl::atomicExchangeAdd(value_, 1);
    }

    AgtUInt32 operator--()    noexcept {
      return this->release();
    }
    AgtUInt32 operator--(int) noexcept {
      return Impl::atomicExchangeAdd(value_, -1);
    }

  private:

    AgtUInt32 value_ = 0;
  };

  class AtomicMonotonicCounter {
  public:

    AtomicMonotonicCounter() = default;


    AgtUInt32 getValue() const noexcept {
      return Impl::atomicRelaxedLoad(value_);
      // return __iso_volatile_load32(&reinterpret_cast<const int&>(value_));
    }

    void reset() noexcept {
      Impl::atomicStore(value_, 0);
    }



    AgtUInt32 operator++()    noexcept {
      AgtUInt32 result = Impl::atomicIncrement(value_);
      notifyWaiters();
      return result;
    }
    AgtUInt32 operator++(int) noexcept {
      AgtUInt32 result = _InterlockedExchangeAdd(&value_, 1) + 1;
      notifyWaiters();
      return result;
    }


    bool      waitFor(AgtUInt32 expectedValue, AgtTimeout timeout) const noexcept {
      switch (timeout) {
        case AGT_WAIT:
          deepWait(expectedValue);
          return true;
        case AGT_DO_NOT_WAIT:
          return isAtLeast(expectedValue);
        default:
          return deepWaitUntil(expectedValue, Deadline::fromTimeout(timeout));
      }
    }


  private:

    void notifyWaiters() noexcept {
      const AgtUInt32 waiters = Impl::atomicRelaxedLoad(deepSleepers_);

      if ( waiters == 0 ) [[likely]] {

      } else if ( waiters == 1 ) {
        Impl::atomicNotifyOne(value_);
      } else {
        Impl::atomicNotifyAll(value_);
      }
    }

    AGT_forceinline AgtUInt32 orderedLoad() const noexcept {
      return Impl::atomicLoad(value_);
    }

    AGT_forceinline bool isAtLeast(AgtUInt32 value) const noexcept {
      return orderedLoad() >= value;
    }

    AGT_forceinline bool      tryWaitOnce(AgtUInt32 expectedValue) const noexcept {
      return orderedLoad() >= expectedValue;
    }

    AGT_noinline    void      deepWait(AgtUInt32 expectedValue) const noexcept {
      AgtUInt32 capturedValue;
      AgtUInt32 backoff = 0;
      while ( (capturedValue = orderedLoad()) < expectedValue ) {
        DUFFS_MACHINE_EX(backoff,
                         Impl::atomicIncrement(deepSleepers_);
                         Impl::atomicDeepWaitRaw(&value_, &capturedValue, sizeof(AgtUInt32));
                         Impl::atomicDecrement(deepSleepers_);
                         );
      }
    }

    AGT_noinline    bool      deepWaitUntil(AgtUInt32 expectedValue, Deadline deadline) const noexcept {
      AgtUInt32 capturedValue;
      AgtUInt32 backoff = 0;

      while ( (capturedValue = Impl::atomicLoad(value_)) < expectedValue ) {
        if ( deadline.hasPassed() )
          return false;
        DUFFS_MACHINE_EX(backoff,
          Impl::atomicIncrement(deepSleepers_);
          bool result = Impl::atomicDeepWaitRaw(&value_, &capturedValue, sizeof(AgtUInt32), deadline.toTimeoutMs());
          Impl::atomicDecrement(deepSleepers_);
          if (!result) return false;
          );
      }

      return true;
    }


    AgtUInt32 value_ = 0;
    mutable AgtUInt32 deepSleepers_ = 0;
  };

}

#endif//JEMSYS_AGATE2_ATOMIC_HPP
