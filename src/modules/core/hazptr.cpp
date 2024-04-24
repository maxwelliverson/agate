//
// Created by maxwe on 2024-04-15.
//

#include "agate/cast.hpp"

#include "core/impl/hazptr_manager.hpp"
#include "core/impl/spinner.hpp"


#include "core/hazptr.hpp"
#include "core/object.hpp"
#include "core/ctx.hpp"
#include "core/time.hpp"

#include "agate/set.hpp"



#include <utility>
#include <tuple>


AGT_object(external_hazptr_object, extends(agt::hazard_obj)) {
  void* ptr;
  void* userData;
};

namespace {
  
  using manager_t = agt::impl::hazptr_manager;

  using hazptr_set_t = agt::set<const void*>;

  using sized_list_t = agt::hazard_obj_sized_list;

  inline constexpr agt_address_t LockBit = 1;
  
  inline constexpr uint32_t IgnoredShardBits = 8;


  
  inline auto get_shard_index(uintptr_t val) noexcept {
    val ^= val >> 33;
    val *= 0xFF51AFD7ED558CCDULL;
    val ^= val >> 33;
    val *= 0xC4CEB9FE1A85EC53ULL;
    val ^= val >> 33;
    val >>= IgnoredShardBits;
    return (val & (agt::impl::HazptrManagerConcurrency - 1));
  }

  inline auto& get_retired_list(manager_t& manager, uintptr_t val) noexcept {
    return manager.retiredLists[get_shard_index(val)];
  }

  inline auto& get_retired_list(manager_t& manager, agt::hazard_obj* obj) noexcept {
    return manager.retiredLists[get_shard_index(reinterpret_cast<uintptr_t>(obj))];
  }

  std::pair<uint32_t, agt_hazptr_t> try_pop_available_hazptrs(agt_ctx_t ctx, manager_t& manager, uint32_t count) noexcept {
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


  int  get_threshold(manager_t& manager) noexcept {
    const auto hazptrCount = atomic_relaxed_load(manager.hazptrCount);
    return std::max(hazptrCount * manager.thresholdMultiplier, manager.minReclaimThreshold);
  }

  int  get_count_if_past_threshold(agt_ctx_t ctx, manager_t &manager) noexcept {
    int count = atomic_load(manager.count);
    while (get_threshold(manager) <= count) {
      if (atomic_cas(manager.count, count, 0)) {
        atomic_store(manager.dueTime, add_duration(ctx, hwnow(), manager.dueTimeSyncPeriod)); // Maybe precompute this??? SHouldn't matter too much, but hey.
        return count;
      }
    }
    return 0;
  }

  int  get_count_if_past_due_time(agt_ctx_t ctx, manager_t& manager) noexcept {
    const auto time = agt::hwnow();
    const auto dueTime = atomic_load(manager.dueTime);
    if (time < dueTime || !atomic_try_replace(manager.dueTime, dueTime, add_duration(ctx, time, manager.dueTimeSyncPeriod)))
      return 0;
    int count = atomic_exchange(manager.count, 0);
    if (count < 0) {
      atomic_add(manager.count, count);
      return 0;
    }
    return count;
  }




  AGT_forceinline agt::hazard_obj* retired_list_pop_all(agt::hazard_obj*& list) noexcept {
    return atomic_exchange(list, nullptr);
  }

  void                retired_list_push(agt::hazard_obj *& retList, const agt::hazard_obj_list& list) noexcept {
    if (empty(list))
      return;
    AGT_invariant( list.tail != nullptr );

    hazard_obj * prevHead = atomic_load(retList);
    do {
      list.tail->nextHazptrObj = prevHead;
    } while(!atomic_cas(retList, prevHead, list.head));
  }

  agt_hazptr_domain_t try_acquire_domain_list(agt_ctx_t ctx, agt::impl::atomic_hazptr_domain_list& list) noexcept {
    // TODO: Implement agt_hazptr_domains_t

    /*uintptr_t head = atomic_load(list.head);

    if (head != reinterpret_cast<uintptr_t>(nullptr)) { // ignore if null
      if ((head & LockBit) == 0) {
        if (atomic_try_replace(list.head, head, head | LockBit)) {
          atomic_relaxed_store(list.lockOwnerCtx, ctx); // May be relaxed, because ordering doesn't matter. The only case that matters to us is if lockOwnerCtx == ctx, which no other concurrent thread can make happen.
          AGT_invariant( list.reenterCount == 0 );
          ++list.reenterCount;
          return reinterpret_cast<agt_hazptr_domain_t>(head);
        }
      }
      else if (ctx == atomic_relaxed_load(list.lockOwnerCtx)) {
        AGT_invariant( list.reenterCount > 0 );
        ++list.reenterCount;
        return reinterpret_cast<agt_hazptr_domain_t>(head - LockBit);
      }
    }

*/
    return nullptr;
  }


  void organize_objects(const hazptr_set_t& set, agt::hazard_obj * objects, sized_list_t& found, sized_list_t& notfound) noexcept {
    hazard_obj * next;
    for (auto obj = objects; obj; obj = next) {
      next = obj->nextHazptrObj;
      const void* ptr;
      if (auto externalObj = nonnull_object_cast<external_hazptr_object>(obj))
        ptr = externalObj->ptr;
      else
        ptr = obj;
      if (set.contains(ptr))
        push(found, obj);
      else
        push(notfound, obj);
    }
  }

  void reclaim_objects(agt_ctx_t ctx, hazard_obj* head, sized_list_t& children) noexcept {
    hazard_obj * next;
    for (auto obj = head; obj; obj = next) {
      next = obj->nextHazptrObj;
      if (auto userObj = nonnull_object_cast<external_hazptr_object>(obj))
        obj->userFunc(ctx, userObj->ptr, userObj->userData); // find some way to allow users to reclaim 'children'
      else
        obj->reclaimFunc(ctx, obj, children);
    }
  }

  void reclaim_objects_recursive(agt_ctx_t ctx, hazard_obj* obj) noexcept {
    sized_list_t children{};
    while (obj) {
      reclaim_objects(ctx, obj, children);
      obj = children.list.head;
    }
  }


  int reclaim_unused_objects(agt_ctx_t ctx, manager_t& manager, const hazptr_set_t& set, hazard_obj *objects[], bool& isFinished) noexcept {
    sized_list_t notReclaimed{};
    int removedCount = 0;
    for (uint32_t i = 0; i < agt::impl::HazptrManagerConcurrency; ++i) {
      sized_list_t found{}, notfound{}, children{};
      organize_objects(set, objects[i], found, notfound);
      removedCount += notfound.size;
      reclaim_objects(ctx, notfound.list.head, children);
      if (!empty(children))
        isFinished = false;
      removedCount -= children.size;
      splice(notReclaimed, found);
      splice(notReclaimed, children);
    }
    if (isFinished) {
      for (auto& retiredList : manager.retiredLists) {
        if (atomic_load(retiredList) != nullptr) {
          isFinished = false;
          break;
        }
      }
    }
    retired_list_push(manager.retiredLists[0], notReclaimed.list);
    return removedCount;
  }

  bool take_retired_objects(agt_ctx_t ctx, manager_t& manager, agt::hazard_obj * objects[], agt::hazard_obj *& domainObjects, agt_hazptr_domain_t& retiredDomains) noexcept {
    bool success = false;
    for (auto i = 0; i < agt::impl::HazptrManagerConcurrency; ++i) {
      objects[i] = retired_list_pop_all(manager.retiredLists[i]);
      if (objects[i])
        success = true;
    }
    // TODO: Enumerate over each domain, and pop the lists from each.


    if (auto domainHead = try_acquire_domain_list(ctx, manager.domainList)) {
      // TODO: Implement agt_hazptr_domain_t
      AGT_unreachable;
    }
    else {
      domainObjects = nullptr;
      retiredDomains = nullptr;
    }
    return success;
  }


  void do_bulk_hazptr_reclaim(agt_ctx_t ctx, manager_t& manager, int reclaimCount) noexcept {
    AGT_assert( reclaimCount >= 0 );
    bool isFinished;

    do {
      agt::hazard_obj * objects[agt::impl::HazptrManagerConcurrency];
      agt::hazard_obj * domainObjects;
      agt_hazptr_domain_t domains;
      isFinished = true;

      if (take_retired_objects(ctx, manager, objects, domainObjects, domains)) {
        hazptr_set_t hazptrSet;
        auto hz = atomic_load(manager.hazptrs);
        while (hz != nullptr) {
          hazptrSet.insert(atomic_load(hz->ptr));
          hz = hz->next;
        }

        reclaimCount -= reclaim_unused_objects(ctx, manager, hazptrSet, objects, isFinished);
      }

      if (reclaimCount != 0)
        atomic_add(manager.count, reclaimCount);

      reclaimCount = get_count_if_past_due_time(ctx, manager);

    } while(reclaimCount != 0 || !isFinished);

    atomic_sub(manager.bulkReclaimCount, 1);
  }

  agt_hazptr_t create_new_hazptr(agt_ctx_t ctx, manager_t &manager) noexcept {
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


  agt_hazptr_t acquire_hazptrs(agt_ctx_t ctx, manager_t& manager, uint32_t count) noexcept {
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


  void         release_hazptrs(agt_ctx_t ctx, manager_t &manager, agt_hazptr_t head, agt_hazptr_t tail) noexcept {
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


  void         try_reclaim_hazptrs(agt_ctx_t ctx, manager_t &manager) noexcept {
    int count = get_count_if_past_threshold(ctx, manager);
    if (count == 0) {
      count = get_count_if_past_due_time(ctx, manager);
      if (count == 0)
        return;
    }

    atomic_add(manager.bulkReclaimCount, 1);

    // TODO: [async-pool] Perhaps if I implement a background executor or two that specifically
    //       is responsible for performing background tasks required by the library, here we can
    //       send it a message indicating reclaimation should occur, rather than doing it
    //       synchronously.

    do_bulk_hazptr_reclaim(ctx, manager, count);
    /*if (!invoke_reclamation_in_executor(rcount)) {

  }*/
  }



  void         retire_list(agt_ctx_t ctx, manager_t& manager, const sized_list_t& list) noexcept {
    AGT_invariant( !empty(list) );

    std::atomic_thread_fence(std::memory_order_seq_cst); // TODO: Switch to internal fence implementation when done.

    retired_list_push(get_retired_list(manager, list.list.head), list.list);

    // TODO: When domains are implemented, test whether or not it should be pushed to a domain list or the regular list.

    atomic_add(manager.count, list.size);
    try_reclaim_hazptrs(ctx, manager);
  }

  void         retire_single(agt_ctx_t ctx, manager_t& manager, agt::hazard_obj* obj) noexcept {
    AGT_invariant( obj != nullptr );
    AGT_invariant( obj->nextHazptrObj == nullptr );

    std::atomic_thread_fence(std::memory_order_seq_cst);

    retired_list_push(get_retired_list(manager, obj), { obj, obj });

    atomic_add(manager.count, 1);
    try_reclaim_hazptrs(ctx, manager);
  }

  void         fill_cache(agt_ctx_t ctx, manager_t& manager, agt::impl::hazptr_ctx_cache& cache, uint32_t count) noexcept {
    auto hazptr = acquire_hazptrs(ctx, manager, count);

    for (uint32_t i = 0; i < count; ++i) {
      AGT_invariant( hazptr != nullptr );
      auto next = hazptr->nextAvail;
      hazptr->nextAvail = nullptr;
      cache.hazptrs[cache.count++] = hazptr;
      hazptr = next;
    }
    AGT_assert( hazptr == nullptr ); // The list returned by acquire_hazptrs should terminate with a nullptr
  }

  void         evict_cache(agt_ctx_t ctx, manager_t& manager,  agt::impl::hazptr_ctx_cache& cache, uint32_t count) noexcept {
    AGT_invariant( count > 0 );
    AGT_invariant( count <= cache.count );

    agt_hazptr_t head = cache.hazptrs[--cache.count];
    agt_hazptr_t tail = head;

    for (uint32_t i = 1; i < count; ++i) {
      auto hazptr = cache.hazptrs[--cache.count];
      AGT_invariant( hazptr != nullptr );
      hazptr->nextAvail = head;
      head = hazptr;
    }

    release_hazptrs(ctx, manager, head, tail);
  }
}







void agt::impl::destroy_hazptr_manager(agt_instance_t instance) noexcept {
  auto& manager = instance->hazptrManager;
  manager.isShuttingDown = true;
  for (auto& list : manager.retiredLists) {
    auto obj = retired_list_pop_all(list);
    reclaim_objects_recursive(nullptr, obj); // at this point, ctx is null, meaning that the library is shutting down.
  }
  auto hazptr = atomic_load(manager.hazptrs);
  while (hazptr) {
    auto next = hazptr->next;
    release(hazptr);
    hazptr = next;
  }
}

void agt::impl::destroy_hazptr_ctx_cache(agt_ctx_t ctx) noexcept {
  auto& cache = ctx->hazptrCache;
  if (cache.count == 0)
    return;
  evict_cache(ctx, ctx->instance->hazptrManager, cache, cache.count);
}


agt_hazptr_t agt::make_hazptr(agt_ctx_t ctx) noexcept {
  AGT_invariant( ctx != nullptr );

  auto& cache = ctx->hazptrCache;

  if (cache.count > 0)
    return cache.hazptrs[--cache.count];


  return acquire_hazptrs(ctx, ctx->instance->hazptrManager, 1);
}

void agt::make_hazptrs(agt_ctx_t ctx, std::span<agt_hazptr_t> hazptrs) noexcept {
  AGT_invariant( ctx != nullptr );
  AGT_invariant( !hazptrs.empty() );

  auto& cache = ctx->hazptrCache;
  auto cacheSize = cache.count;
  const auto reqSize = hazptrs.size();

  // Should be allow allocation of more hazptrs than can fit into the local cache??

  AGT_invariant( reqSize <= std::size(cache.hazptrs) );

  if (reqSize > cacheSize) [[unlikely]] {
    fill_cache(ctx, ctx->instance->hazptrManager, cache, reqSize - cacheSize);
    cacheSize = reqSize;
  }
  uint32_t cacheOffset = cacheSize - reqSize;

  for (uint32_t i = 0; i < reqSize; ++i)
    hazptrs[i] = cache.hazptrs[cacheOffset + i];

  cache.count = cacheOffset;
}


void agt::drop_hazptr(agt_ctx_t ctx, agt_hazptr_t hazptr) noexcept {
  AGT_invariant( ctx != nullptr );
  AGT_invariant( hazptr != nullptr );

  auto& cache = ctx->hazptrCache;

  reset_hazptr(hazptr);

  if (cache.count < std::size(cache.hazptrs)) [[likely]] {
    cache.hazptrs[cache.count++] = hazptr;
    return;
  }

  release_hazptrs(ctx, ctx->instance->hazptrManager, hazptr, hazptr);
}

void agt::drop_hazptrs(agt_ctx_t ctx, std::span<const agt_hazptr_t> hazptrs) noexcept {
  AGT_invariant( ctx != nullptr );
  AGT_invariant( !hazptrs.empty() );

  auto& cache = ctx->hazptrCache;
  auto count = cache.count;
  const auto hazptrCount = hazptrs.size();
  constexpr static auto Capacity = std::extent_v<decltype(cache.hazptrs), 0>;

  if ((hazptrCount + count) > Capacity) [[unlikely]] {
    evict_cache(ctx, ctx->instance->hazptrManager, cache, (hazptrCount + count) - Capacity);
    count = Capacity - hazptrCount;
  }

  for (uint32_t i = 0; i < hazptrCount; ++i) {
    auto hazptr = hazptrs[i];
    atomic_store(hazptr->ptr, nullptr);
    cache.hazptrs[count + i] = hazptr;
  }

  cache.count = count + hazptrCount;
}


void agt::impl::do_retire_hazard(agt_ctx_t ctx, agt::hazard_obj* hazard) noexcept {
  retire_single(ctx, ctx->instance->hazptrManager, hazard);
}


void agt::hazard_retire_list::flush() noexcept {
  if (empty(m_list))
    return;

  auto ctx = m_ctx;
  AGT_resolve_ctx(ctx);
  AGT_assert( m_domain == AGT_DEFAULT_HAZPTR_DOMAIN );
  retire_list(ctx, ctx->instance->hazptrManager, m_list);
}


void agt::retire_user_hazard(agt_ctx_t ctx, void* obj, agt_hazptr_domain_t domain, agt_hazptr_retire_func_t retireFunc, void* userData) noexcept {

  AGT_assert( domain == AGT_DEFAULT_HAZPTR_DOMAIN );

  // TODO: If a non-default domain is specified,

  auto hazptr = alloc<external_hazptr_object>(ctx);
  hazptr->ptr      = obj;
  hazptr->userFunc = retireFunc;
  hazptr->userData = userData;
  hazptr->domainTag = 0;
  hazptr->nextHazptrObj = nullptr;

  retire_single(ctx, ctx->instance->hazptrManager, hazptr);
}