//
// Created by maxwe on 2024-04-15.
//

#include "hazptr_manager.hpp"
#include "core/hazptr.hpp"
#include "core/object.hpp"
#include "core/ctx.hpp"
#include "core/time.hpp"

#include "spinner.hpp"

#include <utility>
#include <tuple>

namespace {

  inline constexpr agt_address_t LockBit = 1;

  std::pair<uint32_t, agt_hazptr_t> try_pop_available_hazptrs(agt_ctx_t ctx, agt::hazptr_manager& manager, uint32_t count) noexcept {
    AGT_invariant( count >= 1 );

    impl::spinner spinner;

    while (true) {
      uintptr_t avail = atomic_load(manager.availList);
      if (avail == reinterpret_cast<uintptr_t>(nullptr))
        return { 0, nullptr };

      if (avail & LockBit) {
        spinner.spin(ctx);
        continue;
      }

      if (atomic_try_replace(manager.availList, avail, avail | LockBit)) {

        auto head = reinterpret_cast<agt_hazptr_t>(avail);
        auto tail = head;
        uint32_t poppedCount = 1;
        auto next = tail->nextAvail;
        while (next && (poppedCount < count)) {
          AGT_assert( (((uintptr_t)next) & LockBit) == 0 ); // next should *not* be locked lol
          tail = next;
          next = tail->nextAvail;
          ++poppedCount;
        }
        const auto newAvailHead = reinterpret_cast<uintptr_t>(next);
        AGT_assert( (newAvailHead & LockBit) == 0 );
        atomic_store(manager.availList, newAvailHead);
        tail->nextAvail = nullptr;
        AGT_assert( 0 < poppedCount && poppedCount <= count );
        return { poppedCount, head };
      }
    }
  }

  bool connection_between(agt_hazptr_t head, agt_hazptr_t tail) noexcept {
    while (head->nextAvail != nullptr) {
      if (head->nextAvail == tail)
        return true;
      head = head->nextAvail;
    }
    return false;
  }


  int  get_threshold(agt::hazptr_manager& manager) noexcept {
    const auto hazptrCount = atomic_relaxed_load(manager.hazptrCount);
    return std::max(hazptrCount * manager.thresholdMultiplier, manager.minReclaimThreshold);
  }

  int  get_count_if_past_threshold(agt_ctx_t ctx, agt::hazptr_manager &manager) noexcept {
    int count = atomic_load(manager.count);
    while (get_threshold(manager) <= count) {
      if (atomic_cas(manager.count, count, 0)) {
        atomic_store(manager.dueTime, add_duration(ctx, hwnow(), manager.dueTimeSyncPeriod)); // Maybe precompute this??? SHouldn't matter too much, but hey.
        return count;
      }
    }
    return 0;
  }

  int  get_count_if_past_due_time(agt_ctx_t ctx, agt::hazptr_manager& manager) noexcept {
    const auto time = agt::hwnow();
    const auto dueTime = atomic_load(manager.dueTime);
    if (time < dueTime || !atomic_try_replace(manager.dueTime, dueTime, add_duration(ctx, time, manager.dueTimeSyncPeriod)))
      return 0;
    int count = atomic_exchange(manager.count, 0);
    if (count < 0) {
      atomic_exchange_add(manager.count, count);
      add_count(count);
      return 0;
    }
    return count;
  }

  void do_bulk_hazptr_reclaim(agt_ctx_t ctx, agt::hazptr_manager &manager, int reclaimCount) noexcept {

  }
}

agt_hazptr_t agt::create_new_hazptr(agt_ctx_t ctx, agt::hazptr_manager &manager) noexcept {
  auto ptr = alloc<agt_hazptr_st>(ctx);
  ptr->nextAvail = nullptr;
  ptr->ptr       = nullptr;
  ptr->bits      = 0;
  auto head = atomic_load(manager.hazptrs);
  do {
    ptr->next = head;
  } while(!atomic_cas(manager.hazptrs, head, ptr));
  atomic_increment(manager.hazptrCount);
  return ptr;
}

agt_hazptr_t agt::acquire_hazptrs(agt_ctx_t ctx, hazptr_manager& manager, uint32_t count) noexcept {
  AGT_invariant( count >= 1 );
  auto [ n, head ] = try_pop_available_hazptrs(ctx, manager, count);
  for (; n < count; ++n) {
    auto ptr = create_new_hazptr(ctx, manager);
    ptr->nextAvail = head;
    head = ptr;
  }
  AGT_invariant( head != nullptr );
  return head;
}

void agt::release_hazptrs(agt_ctx_t ctx, agt::hazptr_manager &manager, agt_hazptr_t head, agt_hazptr_t tail) noexcept {
  // push the linked list from head to tail to the available list held by manager.
  AGT_invariant( head != nullptr );
  AGT_invariant( tail != nullptr );
  AGT_invariant( tail->nextAvail == nullptr );
  AGT_assert( connection_between(head, tail) );

  const auto newHead = reinterpret_cast<uintptr_t>(head);
  AGT_invariant( (newHead & LockBit) == 0 );

  impl::spinner spinner;

  do {
    uintptr_t avail = atomic_load(manager.availList);

    if (avail & LockBit) {
      spinner.spin(ctx);
      continue;
    }

    tail->nextAvail = reinterpret_cast<agt_hazptr_t>(avail);

    if (atomic_cas(manager.availList, avail, newHead))
      return;

  } while(true);
}

void agt::try_reclaim_hazptrs(agt_ctx_t ctx, agt::hazptr_manager &manager) noexcept {
  int count = get_count_if_past_threshold(ctx, manager);
  if (count == 0) {
    count = check_due_time();
    if (count == 0)
      return;
  }
  inc_num_bulk_reclaims();

  if (!invoke_reclamation_in_executor(rcount)) {
    do_bulk_hazptr_reclaim(ctx, manager, count);
  }
}