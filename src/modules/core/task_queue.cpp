//
// Created by maxwe on 2024-04-08.
//

#include "core/task_queue.hpp"

#include "core/object.hpp"
#include "core/ctx.hpp"

#include "agate/on_scope_exit.hpp"
#include "agate/vector.hpp"

#include "hazptr.hpp"


AGT_object(task_queue_vec) {
  agt_u32_t          capacity;
  agt::blocked_task* tasks[];
};


namespace agt {
  AGT_object(spsc_signal_waiter_vec, extends(agt::hazard_obj)) {
    agt_i32_t           capacity;
    agt_i32_t           count; // signed for sake of consistency with use of signed values in the wake functions.
    agt::signal_waiter* waiters[];
  };
}



namespace {
  inline constexpr uintptr_t SetByReceiverBit = 0x2;
  inline constexpr uintptr_t ValueIsVecBit    = 0x1;
  inline constexpr uintptr_t VecMask          = ~(SetByReceiverBit | ValueIsVecBit);
  inline constexpr uintptr_t IsMultiValueBit  = 0x1;
  inline constexpr uint32_t  InitialMultiSize = 2;

  AGT_forceinline task_queue_vec* try_as_vec(uintptr_t val) noexcept {
    return (val & IsMultiValueBit) ? reinterpret_cast<task_queue_vec*>(val & ~IsMultiValueBit) : nullptr;
  }

  AGT_forceinline agt::blocked_task* as_task(uintptr_t val) noexcept {
    return reinterpret_cast<agt::blocked_task*>(val);
  }

  AGT_forceinline void assign(uintptr_t& queue, agt::blocked_task* task) noexcept {
    queue = reinterpret_cast<uintptr_t>(task);
  }

  AGT_forceinline void assign(uintptr_t& queue, task_queue_vec* vec) noexcept {
    queue = (reinterpret_cast<uintptr_t>(vec) | IsMultiValueBit);
  }

  task_queue_vec*      alloc_vec(agt_ctx_t ctx, uint32_t capacity) noexcept {
    auto vec = alloc_dyn<task_queue_vec>(ctx, sizeof(task_queue_vec) + (capacity * sizeof(void*)));
    vec->capacity = capacity;
    return vec;
  }


  AGT_forceinline task_queue atomic_load_tq(const task_queue& src) noexcept {
    union {
      uint64_t   u64[2];
      task_queue queue;
    };
    const auto srcPtr = reinterpret_cast<const uint64_t*>(&src);
    u64[0] = agt::atomic_relaxed_load(srcPtr[0]);
    u64[1] = agt::atomic_relaxed_load(srcPtr[1]);
    return queue;
  }

  AGT_forceinline bool atomic_update_counter(uint32_t& target, uint32_t newValue) noexcept {
    uint32_t prev = atomic_relaxed_load(target);
    do {
      if (newValue < prev) {
        if (prev - newValue < INT_MAX) [[likely]] // this should almost never be false, however it is here so that the counter can roll over without issue.
          return false;
      }
    } while(!atomic_cas(target, prev, newValue));
    return true;
  }

  AGT_forceinline void init_waiter(agt_ctx_t ctx, signal_waiter& waiter) noexcept {
    waiter.resume  = ctx->uexecVPtr->resume;
    waiter.ctxexec = ctx->ctxexec;
    waiter.task    = *static_cast<const agt_utask_t*>(waiter.ctxexec);
  }
}





void agt::blocked_task_init(agt_ctx_t ctx, blocked_task& task) noexcept {
  AGT_invariant( ctx != nullptr );
  task.next = nullptr;
  task.execCtxData   = ctx->ctxexec;
  task.task          = static_cast<agt_ctxexec_header_t*>(ctx->ctxexec)->activeTask;
  task.pfnResumeTask = ctx->uexecVPtr->resume;
}


bool agt::impl::task_queue_contains(const agt::unsafe_task_queue& queue, const agt::blocked_task &task) noexcept {

  if (queue.size == 0)
    return false;

  if (auto vec = try_as_vec(queue.value)) {
    for (uint32_t i = 0; i < queue.size; ++i) {
      if (vec->tasks[i] == &task) {
        return true;
      }
    }
    return false;
  }

  AGT_invariant(queue.size == 1);
  return as_task(queue.value) == &task;
}

void agt::impl::task_queue_insert(agt_ctx_t ctx, unsafe_task_queue& queue, blocked_task& task) noexcept {
  if (queue.value == 0) [[likely]] {
    assign(queue.value, &task);
  }
  else {
    task_queue_vec* nextVec;
    if (const auto vec = try_as_vec(queue.value)) {
      if (queue.size == vec->capacity) {
        nextVec = alloc_vec(ctx, vec->capacity * 2);
        for (uint32_t i = 0; i < queue.size; ++i) {
          nextVec->tasks[i] = vec->tasks[i];
        }
        release(vec);
      }
      else {
        nextVec = vec;
      }
    }
    else {
      nextVec = alloc_vec(ctx, InitialMultiSize);

      nextVec->capacity  = InitialMultiSize;
      nextVec->tasks[0] = as_task(queue.value);
    }

    nextVec->tasks[queue.size] = &task;
    assign(queue.value, nextVec);
  }
}




bool agt::remove(unsafe_task_queue &queue, blocked_task &task) noexcept {
  if (queue.size != 0) {
    if (auto vec = try_as_vec(queue.value)) {
      for (uint32_t i = 0; i < queue.size; ++i) {
        if (vec->tasks[i] == &task) {
          queue.size -= 1;
          // Shift the following entries down
          while (i < queue.size) {
            vec->tasks[i] = vec->tasks[i + 1]; // it's okay that this technically goes 1 past queue.size, cause queue.size was just decremented, so we're taking the previous end.
            ++i;
          }
          if (queue.size == 0) {
            release(vec);
            queue.value = 0;
          }
          return true;
        }
      }
    }
    else {
      AGT_invariant(queue.size == 1);
      if (as_task(queue.value) == &task) {
        queue.value = 0;
        queue.size  = 0;
        return true;
      }
    }
  }

  return false;
}

void agt::wake_one(agt_ctx_t ctx, unsafe_task_queue &queue) noexcept {
  if (queue.size == 0)
    return;

  blocked_task* task;

  if (auto vec = try_as_vec(queue.value)) {
    task = vec->tasks[0];
    queue.size -= 1;
    for (uint32_t i = 0; i < queue.size; ++i) {
      vec->tasks[i] = vec->tasks[i + 1];
    }
    if (queue.size == 0) {
      release(vec);
      queue.value = 0;
    }
  }
  else {
    AGT_invariant( queue.size == 1 );
    task = as_task(queue.value);
    queue.value = 0;
    queue.size = 0;
  }

  task->pfnResumeTask(ctx, task->execCtxData, task->task);
}

void agt::wake_all(agt_ctx_t ctx, unsafe_task_queue &queue) noexcept {

  if (queue.size == 0)
    return;

  AGT_resolve_ctx(ctx);

  if (auto vec = try_as_vec(queue.value)) {
    for (uint32_t i = 0; i < queue.size; ++i) {
      const auto task = vec->tasks[i];
      task->pfnResumeTask(ctx, task->execCtxData, task->task);
    }
    release(vec);
  }
  else {
    AGT_invariant( queue.size == 1 );
    const auto task = as_task(queue.value);
    task->pfnResumeTask(ctx, task->execCtxData, task->task);
  }

  queue.value = 0;
  queue.size  = 0;
}

bool agt::wake_one(agt_ctx_t ctx, unsafe_task_queue &queue, task_queue_predicate_t predicate, void *userData) noexcept {
  if (queue.size > 0) {
    if (auto vec = try_as_vec(queue.value)) {
      for (uint32_t i = 0; i < queue.size; ++i) {
        if (predicate(ctx, vec->tasks[i], userData)) {
          queue.size -= 1;
          if (queue.size == 0) {
            release(vec);
            queue.value = 0;
            return true;
          }

          // Shift everything down 1, as in unsafe_unblock_local
          for (; i < queue.size; ++i) {
            vec->tasks[i] = vec->tasks[i + 1];
          }
        }
      }

    }
    else {
      AGT_invariant(queue.size == 1);
      if (predicate(ctx, as_task(queue.value), userData)) {
        queue.value = 0;
        queue.size = 0;
        return true;
      }
    }
  }

  return false;
}

bool agt::wake_all(agt_ctx_t ctx, unsafe_task_queue &queue, task_queue_predicate_t predicate, void *userData) noexcept {
  if (queue.size == 0)
    return true;

  task_queue_vec* prevVec = nullptr;
  vector<blocked_task*, 8> requeueList{};

  if (auto vec = try_as_vec(queue.value)) {
    for (uint32_t i = 0; i < queue.size; ++i) {
      const auto task = vec->tasks[i];
      if (!predicate(ctx, task, userData))
        requeueList.push_back(task);
    }
    if (requeueList.size() > 1)
      prevVec = vec;
    else
      release(vec);
  }
  else {
    AGT_invariant( queue.size == 1 );
    const auto task = as_task(queue.value);
    if (!predicate(ctx, task, userData))
      requeueList.push_back(task);
  }


  if (requeueList.empty()) [[likely]] {
    queue.value = 0;
    queue.size = 0;
    return true;
  }


  if (requeueList.size() == 1) [[likely]] {
    AGT_invariant( prevVec == nullptr );
    assign(queue.value, requeueList.front());
    queue.size = 1;
    return false;
  }


  // Requeue multiple. Thankfully, unlike the safe version, we don't have to worry about others having appended to our list in the meanwhile.


  AGT_invariant( prevVec != nullptr );

  queue.size = requeueList.size();

  for (auto i = 0; i < queue.size; ++i)
    prevVec->tasks[i] = requeueList[i];

  return false;
}




void agt::insert(agt_ctx_t ctx, task_queue &queue, blocked_task &task) noexcept {
  task_queue prev, next;

  prev.task = atomic_load(queue.task);
  // prev = atomic_load(queue);

  task_queue_vec* vec;
  task_queue_vec* prevVec;
  task_queue_vec* nextVec;

  /*if (prev.task == nullptr) [[likely]] {
    do [[unlikely]] { // the unlikely attribute applies to the loop condition.
        AGT_invariant( prev.size == 0 );
        next.epoch          = prev.epoch + 1;
        next.activeRefCount = prev.activeRefCount;
        next.size           = 1;

        assign(next.value, &task);
        if (atomic_cas(queue, prev, next)) // successfully queued a single task
          return;
        // if the CAS fails but prev.value is still 0, try again. This *may* happen, but is unlikely
      } while (prev.value == 0);
  }

  if (prev.value == 0) [[likely]] { // generally the most likely scenario is where there aren't any waiters yet.
    do [[unlikely]] { // the unlikely attribute applies to the loop condition.
      AGT_invariant( prev.size == 0 );
      next.epoch          = prev.epoch + 1;
      next.activeRefCount = prev.activeRefCount;
      next.size           = 1;
      assign(next.value, &task);
      if (atomic_cas(queue, prev, next)) // successfully queued a single task
        return;
      // if the CAS fails but prev.value is still 0, try again. This *may* happen, but is unlikely
    } while (prev.value == 0);
  }


  do {
    if ((vec = try_as_vec(prev.value))) {
      next = prev;
      ++next.activeRefCount;
      if (!atomic_cas(queue, prev, next))
        continue;

    }

    if (atomic_cas(queue, prev, next))
      break;
  } while(true);*/



  // if (vec && prev.activeRefCount == 1)


}

bool agt::remove(task_queue &queue, blocked_task &task) noexcept {
  return false;
}
void agt::wake_one(agt_ctx_t ctx, task_queue &queue) noexcept {
}
void agt::wake_all(agt_ctx_t ctx, task_queue &queue) noexcept {
}
bool agt::wake_one(agt_ctx_t ctx, task_queue &queue, task_queue_predicate_t predicate, void *userData) noexcept {
  return false;
}
bool agt::wake_all(agt_ctx_t ctx, task_queue &queue, task_queue_predicate_t predicate, void *userData) noexcept {
  return false;
}



/*bool agt::remove(unsafe_signal_task_queue &queue, blocked_task &task) noexcept {
  return false;
}*/
void agt::wake(agt_ctx_t ctx, unsafe_signal_task_queue &queue, uint32_t n) noexcept {

  AGT_invariant(n > 0); // Callers should guarantee that n is never 0

  agt_i32_t newSize = queue.size - static_cast<agt_i32_t>(n);

  // If newSize <= 0, all currently waiting tasks will be woken

  const bool isCleared = newSize <= 0;

  if (auto vec = try_as_vec(queue.value)) {

    const auto toIndex = isCleared ? queue.size : (queue.size - newSize);
    uint32_t i;
    for (i = 0; i < toIndex; ++i) {
      const auto task = vec->tasks[i];
      task->pfnResumeTask(ctx, task->execCtxData, task->task);
    }

    if (!isCleared) {
      for (uint32_t j = 0; j < newSize; ++j, ++i)
        vec->tasks[j] = vec->tasks[i];
    }
    else {
      release(vec);
      queue.value = 0;
    }

  }
  else {
    AGT_invariant( queue.size == 1 );
    // Recall that n > 0, therefore this single task is guaranteed to be woken.
    const auto task = as_task(queue.value);
    task->pfnResumeTask(ctx, task->execCtxData, task->task);
    queue.value = 0;
  }

  queue.size = newSize;
}



bool agt::insert(signal_task_queue &queue, blocked_task &task) noexcept {
  auto newSize= atomic_increment(queue.size);

  if (newSize <= 0)
    return false;

  task.next = nullptr;
  auto tail = atomic_exchange(queue.tail, &task.next);
  atomic_store(*tail, &task);

  return true;

  /*
  msg.next() = nullptr;


  s->tail = &msg.next();
  std::atomic_thread_fence(std::memory_order_acq_rel);
  atomic_store(tail->c_ref(), message);


  task_queue prev, next;
  prev = atomic_load(queue);

  task_queue_vec* vec;
  task_queue_vec* prevVec;
  task_queue_vec* nextVec;

  if (prev.task == nullptr) [[likely]] {
    do [[unlikely]] { // the unlikely attribute applies to the loop condition.
        AGT_invariant( prev.size == 0 );
        next.epoch          = prev.epoch + 1;
        next.activeRefCount = prev.activeRefCount;
        next.size           = 1;

        assign(next.value, &task);
        if (atomic_cas(queue, prev, next)) // successfully queued a single task
          return;
        // if the CAS fails but prev.value is still 0, try again. This *may* happen, but is unlikely
      } while (prev.value == 0);
  }

  if (prev.value == 0) [[likely]] { // generally the most likely scenario is where there aren't any waiters yet.
    do [[unlikely]] { // the unlikely attribute applies to the loop condition.
        AGT_invariant( prev.size == 0 );
        next.epoch          = prev.epoch + 1;
        next.activeRefCount = prev.activeRefCount;
        next.size           = 1;
        assign(next.value, &task);
        if (atomic_cas(queue, prev, next)) // successfully queued a single task
          return;
        // if the CAS fails but prev.value is still 0, try again. This *may* happen, but is unlikely
      } while (prev.value == 0);
  }


  do {
    if ((vec = try_as_vec(prev.value))) {
      next = prev;
      ++next.activeRefCount;
      if (!atomic_cas(queue, prev, next))
        continue;

    }

    if (atomic_cas(queue, prev, next))
      break;
  } while(true);
   */
}

bool agt::remove(signal_task_queue &queue, blocked_task &task) noexcept {


  return false;
}

void agt::wake(signal_task_queue &queue, int32_t n) noexcept {

  AGT_invariant( n > 0 );

  /*const auto prevSize = atomic_exchange_add(queue.size, -n);
  const auto newSize = prevSize - n;



  if (prevSize <= 0) // there were no waiters
    return;

  if (newSize <= 0) { // ie. wake everything, remove all entries from queue
    auto head = atomic_exchange(queue.head, nullptr);
    blocked_task* tail;
    atomic_store(queue.tail, &queue.head);

    for (uint32_t i = prevSize; i > 0; --i) {
      auto next = head->next;
    }
  }
  else { // wake only some entries, walk one by one. This is the slowest, but should also be the rarest.

  }

  // newSize is greater than or equal to zero, meaning it was previously


  blocked_task* head = atomic_relaxed_load(queue.head);

  do {
    while (head == nullptr) [[unlikely]] {
      spin();
      head = atomic_relaxed_load(queue.head);
    }
  } while (!atomic_cas(queue.head, head, head->next));



  atomic_store(r->head, message->next);
  auto newTail = &message->next;
  atomic_try_replace(*r->pTail, newTail, &r->head);*/
}



namespace {

  void vec_erase(agt_ctx_t ctx, agt::spsc_signal_waiter_vec*& newVec, agt::spsc_signal_waiter_vec* prevVec, int32_t index) noexcept {
    AGT_invariant( prevVec != nullptr );
    const auto newSize = prevVec->count - 1;
    AGT_invariant( newSize > 1 );

    if (!newVec) [[likely]] {
      newVec = alloc_dyn<agt::spsc_signal_waiter_vec>(ctx, sizeof(agt::spsc_signal_waiter_vec) + (sizeof(void*) * newSize));
      newVec->capacity = static_cast<agt_i32_t>(newSize);
    }
    AGT_invariant( newSize <= newVec->capacity );

    newVec->count = newSize;

    int32_t i;
    for (i = 0; i < index; ++i)
      newVec->waiters[i] = prevVec->waiters[i];
    for (; i < newSize; ++i)
      newVec->waiters[i] = prevVec->waiters[i + 1];
  }


  // This is safe from any weird shit because there is a maximum of 1 thread calling remove concurrently.
  // The potential scenarios are:
  //   a) This hasn't been removed, and there's no contention. Removal succeeds.
  //   b) Another thread tries waking this before the removal can complete, in which case it fails.
  bool remove_waiter(agt_ctx_t ctx, spsc_signal_queue& queue, signal_waiter& waiter) noexcept {

    agt::spsc_signal_waiter_vec* nextVec = nullptr;
    agt::spsc_signal_waiter_vec* freeVec = nullptr;
    agt_hazptr_t hazptr = nullptr;

    on_scope_exit {
        if (freeVec)
          release(freeVec);
        if (hazptr)
          drop_hazptr(ctx, hazptr);
    };

    auto bits = atomic_load(queue.value);

    do {
      uintptr_t nextBits;
      freeVec = nextVec;

      if ((bits & ValueIsVecBit) == 0) [[likely]] {
        if (bits == 0 || bits != reinterpret_cast<uintptr_t>(&waiter)) [[unlikely]]
          return false;

        nextBits = 0;
      }
      else {
        const auto vec = reinterpret_cast<agt::spsc_signal_waiter_vec*>(bits & VecMask);
        AGT_invariant( vec != nullptr );
        if (hazptr == nullptr) [[likely]]
          hazptr = make_hazptr(ctx);
        set_hazptr(hazptr, vec);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        auto newVec = atomic_load(queue.value);
        if (newVec != bits) [[unlikely]] {
          bits = newVec;
          continue; // retry, don't immediately reset hazptr, cause we will most likely be setting it again very quickly, and if not, definitely upon function exit.
        }

        AGT_assert( vec->count > 1 );

        // vec is now protected!

        if (vec->count == 2) {
          signal_waiter* nextWaiter;
          if (vec->waiters[0] == &waiter)
            nextWaiter = vec->waiters[1];
          else if (vec->waiters[1] == &waiter)
            nextWaiter = vec->waiters[0];
          else [[unlikely]]
            return false;
          nextBits = reinterpret_cast<uintptr_t>(nextWaiter);
        }
        else {
          bool found = false;
          for (int32_t i = 0; i < vec->count; ++i) {
            if (vec->waiters[i] == &waiter) {
              vec_erase(ctx, nextVec, vec, i);
              found = true;
              break;
            }
          }
          if (!found)
            return false;

          freeVec = nullptr;
          nextBits = reinterpret_cast<uintptr_t>(nextVec) | ValueIsVecBit;
        }
      }

      uintptr_t prevBits = bits;

      do {
        if (atomic_cas(queue.value, bits, nextBits))
          return true;
      } while(bits == prevBits);

    } while(true);
  }

  void insert(agt_ctx_t ctx, spsc_signal_queue& queue, signal_waiter& waiter) noexcept {

  }

  // Maybe try with hazard pointers and vec????

  void vec_pop_n(agt_ctx_t ctx, agt::spsc_signal_waiter_vec*& newVec, agt::spsc_signal_waiter_vec* prevVec, int32_t popCount) noexcept {
    AGT_invariant( prevVec != nullptr );
    const auto newSize = prevVec->count - popCount;
    AGT_invariant( newSize > 0 );
    bool shouldAllocateNewVec = false;

    if (!newVec) [[likely]]
      shouldAllocateNewVec = true;
    else if (newVec->capacity < newSize) [[unlikely]] {
      shouldAllocateNewVec = true;
      release(newVec);
    }

    if (shouldAllocateNewVec) {
      const auto newCapacity = align_to<4>(static_cast<agt_u32_t>(newSize));
      newVec = alloc_dyn<agt::spsc_signal_waiter_vec>(ctx, sizeof(agt::spsc_signal_waiter_vec) + (sizeof(void*) * newCapacity));
      newVec->capacity = static_cast<agt_i32_t>(newCapacity);
    }

    newVec->count = newSize;

    for (auto i = 0; i < newSize; ++i)
      newVec->waiters[i] = prevVec->waiters[popCount + i];
  }

  inline void wake(agt_ctx_t ctx, const signal_waiter* waiter) noexcept {
    waiter->resume(ctx, waiter->ctxexec, waiter->task);
  }
}


void agt::signal(agt_ctx_t ctx, spsc_signal_queue& queue, int32_t n) noexcept {
  AGT_invariant( n > 0 );
  auto waiterCount = atomic_exchange_sub(queue.size, n);

  if (waiterCount > 0) {
    n = std::min(waiterCount, n); // have to cap by waiterCount if the new size is negative to ensure this call results in exactly n wakes.

    agt::spsc_signal_waiter_vec* vec     = nullptr;
    agt::spsc_signal_waiter_vec* nextVec = nullptr;
    agt::spsc_signal_waiter_vec* freeVec;
    signal_waiter* loneWaiter            = nullptr;
    agt_hazptr_t hazptr                  = nullptr;

    on_scope_exit {
      if (hazptr != nullptr)
        drop_hazptr(ctx, hazptr);
      if (freeVec)
        release(freeVec); // no need to retire; it was never used, so nobody else could have ever acquired it.
    };

    auto bits = atomic_load(queue.value);

    do {
      uintptr_t nextBits;
      freeVec = nextVec;

      if ((bits & ValueIsVecBit) == 0) [[likely]] {
        if (bits == 0) [[unlikely]] // generally pretty unlikely that any of the waiters will have timed out between now and when the value of n was calculated, but it is possible.
          return;

        // bits is a single waiter, n >= 1, therefore we simply try replacing with zero.
        nextBits = 0;
        loneWaiter = reinterpret_cast<signal_waiter*>(bits);
        // This is expected to be the common path, so set loneWaiter, don't reset prevVec.
        // Instead, in the vec path, we set prevVec, and reset loneWaiter,
        // then, outside of the loop, we check if loneWaiter is set to determine which path was taken.
      }
      else {
        vec = reinterpret_cast<agt::spsc_signal_waiter_vec*>(bits & VecMask);
        AGT_invariant( vec != nullptr );
        if (hazptr == nullptr) [[likely]]
          hazptr = make_hazptr(ctx);
        set_hazptr(hazptr, vec);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        auto newVec = atomic_load(queue.value);
        if (newVec != bits) [[unlikely]] {
          bits = newVec;
          continue; // retry, don't immediately reset hazptr, cause we will most likely be setting it again very quickly, and if not, definitely by the end of the function.
        }
        loneWaiter = nullptr;

        // vec is now protected, and may be safely read from (but not written to).
        if (n >= vec->count) {
          nextBits = 0;
        }
        else if (vec->count - n == 1) {
          nextBits = reinterpret_cast<uintptr_t>(vec->waiters[n]);
        }
        else {
          vec_pop_n(ctx, nextVec, vec, n);
          freeVec = nullptr;
          nextBits = reinterpret_cast<uintptr_t>(nextVec) | ValueIsVecBit;
        }
      }

      uintptr_t prevBits = bits;

      do {

        if (atomic_cas(queue.value, bits, nextBits)) { // Success; now we actually wake the waiters we've popped from the queue.

          if (loneWaiter != nullptr) [[likely]] {
            ::wake(ctx, loneWaiter);
          }
          else {
            AGT_invariant( vec != nullptr );
            for (auto i = 0; i < n; ++i)
              ::wake(ctx, vec->waiters[i]);
            retire(ctx, vec);
          }
          return;
        }

      } while(bits == prevBits);

    } while(true);



  }
}


bool agt::wait(agt_ctx_t ctx, spsc_signal_queue& queue, agt_timeout_t timeout) noexcept {

  auto queueSize = atomic_relaxed_load(queue.size);
  if ( queueSize < 0 ) {
    atomic_relaxed_increment(queue.size);
    return true;
  }

  if (timeout == AGT_DO_NOT_WAIT) [[unlikely]]
    return false;

  signal_waiter waiter;
  init_waiter(ctx, waiter);

  ::insert(ctx, queue, waiter);

  while (!atomic_cas(queue.size, queueSize, queueSize + 1)) {
    if ( queueSize < 0 ) [[unlikely]] {
      remove_waiter(ctx, queue, waiter); // we don't care whether or not this succeeds
      atomic_relaxed_increment(queue.size);
      return true;
    }
  }


  if (timeout != AGT_WAIT) {
    if (!suspend_for(ctx, timeout) && remove_waiter(ctx, queue, waiter)) {
      atomic_decrement(queue.size); //
      return false;
    }
  }
  else {
    suspend(ctx);
  }

  // AGT_assert( !impl::task_queue_contains(queue, task) );

  return true;
}
