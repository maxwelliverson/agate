//
// Created by maxwe on 2024-04-01.
//

#ifndef AGATE_SPINLOCK_HPP
#define AGATE_SPINLOCK_HPP

#include "config.hpp"

#include "agate/atomic.hpp"

namespace agt::impl {

  struct spinlock_t {
    uint32_t value;
  };

  inline constexpr uint32_t SpinlockLockedValue   = static_cast<uint32_t>(-1);
  inline constexpr uint32_t SpinlockUnlockedValue = 0;

  void spinlock_yield() noexcept;

  inline static void lock(spinlock_t& spinlock) noexcept {
    uint32_t backoff = 0;
    do {
      uint32_t prevValue = atomicExchange(spinlock.value, SpinlockLockedValue);
      if (prevValue == SpinlockUnlockedValue) [[likely]]
        return;
      DUFFS_MACHINE_EX(backoff,
                       spinlock_yield();
                       );
    } while(true);
  }

  AGT_forceinline static void unlock(spinlock_t& spinlock) noexcept {
    atomicStore(spinlock.value, SpinlockUnlockedValue);
  }
}

#endif//AGATE_SPINLOCK_HPP
