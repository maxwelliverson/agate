//
// Created by maxwe on 2024-04-08.
//

#include "core/task_queue.hpp"

#include "core/object.hpp"
#include "core/ctx.hpp"

#include "agate/vector.hpp"


AGT_object(task_queue_vec) {
  agt_u32_t          capacity;
  agt::blocked_task* tasks[];
};

namespace {
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


  AGT_forceinline task_queue atomic_load(const task_queue& src) noexcept {
    union {
      uint64_t   u64[2];
      task_queue queue;
    };
    const auto srcPtr = reinterpret_cast<const uint64_t*>(&src);
    u64[0] = agt::atomic_relaxed_load(srcPtr[0]);
    u64[1] = agt::atomic_relaxed_load(srcPtr[1]);
    return queue;
  }

  AGT_forceinline bool atomic_cas(task_queue& var, task_queue& prevVal, const task_queue& newVal) noexcept {
    return agt::atomic_try_replace_16bytes(&var, &prevVal, &newVal);
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
}


void agt::blocked_task_init(agt_ctx_t ctx, blocked_task& task) noexcept {
  AGT_invariant( ctx != nullptr );
  task.execCtxData   = ctx->ctxexec;
  task.task          = static_cast<agt_ctxexec_header_t*>(ctx->ctxexec)->activeTask;
  task.pfnResumeTask = ctx->uexecVPtr->resume;
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
  prev = atomic_load(queue);

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
