//
// Created by maxwe on 2021-12-02.
//

#include "atomic.hpp" // for IDE sake

// #include <intrin.h>
// #include <mmintrin.h>
// #include <atomic_ops.h>


#if AGT_system_windows
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif AGT_system_linux
#include <linux/futex.h>
#include <time.h>
#endif


using namespace agt;



inline agt_u8_t  agt::atomic_load(const agt_u8_t& value) noexcept {
#if AGT_system_windows
  return (agt_u8_t)_InterlockedCompareExchange8((char*)&value, 0, 0);
#else
  return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u16_t agt::atomic_load(const agt_u16_t& value) noexcept {
#if AGT_system_windows
  return (agt_u16_t)_InterlockedCompareExchange16((agt_i16_t*)&value, 0, 0);
#else
  return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u32_t agt::atomic_load(const agt_u32_t& value) noexcept {
#if AGT_system_windows
  return _InterlockedCompareExchange((agt_u32_t*)&value, 0, 0);
#else
  return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u64_t agt::atomic_load(const agt_u64_t& value) noexcept {
#if AGT_system_windows
  return _InterlockedCompareExchange((agt_u64_t*)&value, 0, 0);
#else
  return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
#endif
}

inline agt_u8_t  agt::atomic_relaxed_load(const agt_u8_t& value) noexcept {
#if AGT_system_windows
  return __iso_volatile_load8((const char*)&value);
#else
  return __atomic_load_n(&value, __ATOMIC_RELAXED);
#endif
}
inline agt_u16_t agt::atomic_relaxed_load(const agt_u16_t& value) noexcept {
#if AGT_system_windows
  return __iso_volatile_load16((const agt_i16_t*)&value);
#else
  return __atomic_load_n(&value, __ATOMIC_RELAXED);
#endif
}
inline agt_u32_t agt::atomic_relaxed_load(const agt_u32_t& value) noexcept {
#if AGT_system_windows
  return __iso_volatile_load32((const agt_i32_t*)&value);
#else
  return __atomic_load_n(&value, __ATOMIC_RELAXED);
#endif
}
inline agt_u64_t agt::atomic_relaxed_load(const agt_u64_t& value) noexcept {
#if AGT_system_windows
  return __iso_volatile_load64((const agt_i64_t*)&value);
#else
  return __atomic_load_n(&value, __ATOMIC_RELAXED);
#endif
}

inline void agt::atomic_store(agt_u8_t& value,  agt_u8_t newValue) noexcept {
#if AGT_system_windows
  agt::atomic_exchange(value, newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline void agt::atomic_store(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  agt::atomic_exchange(value, newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline void agt::atomic_store(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  agt::atomic_exchange(value, newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline void agt::atomic_store(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  agt::atomic_exchange(value, newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}

inline void agt::atomic_relaxed_store(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  __iso_volatile_store8((char*)&value, (char)newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_RELAXED);
#endif
}
inline void agt::atomic_relaxed_store(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  __iso_volatile_store16((agt_i16_t*)&value, (agt_i16_t)newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_RELAXED);
#endif
}
inline void agt::atomic_relaxed_store(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  __iso_volatile_store32((agt_i32_t*)&value, (agt_i32_t)newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_RELAXED);
#endif
}
inline void agt::atomic_relaxed_store(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  __iso_volatile_store64((agt_i64_t*)&value, (agt_i64_t)newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_RELAXED);
#endif
}


inline agt_u8_t  agt::atomic_exchange(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u8_t)_InterlockedExchange8((char*)&value, (char)newValue);
#else
  return __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u16_t agt::atomic_exchange(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u16_t)_InterlockedExchange16((agt_i16_t*)&value, (agt_i16_t)newValue);
#else
  return __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u32_t agt::atomic_exchange(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  return _InterlockedExchange(&value, newValue);
#else
  return __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u64_t agt::atomic_exchange(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  return _InterlockedExchange(&value, newValue);
#else
  return __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}


inline bool        agt::atomic_cas(agt_u8_t& value,  agt_u8_t&  compare, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  agt_u8_t oldComp = compare;
  return (compare = (agt_u8_t)_InterlockedCompareExchange8((char*)&value, (char)newValue, (char)compare)) == oldComp;
#else
  return __atomic_compare_exchange_n((char*)&value, (char*)&compare, (char)newValue, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#endif
}
inline bool        agt::atomic_cas(agt_u16_t& value, agt_u16_t& compare, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  agt_u16_t oldComp = compare;
  return (compare = (agt_u16_t)_InterlockedCompareExchange16((agt_i16_t*)&value, (agt_i16_t)newValue, (agt_i16_t)compare)) == oldComp;
#else
  return __atomic_compare_exchange_n(&value, &compare, newValue, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#endif
}
inline bool        agt::atomic_cas(agt_u32_t& value, agt_u32_t& compare, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  agt_u32_t oldComp = compare;
  return (compare = _InterlockedCompareExchange(&value, newValue, compare)) == oldComp;
#else
  return __atomic_compare_exchange_n(&value, &compare, newValue, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#endif
}
inline bool        agt::atomic_cas(agt_u64_t& value, agt_u64_t& compare, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  agt_u64_t oldComp = compare;
  return (compare = _InterlockedCompareExchange(&value, newValue, compare)) == oldComp;
#else
  return __atomic_compare_exchange_n(&value, &compare, newValue, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#endif
}
inline bool        agt::atomic_cas_16bytes(void* value, void* compare, const void* newValue) noexcept {
  const auto val = static_cast<const long long*>(newValue);
  const auto cmp = static_cast<long long*>(compare);
  return _InterlockedCompareExchange128(static_cast<long long*>(value), val[1], val[0], cmp);
}


inline bool agt::atomic_try_replace(agt_u8_t& value,  agt_u8_t compare, agt_u8_t newValue)    noexcept {
  return atomic_cas(value, compare, newValue); // on x64, there's no difference between weak and strong CAS
}
inline bool agt::atomic_try_replace(agt_u16_t& value, agt_u16_t compare, agt_u16_t newValue) noexcept {
  return atomic_cas(value, compare, newValue);
}
inline bool agt::atomic_try_replace(agt_u32_t& value, agt_u32_t compare, agt_u32_t newValue) noexcept {
  return atomic_cas(value, compare, newValue);
}
inline bool agt::atomic_try_replace(agt_u64_t& value, agt_u64_t compare, agt_u64_t newValue) noexcept {
  return atomic_cas(value, compare, newValue);
}
inline bool agt::atomic_try_replace_16bytes(void *value, void *compare, const void *newValue) noexcept {
  return atomic_cas_16bytes(value, compare, newValue);
}





inline agt_u8_t  agt::atomic_increment(agt_u8_t& value) noexcept {
#if AGT_system_windows
  return (agt_u8_t)_InterlockedExchangeAdd8((char*)&value, 1) + 1;
#else
  return __atomic_add_fetch(&value, 1, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u16_t agt::atomic_increment(agt_u16_t& value) noexcept {
#if AGT_system_windows
  return (agt_u16_t)InterlockedIncrement16(reinterpret_cast<agt_i16_t*>(&value));
#else
  return __atomic_add_fetch(&value, 1, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u32_t agt::atomic_increment(agt_u32_t& value) noexcept {
#if AGT_system_windows
  return InterlockedIncrement(&value);
#else
  return __atomic_add_fetch(&value, 1, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u64_t agt::atomic_increment(agt_u64_t& value) noexcept {
#if AGT_system_windows
  return InterlockedIncrement(&value);
#else
  return __atomic_add_fetch(&value, 1, __ATOMIC_SEQ_CST);
#endif
}

inline agt_u8_t agt::atomic_relaxed_increment(agt_u8_t& value) noexcept {
  return atomic_increment(value);
}
inline agt_u16_t agt::atomic_relaxed_increment(agt_u16_t& value) noexcept {
  return atomic_increment(value);
}
inline agt_u32_t agt::atomic_relaxed_increment(agt_u32_t& value) noexcept {
  return atomic_increment(value);
}
inline agt_u64_t agt::atomic_relaxed_increment(agt_u64_t& value) noexcept {
  return atomic_increment(value);
}



inline agt_u8_t  agt::atomic_decrement(agt_u8_t& value) noexcept {
#if AGT_system_windows
  return (agt_u8_t)_InterlockedExchangeAdd8((char*)&value, (char)-1) - 1;
#else
  return __atomic_sub_fetch(&value, 1, __ATOMIC_SEQ_CST) - 1;
#endif
}
inline agt_u16_t agt::atomic_decrement(agt_u16_t& value) noexcept {
#if AGT_system_windows
  return (agt_u16_t)_InterlockedDecrement16((agt_i16_t*)&value);
#else
  return __atomic_sub_fetch(&value, 1, __ATOMIC_SEQ_CST) - 1;
#endif
}
inline agt_u32_t agt::atomic_decrement(agt_u32_t& value) noexcept {
#if AGT_system_windows
  return _InterlockedDecrement(&value);
#else
  return __atomic_sub_fetch(&value, 1, __ATOMIC_SEQ_CST) - 1;
#endif
}
inline agt_u64_t agt::atomic_decrement(agt_u64_t& value) noexcept {
#if AGT_system_windows
  return _InterlockedDecrement(&value);
#else
  return __atomic_sub_fetch(&value, 1, __ATOMIC_SEQ_CST) - 1;
#endif
}


inline agt_u8_t  agt::atomic_exchange_add(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u8_t)_InterlockedExchangeAdd8((char*)&value, (char)newValue);
#else
  return __atomic_fetch_add(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u16_t agt::atomic_exchange_add(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u16_t)_InterlockedExchangeAdd16((agt_i16_t*)&value, (agt_i16_t)newValue);
#else
  return __atomic_fetch_add(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u32_t agt::atomic_exchange_add(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  return _InterlockedExchangeAdd(&value, newValue);
#else
  return __atomic_fetch_add(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u64_t agt::atomic_exchange_add(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  return _InterlockedExchangeAdd(&value, newValue);
#else
  return __atomic_fetch_add(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}


inline void      agt::atomic_add(agt_u8_t&  value, agt_u8_t  newValue) noexcept {
  atomic_exchange_add(value, newValue);
}
inline void      agt::atomic_add(agt_u16_t& value, agt_u16_t newValue) noexcept {
  atomic_exchange_add(value, newValue);
}
inline void      agt::atomic_add(agt_u32_t& value, agt_u32_t newValue) noexcept {
  atomic_exchange_add(value, newValue);
}
inline void      agt::atomic_add(agt_u64_t& value, agt_u64_t newValue) noexcept {
  atomic_exchange_add(value, newValue);
}


inline agt_u8_t  agt::atomic_exchange_sub(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u8_t)_InterlockedExchangeAdd8((char*)&value, -(char)newValue);
#else
  return __atomic_fetch_add(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u16_t agt::atomic_exchange_sub(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u16_t)_InterlockedExchangeAdd16((agt_i16_t*)&value, -(agt_i16_t)newValue);
#else
  return __atomic_fetch_add(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u32_t agt::atomic_exchange_sub(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  return _InterlockedExchangeAdd(&value, -newValue);
#else
  return __atomic_fetch_add(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u64_t agt::atomic_exchange_sub(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  return _InterlockedExchangeSub64(reinterpret_cast<LONGLONG*>(&value), std::bit_cast<agt_i64_t>(newValue));

#else
  return __atomic_fetch_add(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}

inline void      agt::atomic_sub(agt_u8_t&  value, agt_u8_t  newValue) noexcept {
  atomic_exchange_sub(value, newValue);
}
inline void      agt::atomic_sub(agt_u16_t& value, agt_u16_t newValue) noexcept {
  atomic_exchange_sub(value, newValue);
}
inline void      agt::atomic_sub(agt_u32_t& value, agt_u32_t newValue) noexcept {
  atomic_exchange_sub(value, newValue);
}
inline void      agt::atomic_sub(agt_u64_t& value, agt_u64_t newValue) noexcept {
  atomic_exchange_sub(value, newValue);
}



inline agt_u8_t  agt::atomic_exchange_and(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u8_t)InterlockedAnd8((char*)&value, (char)newValue);
#else
  return __atomic_fetch_and(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u16_t agt::atomic_exchange_and(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u16_t)InterlockedAnd16((agt_i16_t*)&value, (agt_i16_t)newValue);
#else
  return __atomic_fetch_and(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u32_t agt::atomic_exchange_and(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u32_t)InterlockedAnd((LONG*)&value, (agt_i32_t)newValue);
#else
  return __atomic_fetch_and(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u64_t agt::atomic_exchange_and(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u64_t)InterlockedAnd64((agt_i64_t*)&value, (agt_i64_t)newValue);
#else
  return __atomic_fetch_and(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}

inline void      agt::atomic_and(agt_u8_t& value, agt_u8_t newValue)   noexcept {
  atomic_exchange_and(value, newValue);
}
inline void      agt::atomic_and(agt_u16_t& value, agt_u16_t newValue) noexcept {
  atomic_exchange_and(value, newValue);
}
inline void      agt::atomic_and(agt_u32_t& value, agt_u32_t newValue) noexcept {
  atomic_exchange_and(value, newValue);
}
inline void      agt::atomic_and(agt_u64_t& value, agt_u64_t newValue) noexcept {
  atomic_exchange_and(value, newValue);
}


inline agt_u8_t  agt::atomic_exchange_or(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u8_t)InterlockedOr8((char*)&value, (char)newValue);
#else
  return __atomic_fetch_or(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u16_t agt::atomic_exchange_or(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u16_t)InterlockedOr16((agt_i16_t*)&value, (agt_i16_t)newValue);
#else
  return __atomic_fetch_or(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u32_t agt::atomic_exchange_or(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u32_t)InterlockedOr((LONG*)&value, (agt_i32_t)newValue);
#else
  return __atomic_fetch_or(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u64_t agt::atomic_exchange_or(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u64_t)InterlockedOr64((agt_i64_t*)&value, (agt_i64_t)newValue);
#else
  return __atomic_fetch_or(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}

inline void      agt::atomic_or(agt_u8_t& value, agt_u8_t newValue)   noexcept {
  atomic_exchange_or(value, newValue);
}
inline void      agt::atomic_or(agt_u16_t& value, agt_u16_t newValue) noexcept {
  atomic_exchange_or(value, newValue);
}
inline void      agt::atomic_or(agt_u32_t& value, agt_u32_t newValue) noexcept {
  atomic_exchange_or(value, newValue);
}
inline void      agt::atomic_or(agt_u64_t& value, agt_u64_t newValue) noexcept {
  atomic_exchange_or(value, newValue);
}


inline agt_u8_t  agt::atomic_exchange_xor(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u8_t)InterlockedXor8((char*)&value, (char)newValue);
#else
  return __atomic_fetch_xor(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u16_t agt::atomic_exchange_xor(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u16_t)InterlockedXor16((agt_i16_t*)&value, (agt_i16_t)newValue);
#else
  return __atomic_fetch_xor(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u32_t agt::atomic_exchange_xor(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u32_t)InterlockedXor((LONG*)&value, (agt_i32_t)newValue);
#else
  return __atomic_fetch_xor(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
inline agt_u64_t agt::atomic_exchange_xor(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u64_t)InterlockedXor64((agt_i64_t*)&value, (agt_i64_t)newValue);
#else
  return __atomic_fetch_xor(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}

inline void      agt::atomic_xor(agt_u8_t& value, agt_u8_t newValue)   noexcept {
  atomic_exchange_xor(value, newValue);
}
inline void      agt::atomic_xor(agt_u16_t& value, agt_u16_t newValue) noexcept {
  atomic_exchange_xor(value, newValue);
}
inline void      agt::atomic_xor(agt_u32_t& value, agt_u32_t newValue) noexcept {
  atomic_exchange_xor(value, newValue);
}
inline void      agt::atomic_xor(agt_u64_t& value, agt_u64_t newValue) noexcept {
  atomic_exchange_xor(value, newValue);
}


inline agt_u8_t  agt::atomic_exchange_nand(agt_u8_t& value, agt_u8_t newValue) noexcept {
  return atomic_exchange_and(value, ~newValue);
}
inline agt_u16_t agt::atomic_exchange_nand(agt_u16_t& value, agt_u16_t newValue) noexcept {
  return atomic_exchange_and(value, ~newValue);
}
inline agt_u32_t agt::atomic_exchange_nand(agt_u32_t& value, agt_u32_t newValue) noexcept {
  return atomic_exchange_and(value, ~newValue);
}
inline agt_u64_t agt::atomic_exchange_nand(agt_u64_t& value, agt_u64_t newValue) noexcept {
  return atomic_exchange_and(value, ~newValue);
}

inline void      agt::atomic_nand(agt_u8_t& value, agt_u8_t newValue)   noexcept {
  atomic_exchange_nand(value, newValue);
}
inline void      agt::atomic_nand(agt_u16_t& value, agt_u16_t newValue) noexcept {
  atomic_exchange_nand(value, newValue);
}
inline void      agt::atomic_nand(agt_u32_t& value, agt_u32_t newValue) noexcept {
  atomic_exchange_nand(value, newValue);
}
inline void      agt::atomic_nand(agt_u64_t& value, agt_u64_t newValue) noexcept {
  atomic_exchange_nand(value, newValue);
}




inline agt_u64_t deadline::getCurrentTimestamp() noexcept {
#if AGT_system_windows
  LARGE_INTEGER lrgInt;
  QueryPerformanceCounter(&lrgInt);
  return lrgInt.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (static_cast<agt_u64_t>(ts.tv_sec) * 1000000000ULL) + static_cast<agt_u64_t>(ts.tv_nsec);
#endif
}
