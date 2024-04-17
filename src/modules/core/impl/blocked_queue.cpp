//
// Created by maxwe on 2024-04-01.
//

#include "blocked_queue.hpp"

#include "core/object.hpp"

#include "core/exec.hpp"

#include <bit>


AGT_object(blocked_queue_vec) {
  agt_u32_t                capacity;
  agt::impl::blocked_exec* execs[];
};

namespace {

  inline constexpr uintptr_t IsMultiValueBit  = 0x1;
  inline constexpr uint32_t  InitialMultiSize = 2;

  AGT_forceinline blocked_queue_vec* try_as_vec(uintptr_t val) noexcept {
    return (val & IsMultiValueBit) ? reinterpret_cast<blocked_queue_vec*>(val & ~IsMultiValueBit) : nullptr;
  }

  AGT_forceinline agt::impl::blocked_exec* as_exec(uintptr_t val) noexcept {
    return reinterpret_cast<agt::impl::blocked_exec*>(val);
  }

  AGT_forceinline void assign(agt::impl::blocked_queue& queue, agt::impl::blocked_exec* exec) noexcept {
    queue.value = reinterpret_cast<uintptr_t>(exec);
  }

  AGT_forceinline void assign(agt::impl::blocked_queue& queue, blocked_queue_vec* vec) noexcept {
    queue.value = (reinterpret_cast<uintptr_t>(vec) | IsMultiValueBit);
  }

  blocked_queue_vec* alloc_vec(agt_ctx_t ctx, uint32_t capacity) noexcept {
    auto vec = alloc_dyn<blocked_queue_vec>(ctx, sizeof(blocked_queue_vec) + (capacity * sizeof(void*)));
    vec->capacity = capacity;
    return vec;
  }


  void insert_single(agt_ctx_t ctx, agt::impl::blocked_queue& queue, agt::impl::blocked_exec* blockedExec) noexcept {

    if (queue.value == 0) [[likely]] {
      assign(queue, blockedExec);
    }
    else {
      blocked_queue_vec* nextVec;
      if (const auto vec = try_as_vec(queue.value)) {
        if (queue.size == vec->capacity) {
          nextVec = alloc_vec(ctx, vec->capacity * 2);
          for (uint32_t i = 0; i < queue.size; ++i) {
            nextVec->execs[i] = vec->execs[i];
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
        nextVec->execs[0] = as_exec(queue.value);
      }

      nextVec->execs[queue.size] = blockedExec;
      assign(queue, nextVec);
    }

    queue.size += 1;
  }
}




void agt::impl::unsafe_blocked_queue_insert(agt_ctx_t ctx, blocked_queue& queue, blocked_exec* blockedExec) noexcept {
  insert_single(ctx, queue, blockedExec);

  blockedExec->queue = &queue;
}


// Returns true if successful,
bool agt::impl::unsafe_unblock_local(blocked_exec* blockedExec) noexcept {

  auto& queue = *blockedExec->queue;

  if (queue.size != 0) {
    if (auto vec = try_as_vec(queue.value)) {
      for (uint32_t i = 0; i < queue.size; ++i) {
        if (vec->execs[i] == blockedExec) {
          queue.size -= 1;
          // Shift the following entries down
          while (i < queue.size) {
            vec->execs[i] = vec->execs[i + 1]; // it's okay that this technically goes 1 past queue.size, cause queue.size was just decremented, so we're taking the previous end.
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
      if (as_exec(queue.value) == blockedExec) {
        queue.value = 0;
        queue.size  = 0;
        return true;
      }
    }
  }

  return false;
}


// Slightly more complex implementation of blocked_queue_resolve that *may* reduce lock contention.
/*
bool agt::impl::blocked_queue_resolve(agt_ctx_t ctx, blocked_queue& queue, bool(* unblockCallback)(blocked_exec* exec, void* userData), void* userData) noexcept {
  lock(queue.spinlock);
  const uintptr_t value = std::exchange(queue.value, 0);
  const uint32_t  size  = std::exchange(queue.size, 0);
  unlock(queue.spinlock);

  blocked_queue_vec* prevVec = nullptr;
  vector<blocked_exec*, 4> requeueList{};

  if (size == 0)
    return true;

  if (auto vec = try_as_vec(value)) {
    for (uint32_t i = 0; i < size; ++i) {
      const auto exec = vec->execs[i];
      if (!unblockCallback(exec, userData))
        requeueList.push_back(exec);
    }
    if (requeueList.size() > 1)
      prevVec = vec;
    else
      release(vec);
  }
  else {
    AGT_invariant( size == 1 );
    const auto exec = as_exec(value);
    if (!unblockCallback(exec, userData))
      requeueList.push_back(exec);
  }


  if (requeueList.empty()) [[likely]]
    return true;

  if (requeueList.size() == 1) [[likely]] {
    assert( prevVec == nullptr );
    blocked_queue_insert(ctx, queue, requeueList.front());
    return false;
  }


  // Requeue multiple. Most complex/time-consuming, but should almost never happen, except where there's the highest contention.

  assert( prevVec != nullptr );
  lock(queue.spinlock);

  uint32_t newSize = queue.size + requeueList.size();


  if (newSize <= prevVec->capacity) [[likely]] {
    // The requeued blocks will always predate the new ones, so order is retained without any extra work :)
    for (auto i = 0; i < requeueList.size(); ++i)
      prevVec->execs[i] = requeueList[i];

    if (queue.size == 0) [[likely]] {
      // Have an empty body here rather than ignoring the case altogether, cause this is by far the most likely case, and thus we should catch it early most of the time.
    }
    else if (const auto vec = try_as_vec(queue.value)) [[unlikely]] {
      for (uint32_t i = 0; i < queue.size; ++i)
        prevVec->execs[requeueList.size() + i] = vec->execs[i];
      release(vec);
    }
    else {
      const auto singleExec = as_exec(queue.value);
      assign(queue, prevVec);
      queue.size = requeueList.size();
      insert_single(ctx, queue, singleExec);
    }
    assign(queue, prevVec);
  }
  else {
    release(prevVec);

    uint32_t newCapacity = align_to(newSize, 8);
    blocked_queue_vec* newVec = alloc_vec(ctx, newCapacity);

    uint32_t i;
    for (i = 0; i < requeueList.size(); ++i)
      newVec->execs[i] = requeueList[i];

    if (const auto vec = try_as_vec(queue.value)) {
      for (uint32_t j = 0; j < queue.size; ++j)
        newVec->execs[i + j] = vec->execs[j];
      release(vec);
    }
    else if (queue.size == 1) {
      newVec->execs[i] = as_exec(queue.value);
    }

    assign(queue, newVec);
  }

  queue.size = newSize;

  unlock(queue.spinlock);

  return false;
}
*/


// Unblock unconditionally. Queue is necessarily cleared following this call.
void agt::impl::unsafe_blocked_queue_resolve(agt_ctx_t ctx, blocked_queue& queue) noexcept {

  if (queue.size == 0)
    return;

  if (auto vec = try_as_vec(queue.value)) {
    for (uint32_t i = 0; i < queue.size; ++i) {
      const auto exec = vec->execs[i];
      exec->exec->vptr->wake(exec->exec, exec->self);
    }
    release(vec);
  }
  else {
    AGT_invariant( queue.size == 1 );
    const auto exec = as_exec(queue.value);
    exec->exec->vptr->wake(exec->exec, exec->self);
  }

  queue.value = 0;
  queue.size  = 0;
}

// Unblock unconditionally.
void agt::impl::unsafe_blocked_queue_resolve_one(agt_ctx_t ctx, blocked_queue& queue) noexcept {
  if (queue.size == 0)
    return;

  blocked_exec* exec;

  if (auto vec = try_as_vec(queue.value)) {
    exec = vec->execs[0];
    queue.size -= 1;
    for (uint32_t i = 0; i < queue.size; ++i) {
      vec->execs[i] = vec->execs[i + 1];
    }
    if (queue.size == 0) {
      release(vec);
      queue.value = 0;
    }
  }
  else {
    AGT_invariant( queue.size == 1 );
    exec = as_exec(queue.value);
    queue.value = 0;
    queue.size = 0;
  }

  exec->exec->vptr->wake(exec->exec, exec->self);
}



bool agt::impl::unsafe_blocked_queue_resolve(agt_ctx_t ctx, blocked_queue& queue, bool(* unblockCallback)(blocked_exec* exec, void* userData), void* userData) noexcept {

  if (queue.size == 0)
    return true;

  blocked_queue_vec* prevVec = nullptr;
  vector<blocked_exec*, 4> requeueList{};

  if (auto vec = try_as_vec(queue.value)) {
    for (uint32_t i = 0; i < queue.size; ++i) {
      const auto exec = vec->execs[i];
      if (!unblockCallback(exec, userData))
        requeueList.push_back(exec);
    }
    if (requeueList.size() > 1)
      prevVec = vec;
    else
      release(vec);
  }
  else {
    AGT_invariant( queue.size == 1 );
    const auto exec = as_exec(queue.value);
    if (!unblockCallback(exec, userData))
      requeueList.push_back(exec);
  }


  if (requeueList.empty()) [[likely]] {
    queue.value = 0;
    queue.size = 0;
    return true;
  }


  if (requeueList.size() == 1) [[likely]] {
    AGT_invariant( prevVec == nullptr );
    assign(queue, requeueList.front());
    queue.size = 1;
    return false;
  }


  // Requeue multiple. Thankfully, unlike the safe version, we don't have to worry about others having appended to our list in the meanwhile.


  AGT_invariant( prevVec != nullptr );

  queue.size = requeueList.size();

  for (auto i = 0; i < queue.size; ++i)
    prevVec->execs[i] = requeueList[i];

  return false;
}


bool agt::impl::unsafe_blocked_queue_resolve_one(agt_ctx_t ctx, blocked_queue& queue, bool(* unblockCallback)(blocked_exec* exec, void* userData), void* userData) noexcept {

  if (queue.size > 0) {
    if (auto vec = try_as_vec(queue.value)) {
      for (uint32_t i = 0; i < queue.size; ++i) {
        if (unblockCallback(vec->execs[i], userData)) {
          queue.size -= 1;
          if (queue.size == 0) {
            release(vec);
            queue.value = 0;
            return true;
          }

          // Shift everything down 1, as in unsafe_unblock_local
          for (; i < queue.size; ++i) {
            vec->execs[i] = vec->execs[i + 1];
          }
        }
      }

    }
    else {
      AGT_invariant(queue.size == 1);
      if (unblockCallback(as_exec(queue.value), userData)) {
        queue.value = 0;
        queue.size = 0;
        return true;
      }
    }
  }

  return false;
}
