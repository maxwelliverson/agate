//
// Created by maxwe on 2024-04-01.
//

#include "monitor_list.hpp"

#include "core/object.hpp"
#include "core/instance.hpp"
#include "core/exec.hpp"

#include <bit>

#define NOMINMAX
#include <Windows.h>


namespace {
  // NOTE: Please monitor and benchmark this.
  // The question is whether or not this gives an even-ish distribution modulo
  // the number of segments. I've done some very basic tests that show that this
  // *should* work (assuming most addresses monitored will be aligned to 8 or 16
  // bytes), but it hasn't been tested with actual addresses produces in realistic
  // usage scenarios.
  inline uint64_t hash_address(const void* addr) noexcept {
    // Assume there will be very little variation between addresses in the low 3-4 bits (due to alignment)
    return (reinterpret_cast<uintptr_t>(addr) >> 4) * 37u;
  }

  // MAKE SURE the return value isn't copied.
  inline agt::impl::spinlock_t& lock(agt_ctx_t ctx, agt::impl::monitor_list& list, uint64_t hash) noexcept {
    auto& lck = list.segmentLocks[hash & list.segmentMask];
    lock(ctx, lck);
    return lck;
  }

  inline agt::impl::monitored_address* find_entry(uint64_t hash, agt::impl::monitor_list& list, const void* addr) noexcept {

    for (agt::impl::monitored_address* entry = list.table[hash & list.tableMask]; entry != nullptr; entry = entry->next) {
      assert( !test(entry->flags, agt::impl::eMonitoredAddressIsShared) && "Shared address monitoring is not yet implemented.");
      if (entry->address == addr) {
        atomic_relaxed_increment(entry->activeRefCount);
        return entry;
      }

    }
    return nullptr;
  }
}


void agt::impl::init_monitor_list_local(agt_instance_t instance, monitor_list& list, size_t tableSize, uint32_t concurrencyFactor) noexcept {
  AGT_invariant( is_pow2(tableSize) );
  const uint32_t trueConcurrencyFactor = std::max(std::bit_ceil(concurrencyFactor), 64u);

  const auto tableMemSize = tableSize * sizeof(void*);
  const auto segmentListMemSize = trueConcurrencyFactor * AGT_CACHE_LINE; // Let each segment lock live on its own cacheline. Only one of these arrays should exist per process, so this doesn't waste too much memory in the grand scheme of things, but the speedup is *significant*.

  auto tableMem = instance_mem_alloc(instance, tableMemSize, tableMemSize);
  std::memset(tableMem, 0, tableMemSize);

  auto segmentListMem = instance_mem_alloc(instance, segmentListMemSize, AGT_CACHE_LINE);


  for (uint32_t i = 0; i < trueConcurrencyFactor; ++i) {
    auto spinlock = reinterpret_cast<spinlock_t*>(static_cast<char*>(segmentListMem) + (i * AGT_CACHE_LINE));
    spinlock->value = SpinlockUnlockedValue;
  }

  list.tableSize    = static_cast<agt_u32_t>(tableSize);
  list.segmentCount = trueConcurrencyFactor;
  list.segmentMask  = trueConcurrencyFactor - 1;
  list.segmentLocks = static_cast<spinlock_t*>(segmentListMem);
  list.tableMask    = tableSize - 1;
  list.table        = static_cast<monitored_address**>(tableMem);
}

void agt::impl::destroy_monitor_list_local(agt_instance_t instance, monitor_list& list) noexcept {
  const auto segmentListSize = list.segmentCount * AGT_CACHE_LINE;
  const auto tableSize = list.tableSize * sizeof(void*);
  instance_mem_free(instance, list.segmentLocks, segmentListSize, segmentListSize);
  instance_mem_free(instance, list.table, tableSize, tableSize);
  std::memset(&list, 0, sizeof list);
}

void agt::impl::init_monitor(agt_ctx_t ctx, monitor_waiter& waiter, address_predicate_t predicate, void* userData) noexcept {
  waiter.execCtxData = ctx->ctxexec;
  waiter.task = static_cast<const agt_ctxexec_header_t*>(ctx->ctxexec)->activeTask;
  waiter.predicate = predicate;
  waiter.predicateUserData = userData;
  waiter.isEnqueued = AGT_FALSE;
  set_flags(waiter.flags, eMonitorWaiterHasPredicateCallback);
}

void agt::impl::init_monitor(agt_ctx_t ctx, monitor_waiter& waiter, size_t dataSize, agt_u64_t cmpValue) noexcept {
  waiter.execCtxData = ctx->ctxexec;
  waiter.task = static_cast<const agt_ctxexec_header_t*>(ctx->ctxexec)->activeTask;
  AGT_invariant( is_pow2(dataSize) && dataSize <= 8 );
  waiter.valueSize = dataSize;
  waiter.cmpValue  = cmpValue;
  waiter.isEnqueued = AGT_FALSE;
  reset(waiter.flags, eMonitorWaiterHasPredicateCallback);
}

void agt::impl::attach_monitor(agt_ctx_t ctx, monitor_list& list, const void* addr, monitor_waiter & exec) noexcept {
  const auto hash = hash_address(addr);
  auto& seg = ::lock(ctx, list, hash);

  const auto tableIndex = hash & list.tableMask;
  auto& tableEntry = list.table[tableIndex];
  monitored_address* entry;

  for (entry = tableEntry; entry != nullptr; entry = entry->next) {
      assert( !test(entry->flags, eMonitoredAddressIsShared) && "Shared address monitoring is not yet implemented.");
      if (entry->address == addr)
        goto add_to_list;
  }

  entry = alloc<monitored_address>(ctx);
  entry->next = tableEntry;
  entry->prevNext = &tableEntry;
  tableEntry = entry;
  entry->address = addr;
  entry->segmentLock = &seg;
  entry->activeRefCount = 1;
  task_queue_init(entry->waiterQueue);

  std::atomic_signal_fence(std::memory_order_release);

add_to_list:

  exec.address = entry;
  exec.isEnqueued = AGT_TRUE;

  insert(ctx, entry->waiterQueue, exec);

  unlock(seg);
}


bool agt::impl::remove_monitor(agt_ctx_t ctx, monitor_waiter & exec) noexcept {

  monitored_address* entryToBeReleased = nullptr;

  // Do an early check, before acquiring the lock.
  if (atomic_relaxed_load(exec.isEnqueued) == AGT_FALSE)
    return false;

  auto& spinlock = *exec.address->segmentLock;

  lock(ctx, spinlock);

  // Check once more whether exec has already been unblocked.
  // Note that isEnqueued may only be set to false while the segment spinlock is held, so
  // following this check, we know that we will be the ones to unlock!
  if (exec.isEnqueued == AGT_FALSE) {
    unlock(spinlock);
    return false;
  }

  auto entry = exec.address;

  if (remove(entry->waiterQueue, exec)) {
    exec.isEnqueued = AGT_FALSE;

    if (entry->waiterQueue.size == 0) {
      *entry->prevNext = entry->next;
      entry->next->prevNext = entry->prevNext;
      entryToBeReleased = entry;
    }
  }

  unlock(spinlock);

  if (entryToBeReleased)
    release(entryToBeReleased);

  return true;
}


namespace {

  struct unblock_data {
    agt_ctx_t   ctx;
    const void* addr;
  };

  bool unblock_monitor_blocked_exec(agt_ctx_t ctx, agt::blocked_task* task_, void* userData) noexcept {
    const auto waiter = static_cast<agt::impl::monitor_waiter *>(task_);
    auto data = static_cast<unblock_data*>(userData);
    bool shouldUnblock;
    if (test(waiter->flags, agt::impl::eMonitorWaiterHasPredicateCallback)) {
      shouldUnblock = waiter->predicate(data->ctx, data->addr, waiter->predicateUserData);
    }
    else {
      switch (waiter->valueSize) {
        case 1:
          shouldUnblock = atomic_load(*static_cast<uint8_t*>(userData)) != static_cast<uint8_t>(waiter->cmpValue);
          break;
        case 2:
          shouldUnblock = atomic_load(*static_cast<uint16_t*>(userData)) != static_cast<uint16_t>(waiter->cmpValue);
          break;
        case 4:
          shouldUnblock = atomic_load(*static_cast<uint32_t*>(userData)) != static_cast<uint32_t>(waiter->cmpValue);
          break;
        case 8:
          shouldUnblock = atomic_load(*static_cast<uint64_t*>(userData)) != static_cast<uint64_t>(waiter->cmpValue);
          break;
      }
    }
    if (shouldUnblock) {
      waiter->pfnResumeTask(ctx, waiter->execCtxData, waiter->task);
      waiter->isEnqueued = false;
      std::atomic_thread_fence(std::memory_order_release);
    }
    return shouldUnblock;
  }
}

void agt::impl::wake_one_on_monitored_address(agt_ctx_t ctx, monitor_list& list, const void* addr) noexcept {
  const auto hash = hash_address(addr);
  auto& seg = ::lock(ctx, list, hash);

  monitored_address* entryToBeReleased = nullptr;

  if (auto entry = find_entry(hash, list, addr)) {
    unblock_data unblockData{
        ctx,
        addr
    };
    if (wake_one(ctx, entry->waiterQueue, unblock_monitor_blocked_exec, &unblockData)) {
      *entry->prevNext = entry->next;
      entry->next->prevNext = entry->prevNext;
      entryToBeReleased = entry;
    }
  }

  unlock(seg);

  if (entryToBeReleased)
    release(entryToBeReleased);
}

void agt::impl::wake_all_on_monitored_address(agt_ctx_t ctx, monitor_list& list, const void* addr) noexcept {
  const auto hash = hash_address(addr);
  auto& seg = ::lock(ctx, list, hash);

  monitored_address* entryToBeReleased = nullptr;

  if (auto entry = find_entry(hash, list, addr)) {
    unblock_data unblockData{
        ctx,
        addr
    };
    if (wake_all(ctx, entry->waiterQueue, unblock_monitor_blocked_exec, &unblockData)) {
      *entry->prevNext = entry->next;
      entry->next->prevNext = entry->prevNext;
      entryToBeReleased = entry;
    }
  }

  unlock(seg);

  if (entryToBeReleased)
    release(entryToBeReleased);
}
