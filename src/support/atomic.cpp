//
// Created by maxwe on 2021-12-02.
//

#include "atomic.hpp"

// #include <intrin.h>
// #include <mmintrin.h>
// #include <atomic_ops.h>
#if AGT_system_linux
#include <linux/futex.h>
#endif


using namespace agt;



agt_u8_t  impl::atomicLoad(const agt_u8_t& value) noexcept {
#if AGT_system_windows
  return (agt_u8_t)_InterlockedCompareExchange8((char*)&value, 0, 0);
#else
  return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
#endif
}
agt_u16_t impl::atomicLoad(const agt_u16_t& value) noexcept {
#if AGT_system_windows
  return (agt_u16_t)_InterlockedCompareExchange16((agt_i16_t*)&value, 0, 0);
#else
  return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
#endif
}
agt_u32_t impl::atomicLoad(const agt_u32_t& value) noexcept {
#if AGT_system_windows
  return _InterlockedCompareExchange((agt_u32_t*)&value, 0, 0);
#else
  return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
#endif
}
agt_u64_t impl::atomicLoad(const agt_u64_t& value) noexcept {
#if AGT_system_windows
  return _InterlockedCompareExchange((agt_u64_t*)&value, 0, 0);
#else
  return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
#endif
}

agt_u8_t impl::atomicRelaxedLoad(const agt_u8_t& value) noexcept {
#if AGT_system_windows
  return __iso_volatile_load8((const char*)&value);
#else
  return __atomic_load_n(&value, __ATOMIC_RELAXED);
#endif
}
agt_u16_t impl::atomicRelaxedLoad(const agt_u16_t& value) noexcept {
#if AGT_system_windows
  return __iso_volatile_load16((const agt_i16_t*)&value);
#else
  return __atomic_load_n(&value, __ATOMIC_RELAXED);
#endif
}
agt_u32_t impl::atomicRelaxedLoad(const agt_u32_t& value) noexcept {
#if AGT_system_windows
  return __iso_volatile_load32((const agt_i32_t*)&value);
#else
  return __atomic_load_n(&value, __ATOMIC_RELAXED);
#endif
}
agt_u64_t impl::atomicRelaxedLoad(const agt_u64_t& value) noexcept {
#if AGT_system_windows
  return __iso_volatile_load64((const agt_i64_t*)&value);
#else
  return __atomic_load_n(&value, __ATOMIC_RELAXED);
#endif
}

void impl::atomicStore(agt_u8_t& value,  agt_u8_t newValue) noexcept {
#if AGT_system_windows
  impl::atomicExchange(value, newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
void impl::atomicStore(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  impl::atomicExchange(value, newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
void impl::atomicStore(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  impl::atomicExchange(value, newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
void impl::atomicStore(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  impl::atomicExchange(value, newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}

void impl::atomicRelaxedStore(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  __iso_volatile_store8((char*)&value, (char)newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_RELAXED);
#endif
}
void impl::atomicRelaxedStore(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  __iso_volatile_store16((agt_i16_t*)&value, (agt_i16_t)newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_RELAXED);
#endif
}
void impl::atomicRelaxedStore(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  __iso_volatile_store32((agt_i32_t*)&value, (agt_i32_t)newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_RELAXED);
#endif
}
void impl::atomicRelaxedStore(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  __iso_volatile_store64((agt_i64_t*)&value, (agt_i64_t)newValue);
#else
  __atomic_store_n(&value, newValue, __ATOMIC_RELAXED);
#endif
}


agt_u8_t  impl::atomicExchange(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u8_t)_InterlockedExchange8((char*)&value, (char)newValue);
#else
  return __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u16_t impl::atomicExchange(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u16_t)_InterlockedExchange16((agt_i16_t*)&value, (agt_i16_t)newValue);
#else
  return __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u32_t impl::atomicExchange(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  return _InterlockedExchange(&value, newValue);
#else
  return __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u64_t impl::atomicExchange(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  return _InterlockedExchange(&value, newValue);
#else
  return __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}

bool impl::atomicCompareExchange(agt_u8_t& value,  agt_u8_t& compare, agt_u8_t newValue)    noexcept {
#if AGT_system_windows
  agt_u8_t oldComp = compare;
  return (compare = (agt_u8_t)_InterlockedCompareExchange8((char*)&value, (char)newValue, (char)compare)) == oldComp;
#else
  return __atomic_compare_exchange_n((char*)&value, (char*)&compare, (char)newValue, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#endif
}
bool impl::atomicCompareExchange(agt_u16_t& value, agt_u16_t& compare, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  agt_u16_t oldComp = compare;
  return (compare = (agt_u16_t)_InterlockedCompareExchange16((agt_i16_t*)&value, (agt_i16_t)newValue, (agt_i16_t)compare)) == oldComp;
#else
  return __atomic_compare_exchange_n(&value, &compare, newValue, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#endif
}
bool impl::atomicCompareExchange(agt_u32_t& value, agt_u32_t& compare, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  agt_u32_t oldComp = compare;
  return (compare = _InterlockedCompareExchange(&value, newValue, compare)) == oldComp;
#else
  return __atomic_compare_exchange_n(&value, &compare, newValue, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#endif
}
bool impl::atomicCompareExchange(agt_u64_t& value, agt_u64_t& compare, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  agt_u64_t oldComp = compare;
  return (compare = _InterlockedCompareExchange(&value, newValue, compare)) == oldComp;
#else
  return __atomic_compare_exchange_n(&value, &compare, newValue, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#endif
}


agt_u8_t  impl::atomicIncrement(agt_u8_t& value) noexcept {
#if AGT_system_windows
  return (agt_u8_t)_InterlockedExchangeAdd8((char*)&value, 1) + 1;
#else
  return __atomic_add_fetch(&value, 1, __ATOMIC_SEQ_CST);
#endif
}
agt_u16_t impl::atomicIncrement(agt_u16_t& value) noexcept {
#if AGT_system_windows
  return (agt_u16_t)InterlockedIncrement16(reinterpret_cast<agt_i16_t*>(&value));
#else
  return __atomic_add_fetch(&value, 1, __ATOMIC_SEQ_CST);
#endif
}
agt_u32_t impl::atomicIncrement(agt_u32_t& value) noexcept {
#if AGT_system_windows
  return InterlockedIncrement(&value);
#else
  return __atomic_add_fetch(&value, 1, __ATOMIC_SEQ_CST);
#endif
}
agt_u64_t impl::atomicIncrement(agt_u64_t& value) noexcept {
#if AGT_system_windows
  return InterlockedIncrement(&value);
#else
  return __atomic_add_fetch(&value, 1, __ATOMIC_SEQ_CST);
#endif
}


agt_u8_t  impl::atomicDecrement(agt_u8_t& value) noexcept {
#if AGT_system_windows
  return (agt_u8_t)_InterlockedExchangeAdd8((char*)&value, (char)-1) - 1;
#else
  return __atomic_sub_fetch(&value, 1, __ATOMIC_SEQ_CST);
#endif
}
agt_u16_t impl::atomicDecrement(agt_u16_t& value) noexcept {
#if AGT_system_windows
  return (agt_u16_t)_InterlockedDecrement16((agt_i16_t*)&value);
#else
  return __atomic_sub_fetch(&value, 1, __ATOMIC_SEQ_CST);
#endif
}
agt_u32_t impl::atomicDecrement(agt_u32_t& value) noexcept {
#if AGT_system_windows
  return _InterlockedDecrement(&value);
#else
  return __atomic_sub_fetch(&value, 1, __ATOMIC_SEQ_CST);
#endif
}
agt_u64_t impl::atomicDecrement(agt_u64_t& value) noexcept {
#if AGT_system_windows
  return _InterlockedDecrement(&value);
#else
  return __atomic_sub_fetch(&value, 1, __ATOMIC_SEQ_CST);
#endif
}


agt_u8_t  impl::atomicExchangeAdd(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u8_t)_InterlockedExchangeAdd8((char*)&value, (char)newValue);
#else
  return __atomic_fetch_add(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u16_t impl::atomicExchangeAdd(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u16_t)_InterlockedExchangeAdd16((agt_i16_t*)&value, (agt_i16_t)newValue);
#else
  return __atomic_fetch_add(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u32_t impl::atomicExchangeAdd(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  return _InterlockedExchangeAdd(&value, newValue);
#else
  return __atomic_fetch_add(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u64_t impl::atomicExchangeAdd(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  return _InterlockedExchangeAdd(&value, newValue);
#else
  return __atomic_fetch_add(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}


agt_u8_t  impl::atomicExchangeAnd(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u8_t)InterlockedAnd8((char*)&value, (char)newValue);
#else
  return __atomic_fetch_and(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u16_t impl::atomicExchangeAnd(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u16_t)InterlockedAnd16((agt_i16_t*)&value, (agt_i16_t)newValue);
#else
  return __atomic_fetch_and(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u32_t impl::atomicExchangeAnd(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u32_t)InterlockedAnd((LONG*)&value, (agt_i32_t)newValue);
#else
  return __atomic_fetch_and(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u64_t impl::atomicExchangeAnd(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u64_t)InterlockedAnd64((agt_i64_t*)&value, (agt_i64_t)newValue);
#else
  return __atomic_fetch_and(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}


agt_u8_t  impl::atomicExchangeOr(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u8_t)InterlockedOr8((char*)&value, (char)newValue);
#else
  return __atomic_fetch_or(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u16_t impl::atomicExchangeOr(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u16_t)InterlockedOr16((agt_i16_t*)&value, (agt_i16_t)newValue);
#else
  return __atomic_fetch_or(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u32_t impl::atomicExchangeOr(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u32_t)InterlockedOr((LONG*)&value, (agt_i32_t)newValue);
#else
  return __atomic_fetch_or(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u64_t impl::atomicExchangeOr(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u64_t)InterlockedOr64((agt_i64_t*)&value, (agt_i64_t)newValue);
#else
  return __atomic_fetch_or(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}


agt_u8_t  impl::atomicExchangeXor(agt_u8_t& value, agt_u8_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u8_t)InterlockedXor8((char*)&value, (char)newValue);
#else
  return __atomic_fetch_xor(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u16_t impl::atomicExchangeXor(agt_u16_t& value, agt_u16_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u16_t)InterlockedXor16((agt_i16_t*)&value, (agt_i16_t)newValue);
#else
  return __atomic_fetch_xor(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u32_t impl::atomicExchangeXor(agt_u32_t& value, agt_u32_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u32_t)InterlockedXor((LONG*)&value, (agt_i32_t)newValue);
#else
  return __atomic_fetch_xor(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}
agt_u64_t impl::atomicExchangeXor(agt_u64_t& value, agt_u64_t newValue) noexcept {
#if AGT_system_windows
  return (agt_u64_t)InterlockedXor64((agt_i64_t*)&value, (agt_i64_t)newValue);
#else
  return __atomic_fetch_xor(&value, newValue, __ATOMIC_SEQ_CST);
#endif
}


void impl::atomicNotifyOne(void* value) noexcept {
  WakeByAddressSingle(value);
  syscall(SYS_futex, );
}
void impl::atomicNotifyAll(void* value) noexcept {
  WakeByAddressAll(value);
}


void impl::atomicDeepWaitRaw(const void* atomicAddress, void* compareAddress, size_t addressSize) noexcept {
  WaitOnAddress(const_cast<volatile void*>(atomicAddress), compareAddress, addressSize, INFINITE);
}

bool impl::atomicDeepWaitRaw(const void* atomicAddress, void* compareAddress, size_t addressSize, agt_u32_t timeout) noexcept {
  if (!WaitOnAddress(const_cast<volatile void*>(atomicAddress), compareAddress, addressSize, timeout))
    return GetLastError() != ERROR_TIMEOUT;
  return true;
}




agt_u64_t deadline::getCurrentTimestamp() noexcept {
  LARGE_INTEGER lrgInt;
  QueryPerformanceCounter(&lrgInt);
  return lrgInt.QuadPart;
}


bool ResponseCount::doDeepWait(agt_u32_t& capturedValue, agt_u32_t timeout) const noexcept {
  _InterlockedIncrement(&deepSleepers_);
  BOOL result = WaitOnAddress(&const_cast<volatile agt_u32_t&>(arrivedCount_), &capturedValue, sizeof(agt_u32_t), timeout);
  _InterlockedDecrement(&deepSleepers_);
  if (result)
    capturedValue = fastLoad();
  return result;
}

void ResponseCount::notifyWaiters() noexcept {
  const agt_u32_t waiters = __iso_volatile_load32(&reinterpret_cast<const int&>(deepSleepers_));

  if ( waiters == 0 ) [[likely]] {

  } else if ( waiters == 1 ) {
    WakeByAddressSingle(&arrivedCount_);
  } else {
    WakeByAddressAll(&arrivedCount_);
  }
}
