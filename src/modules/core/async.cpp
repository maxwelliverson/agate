//
// Created by maxwe on 2021-11-28.
//

#include "async.hpp"

#include "agate/atomic.hpp"
#include "ctx.hpp"
#include "exec.hpp"

#include "agate/flags.hpp"
#include "core/msg/message.hpp"
#include "instance.hpp"
#include "object.hpp"


#include "exports.hpp"


#include <cmath>
#include <utility>


using namespace agt;



namespace {


  inline constexpr agt_u64_t IncNonWaiterRef = 0x0000'0000'0000'0001ULL;
  inline constexpr agt_u64_t IncWaiterRef    = 0x0000'0001'0000'0001ULL;
  inline constexpr agt_u64_t DecNonWaiterRef = 0xFFFF'FFFF'FFFF'FFFFULL;
  inline constexpr agt_u64_t DecWaiterRef    = 0xFFFF'FFFE'FFFF'FFFFULL;
  inline constexpr agt_u64_t DetachWaiter    = 0xFFFF'FFFF'0000'0000ULL; // Adding the two's compliment works better than subtracting the normal value (handles overflow in a consistent manner).
  inline constexpr agt_u64_t AttachWaiter    = 0x0000'0001'0000'0000ULL;

  inline constexpr agt_u64_t ArrivedResponse = 0x0000'0000'0000'0001ULL;
  inline constexpr agt_u64_t DroppedResponse = 0x0000'0001'0000'0001ULL;


  agt::shared_async_data* get_shared_async_data(agt_ctx_t ctx, async_data_t data) noexcept {
    return nullptr; // TODO: Implement
  }




  /*


  agt_u64_t async_dropped_responses(agt_u64_t i) noexcept {
    return i >> 32;
  }

  agt_u64_t async_total_responses(agt_u64_t i) noexcept {
    return i & 0xFFFF'FFFF;
  }

  agt_u64_t async_successful_responses(agt_u64_t i) noexcept {
    return async_total_responses(i) - async_dropped_responses(i);
  }

  agt_u64_t async_total_waiters(agt_u64_t refCount) noexcept {
    return refCount >> 32;
  }

  agt_u64_t async_total_references(agt_u64_t refCount) noexcept {
    return refCount & 0xFFFF'FFFF;
  }


  void      async_data_attach_waiter(agt::async_data* data) noexcept {
    agt::atomic_exchange_add(data->refCount, AttachWaiter);
  }
  void      async_data_detach_waiter(agt::async_data* data) noexcept {
    agt::atomic_exchange_add(data->refCount, DetachWaiter);
  }

  void      async_data_modify_ref_count(agt_ctx_t ctx, async_data_t data_, agt_u64_t diff) noexcept {
    if (auto data = handle_cast<local_async_data>(data_))
      agt::atomic_exchange_add(data->refCount, diff);
    else
      agt::atomic_exchange_add(get_shared_async_data(ctx, data_)->refCount, diff);
  }

  void      async_data_attach_waiter(agt_ctx_t ctx, async_data_t data) noexcept {
    async_data_modify_ref_count(ctx, data, AttachWaiter);
  }

  void      async_data_detach_waiter(agt_ctx_t ctx, async_data_t data) noexcept {
    async_data_modify_ref_count(ctx, data, DetachWaiter);
  }


  void      async_data_advance_epoch(agt::async_data* data) noexcept {
    ++data->epoch;
    // data->currentKey = (async_key_t)((std::underlying_type_t<async_key_t>)data->currentKey + 1);
    agt::atomic_relaxed_store(data->responseCounter, 0);
  }*/




  /*agt_status_t     async_data_status(agt::async_data* data, agt_u32_t expectedCount) noexcept {
    if (expectedCount != 0) [[likely]] {
      agt_u64_t responseCount = agt::atomic_load(data->responseCounter);
      if (async_total_responses(responseCount) >= expectedCount) {
        // FIXME: (Probably fine) The following line may or may not need to be here depending on when waiters are attached, so to speak
        async_data_detach_waiter(data);
        return async_successful_responses(responseCount) >= expectedCount ? AGT_SUCCESS : AGT_ERROR_PARTNER_DISCONNECTED;
      }
      return AGT_NOT_READY;
    }
    return AGT_ERROR_NOT_BOUND;
  }

  agt_status_t     async_data_status(agt::shared_async_data* data, agt_u32_t expectedCount) noexcept {
    /*if (expectedCount != 0) [[likely]] {
      agt_u64_t responseCount = agt::impl::atomic_load(data->responseCounter);
      if (totalResponses(responseCount) >= expectedCount) {
        // FIXME: agt::atomic_exchange_add(asyncData->refCount, DetatchWaiter);
        //        The above line may or may not need to be here depending on when waiters are attached, so to speak
        return hasDroppedResponses(responseCount) ? AGT_ERROR_PARTNER_DISCONNECTED : AGT_SUCCESS;
      }
      return AGT_NOT_READY;
    }

    return AGT_ERROR_NOT_BOUND;#1#
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }

  agt_status_t     async_data_wait(agt_ctx_t ctx, agt::async_data* data, agt_u32_t expectedCount, agt_timeout_t timeout) noexcept {
    if (expectedCount != 0) [[likely]] {

    }
    /*if (expectedCount != 0) [[likely]] {
      if (!data->responseCount.waitFor(expectedCount, timeout))
        return false;
    }
    return true;#1#
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }

  agt_status_t     async_data_wait(agt_ctx_t ctx, agt::shared_async_data* data, agt_u32_t expectedCount, agt_timeout_t timeout) noexcept {
    /*if (expectedCount != 0) [[likely]] {
      if (!data->responseCount.waitFor(expectedCount, timeout))
        return false;
    }
    return true;#1#
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }

  void             async_data_destroy(agt_ctx_t context, agt::async_data* data) noexcept {

    AGT_assert(atomic_load(data->refCount) == 0);

    ctxReleaseLocalAsyncData(context, data);

    /*if (!data->isShared) {
      // ctxFreeAsyncData(context, data);
    }
    else {
      ctxRefFreeAsyncData(ctxGetContextById(context, data->contextId), data);
    }#1#
  }

  void             async_data_destroy(agt_ctx_t context, agt::shared_async_data* data, async_data_t handle) noexcept {

    AGT_assert(atomic_load(data->refCount) == 0);

    ctxReleaseSharedAsyncData(context, handle);

    /*if (!data->isShared) {
      // ctxFreeAsyncData(context, data);
    }
    else {
      ctxRefFreeAsyncData(ctxGetContextById(context, data->contextId), data);
    }#1#
  }

  agt::async_data* create_async_data(agt_ctx_t context, agt_u64_t initialRefCount) noexcept {
    auto data = (agt::async_data*)agt::ctxAcquireLocalAsyncData(context);

    data->epoch = {}; // key is reset between allocations;
    data->refCount   = initialRefCount;
    data->responseCounter = 0;
    data->expectedResponses = 0;
    data->errorCode = AGT_NOT_READY;
    data->isComplete = false;
    data->threadWaiterEstimate = 0;
    data->resultValue = 0;

    return data;
  }


  AGT_forceinline async_key_t current_key(agt::async_data* data) noexcept {
    return static_cast<async_key_t>(data->epoch);
  }


  async_data_t     asHandle(agt::async_data* data) noexcept {
    return static_cast<async_data_t>(reinterpret_cast<uintptr_t>(data));
  }

  async_data_t     to_handle(agt::local_async_data* data) noexcept {
    return static_cast<async_data_t>(reinterpret_cast<uintptr_t>(data));
  }


  // returns total responses

  AGT_forceinline agt_u32_t async_respond(agt::async_data* asyncData, agt_u64_t response) noexcept {
    return static_cast<agt_u32_t>(async_total_responses(atomic_exchange_add(asyncData->responseCounter, response) + response));
  }


  // Returns true if the caller should destroy the async_data object.
  template <bool Dropped, typename R>
  bool      try_async_response(agt_ctx_t ctx, async_data* data, agt_u32_t key, R result, bool& isComplete) noexcept {
    if (data->epoch == key) {
      auto totalResponses = async_respond<Dropped>(data);
      if (totalResponses == data->expectedResponses) [[likely]] {
        isComplete = true;
        data->isComplete = AGT_TRUE;
        std::atomic_thread_fence(std::memory_order_release);
        if constexpr (!std::same_as<R, int>) {
          if constexpr (Dropped)
            data->errorCode = result;
          else
            data->resultValue = result;
        }

        switch(data->threadWaiterEstimate) {
          [[likely]] case 0:
            break;
          [[likely]] case 1:
            atomicNotifyOneLocal(data->responseCounter);
          break;
          default:
            atomicNotifyAllLocal(data->responseCounter);
        }
      }
    }

    return atomic_exchange_add(data->refCount, DecNonWaiterRef) == 0x1;
  }


  template <bool SharedIsEnabled, bool Dropped, typename R = int>
  bool      async_data_signal_response(agt_ctx_t ctx, async_data_t data_, async_key_t key_, R result = 0) noexcept {
    local_async_data* localData;
    const auto key = static_cast<agt_u32_t>(key_);
    bool isComplete = false;


    if constexpr (SharedIsEnabled) {
      if (!(localData = handle_cast<local_async_data>(data_))) {
        const auto sharedData = get_shared_async_data(ctx, data_);
        if (try_async_response<Dropped>(ctx, sharedData, key, result, isComplete))
          async_data_destroy(ctx, sharedData, data_);
        return isComplete;
      }
    }
    else
      localData = reinterpret_cast<local_async_data*>(data_);

    if (try_async_response<Dropped>(ctx, localData, key, result, isComplete))
      async_data_destroy(ctx, localData);

    return isComplete;
  }
  */




  void destroy_local_async_data(local_async_data* data) noexcept {
    // TODO: If custom data types are added that require cleanup, indicate that in scData and do so here.
    // if (data->scData & AGT_ASYNC_DATA_ALLOCATED_CALLBACK_DATA)
      // agt::close_local(data->callbackData);
    release(data);
  }

  void release_ownership_of_local_async_data(local_async_data* data) noexcept {
    if (agt::impl::_release_ownership(data->rc)) {
      // there are no other open references to data; it is safe to clear, need be.
      // TODO: If custom data types are added that require cleanup, indicate that in scData and do so here.
      // if (data->scData & AGT_ASYNC_DATA_ALLOCATED_CALLBACK_DATA)
        // agt::close_local(data->callbackData);
      // data->scData &= ~AGT_ASYNC_DATA_ALLOCATED_CALLBACK_DATA; // clear the flag
    }

  }

}











namespace {

  void try_release_async_data(agt::async& async, agt_status_t status) noexcept {
    if (status != AGT_NOT_READY) {
      if (!(status == AGT_SUCCESS && test(async.flags, eAsyncRetainsDataOnSuccess))) {
        release_ownership_of_local_async_data(unsafe_handle_cast<local_async_data>(async.data));
        reset(async.flags, eAsyncHasOwnershipOfData | eAsyncRetainsDataOnSuccess);
      }
    }
  }
}









agt::local_async_data* agt::alloc_local_async_data(agt_ctx_t ctx, agt_u32_t weakRefs) noexcept {
  auto data = alloc<local_async_data>(ctx);
  _init(data->rc, weakRefs);
  data->scData = 0;
  task_queue_init(data->blockedQueue);
  std::memset(&data->userData[0], 0, sizeof data->userData);
  // data->callbackData = nullptr;
  // data->callback = callback;
  // data->callbackData = callbackData;
  return data;
}

agt::local_async_data* agt::recycle_local_async_data(agt_ctx_t ctx, async_data_t data_, agt_u32_t weakRefs, bool hasOwnership) noexcept {
  auto data = unsafe_handle_cast<local_async_data>(data_);
  if (!impl::_try_recycle(data->rc, weakRefs, hasOwnership)) // no need to release if recycle fails, cause recycling only fails if there's other owning references.
    return alloc_local_async_data(ctx, weakRefs);
  return data;
}

void                   agt::drop_local_async_data(async_data_t data_, bool hasOwnership) noexcept {
  auto data = unsafe_handle_cast<local_async_data>(data_);
  bool shouldRelease;
  if (hasOwnership)
    shouldRelease = impl::_drop_ownership(data->rc);
  else
    shouldRelease = impl::_drop_nonowner(data->rc);
  if (shouldRelease)
    destroy_local_async_data(data);
}

local_async_data*      agt::try_acquire_local_async(async_data_t data_, async_key_t key, bool& refCountIsZero) noexcept {
  const auto data = unsafe_handle_cast<local_async_data>(data_);

  if (!impl::_try_acquire(data->rc, key, refCountIsZero)) {
    if (refCountIsZero)
      destroy_local_async_data(data);
    return nullptr;
  }

  return data;
}

bool                   agt::release_local_async(async_data_t data_, bool retain) noexcept {
  const auto data = unsafe_handle_cast<local_async_data>(data_);
  bool shouldRelease;

  if (retain)
    shouldRelease = impl::_release_ownership(data->rc);
  else
    shouldRelease = impl::_drop_ownership(data->rc);

  if (shouldRelease)
    destroy_local_async_data(data);

  return shouldRelease;
}





agt::local_async_data* agt::local_async_bind(async& async, agt_u32_t weakRefs) noexcept {

  auto ctx = async.ctx;

  if (!ctx) [[unlikely]] {
    ctx = get_ctx();
    if (!ctx)
      abort(); // EHHhHHHHH
    async.ctx = ctx;
  }

  local_async_data* data;

  if (test(async.flags, eAsyncBound)) { // already something bound...
    data = recycle_local_async_data(ctx, async.data, weakRefs, test(async.flags, eAsyncHasOwnershipOfData));
  }
  else {
    data = alloc_local_async_data(ctx, weakRefs);
  }

  AGT_assert( data != nullptr );

  set_flags(async.flags, eAsyncBound | eAsyncHasOwnershipOfData);

  async.data = to_handle(data);
  // async.dataKey =
  return data;
}

void agt::local_async_clear(async& async) noexcept {
  if (test(async.flags, eAsyncHasOwnershipOfData)) {
    auto data = unsafe_handle_cast<local_async_data>(async.data);
    impl::_release_ownership(data->rc); // While this returns a boolean value, we still wish to keep the data even if no other references to it exist.
  }
  reset(async.flags, eAsyncHasOwnershipOfData);
  async.status = AGT_ERROR_NOT_BOUND;
}

void agt::local_async_destroy(async& async) noexcept {
  if (test(async.flags, eAsyncBound)) {
    drop_local_async_data(async.data, test(async.flags, eAsyncHasOwnershipOfData));
  }

  if (test(async.flags, eAsyncMemoryIsOwned)) {
    release(&async);
  }
}

void AGT_stdcall  agt::destroy_async_local(agt_async_t async) noexcept {
  if (!async)
    return;
  local_async_destroy(*static_cast<agt::async*>(async));
}




/*
template <bool SharedIsEnabled>
async_key_t agt::async_data_attach(agt_ctx_t ctx, async_data_t data) noexcept {
  local_async_data* localData;

  if constexpr (SharedIsEnabled) {
    // if data is local, assign to localData and fallthrough
    if (!(localData = handle_cast<local_async_data>(data))) {
      auto sharedData = get_shared_async_data(ctx, data);
      atomic_exchange_add(sharedData->refCount, IncNonWaiterRef);
      return static_cast<async_key_t>(sharedData->epoch);
    }
  }
  else {
    localData = reinterpret_cast<local_async_data*>(data);
  }

  atomic_exchange_add(localData->refCount, IncNonWaiterRef);
  return static_cast<async_key_t>(localData->epoch);
}
template <bool SharedIsEnabled>
async_key_t agt::async_data_get_key(agt_ctx_t ctx, async_data_t data) noexcept {
  async_data* dataBase;
  if constexpr (SharedIsEnabled) {
    if (!(dataBase = handle_cast<local_async_data>(data)))
      dataBase = get_shared_async_data(ctx, data);
  }
  else {
    dataBase = reinterpret_cast<local_async_data*>(data);
  }

  return static_cast<async_key_t>(dataBase->epoch);
}
template <bool SharedIsEnabled>
bool        agt::async_drop(agt_ctx_t ctx, async_data_t data, async_key_t key, agt_status_t status) noexcept {
  return async_data_signal_response<SharedIsEnabled, true>(ctx, data, key, status);
}
template <bool SharedIsEnabled>
bool        agt::async_arrive(agt_ctx_t ctx, async_data_t data, async_key_t key) noexcept {
  return async_data_signal_response<SharedIsEnabled, false>(ctx, data, key);
}
template <bool SharedIsEnabled>
bool        agt::async_arrive_with_result(agt_ctx_t ctx, async_data_t data, async_key_t key, agt_u64_t result) noexcept {
  return async_data_signal_response<SharedIsEnabled, false>(ctx, data, key, result);
}



template async_key_t agt::async_data_attach<true>(agt_ctx_t, async_data_t) noexcept;
template async_key_t agt::async_data_attach<false>(agt_ctx_t, async_data_t) noexcept;
template async_key_t agt::async_data_get_key<true>(agt_ctx_t, async_data_t) noexcept;
template async_key_t agt::async_data_get_key<false>(agt_ctx_t, async_data_t) noexcept;
template bool        agt::async_drop<true>(agt_ctx_t, async_data_t, async_key_t, agt_status_t) noexcept;
template bool        agt::async_drop<false>(agt_ctx_t, async_data_t, async_key_t, agt_status_t) noexcept;
template bool        agt::async_arrive<true>(agt_ctx_t, async_data_t, async_key_t) noexcept;
template bool        agt::async_arrive<false>(agt_ctx_t, async_data_t, async_key_t) noexcept;
template bool        agt::async_arrive_with_result<true>(agt_ctx_t, async_data_t, async_key_t, agt_u64_t) noexcept;
template bool        agt::async_arrive_with_result<false>(agt_ctx_t, async_data_t, async_key_t, agt_u64_t) noexcept;
*/







/*void         agt::async_copy_to(const async& fromAsync, async& toAsync) noexcept {

  AGT_assert(fromAsync.ctx == toAsync.ctx && "Cannot copy between agt_async_t objects from different contexts");

  if (fromAsync.status == AGT_NOT_READY) [[likely]] {

    if (fromAsync.data != toAsync.data ) [[likely]] {

      async_destroy(toAsync, false);
      toAsync.data                 = fromAsync.data;
      toAsync.dataKey              = fromAsync.dataKey;
      toAsync.resultValue          = fromAsync.resultValue;
      toAsync.status               = AGT_NOT_READY;
      async_data_modify_ref_count(toAsync.ctx, toAsync.data, IncWaiterRef);
    }
    else if (fromAsync.dataKey != toAsync.dataKey) {
      // We can determine that fromAsync.dataKey > toAsync.dataKey because we know fromAsync is waiting,
      // And a data's epoch can only be advanced by an entity if there are no other entities waiting on it.
      // Following the same logic, we can determine that toAsync is not waiting.

      toAsync.dataKey              = fromAsync.dataKey;
      toAsync.status               = AGT_NOT_READY;
      async_data_attach_waiter(toAsync.ctx, toAsync.data);
    }

    // Don't do anything otherwise. Because we already know fromAsync is still waiting,
    // toAsync is either already in the exact same state OR has already waited on the data object
    // and cached the results, which would be pointless and stupid to undo.
  }
  else {
    // Cheat a lil bit :^)
    async_clear(toAsync);
    toAsync.status = fromAsync.status;
  }

}*/

/*agt_async_data_t agt::asyncAttach(agt_async_t& async, agt_signal_t) noexcept {

}*/


/*
std::pair<async_data_t, async_key_t> agt::async_attach_local(async& async, agt_u32_t expectedCount, agt_u32_t attachedCount) noexcept {

  // TODO: Optimize this function lmao

  const bool isWaiting = async.status == AGT_NOT_READY;
  const auto refOffset = isWaiting ? DecWaiterRef : DecNonWaiterRef;
  const auto ctx = async.ctx;

  bool shouldCreateNew = false;

  if (auto data = handle_cast<local_async_data>(async.data)) {

    bool shouldDetatchAndCreateNew = false;

    if (isWaiting) {
      if (async_total_waiters(atomic_load(data->refCount)) != 1)
        shouldDetatchAndCreateNew = true;
    }
    else if (current_key(data) == async.dataKey) {
      // It is possible that the epochs don't match up but that it'd still be reusable (ie. the new operations have completed),
      // but it's not worth the effort to try. Chances are very likely that the entity that has already reused it will try to
      // attach again, in which case its best to let it for the sake of data locality. We wouldn't be avoiding new allocations
      // in that case, because if we reuse this data and the other entity tries to also reuse this data, it'll also have to
      // allocate a new object. TL;DR, while tempting to reuse every async_data object free for reuse, it's not always optimal.
      agt_u64_t refCount = atomic_relaxed_load(data->refCount);
      if (async_total_waiters(refCount) || !atomic_try_replace(data->refCount, refCount, refCount + AttachWaiter))
        shouldDetatchAndCreateNew = true;
    }
    else
      shouldDetatchAndCreateNew = true;


    if (shouldDetatchAndCreateNew) {
      // Detatch, but in the off chance that all other references were dropped before this, reuse.
      if (atomic_exchange_add(data->refCount, refOffset) + refOffset == 0)
        atomic_store(data->refCount, IncWaiterRef);
      else
        shouldCreateNew = true;
    }


    if (!shouldCreateNew) {
      async_data_advance_epoch(data);
      async.dataKey = current_key(data);
      data->expectedResponses = expectedCount;
      data->errorCode = AGT_NOT_READY;
      data->isComplete = false;
      atomic_store(data->responseCounter, 0);
    }
  }
  else {
    auto sharedData = get_shared_async_data(ctx, async.data);
    if (atomic_exchange_add(sharedData->refCount, refOffset) == 0)
      async_data_destroy(ctx, sharedData, async.data);
    shouldCreateNew = true;
  }

  if (shouldCreateNew) {
    const auto initialRefCount = IncWaiterRef + attachedCount;
    auto newData  = create_async_data(ctx, initialRefCount);
    newData->expectedResponses = expectedCount;
    newData->errorCode = AGT_NOT_READY;
    async.data    = asHandle(newData);
    async.dataKey = current_key(newData);
  }

  return { async.data, async.dataKey };
}



agt_status_t agt::async_wait(agt_async_t& async_, agt_timeout_t timeout) noexcept {

  auto& async = (agt::async&)async_;

  if (async.status != AGT_NOT_READY)
    return async.status;

  agt_status_t status;

  if (auto data = handle_cast<local_async_data>(async.data))
    status = async_data_wait(async.ctx, data, data->expectedResponses, timeout);
  else
    status = async_data_wait(async.ctx, get_shared_async_data(async.ctx, async.data), 1 /* what the fuck is this 1??? It is a placeholder #1#, timeout);

  async.status = status;

  // TODO: Answer question,
  //       As is, a wait timing out will return AGT_NOT_READY rather than AGT_TIMED_OUT. Is this what we want?
  //       Is there a meaningful difference between the two statuses that should be made clear?
  return status;
}

agt_status_t agt::async_status(async& async) noexcept {
  if (async.status != AGT_NOT_READY)
    return async.status;

  agt_status_t status;

  if (auto data = handle_cast<local_async_data>(async.data))
    status = async_data_status(data, data->expectedResponses);
  else
    status = async_data_status(get_shared_async_data(async.ctx, async.data), 1 /* ??? pls fix #1#);

  async.status = status;
  return status;
}


bool         agt::async_is_complete(async& async) noexcept {
  if (test(async.flags, eAsyncIsComplete))
    return true;
  bool isComplete = true; /// TODO: implement
  if (isComplete)
    async.flags |= eAsyncIsComplete;
  return isComplete;
}

void         agt::init_async(agt_ctx_t context, agt_async_t& async_, agt_async_flags_t flags) noexcept {
  auto&& async = (agt::async&)async_;
  auto data  = create_async_data(context, IncNonWaiterRef);

  async.ctx              = context;
  if (test(async.flags, eAsyncMemoryIsOwned))
    async.structSize           = sizeof(agt::async)/*inst_get_builtin(context->instance, builtin_value::async_struct_size)#1#;
  async.data                 = asHandle(data);
  async.dataKey              = current_key(data);
  async.status               = AGT_ERROR_NOT_BOUND;
  async.resultValue          = 0;
  // async.desiredResponseCount = 0;
}




agt_status_t agt::async_wait(async& async, enter_wait_callback_t enterWait, void *enterWaitUserData, async_callback_t wakeCallback, void* wakeData) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
*/





agt_status_t agt::local_async_status(async& async, agt_u64_t* pResult) noexcept {
  if (!test(async.flags, eAsyncHasOwnershipOfData))
    return AGT_ERROR_NOT_BOUND;

  AGT_invariant( async_data_is_attached(async.data) );
  AGT_invariant( async.resolveCallback != nullptr );
  const auto data = unsafe_handle_cast<local_async_data>(async.data);
  // const auto dataObj = data->callbackData;

  agt_status_t status = async.resolveCallback(&data->userData[0], pResult, async.resolveCallbackData);

  try_release_async_data(async, status);

  return status;
}


agt_async_t  AGT_stdcall agt::make_new_async(agt_ctx_t ctx, agt_async_flags_t flags) {
  auto async = alloc<agt::async>(ctx);
  async->structSize = static_cast<agt_u32_t>(sizeof(agt::async));
  async->ctx = ctx;
  async->flags = static_cast<agt::async_flags>(flags) | eAsyncMemoryIsOwned;
  async->data = {};
  async->status = AGT_ERROR_NOT_BOUND;
  async->resolveCallback = nullptr;
  async->resolveCallbackData = nullptr;
  return async;
}


void         AGT_stdcall agt::copy_async_private(const async* from, async* to) {}
void         AGT_stdcall agt::copy_async_shared(const async* from, async* to) {}


agt_status_t agt::local_async_wait(async& async, agt_u64_t* pResult) noexcept {
  const auto ctx = async.ctx;

  if (!test(async.flags, eAsyncHasOwnershipOfData))
    return AGT_ERROR_NOT_BOUND;
  AGT_assert( async_data_is_attached(async.data) );
  const auto data = unsafe_handle_cast<local_async_data>(async.data);

  blocked_task blockedTask;

  auto resolveFunc = async.resolveCallback;
  auto resolveData = async.resolveCallbackData;

  auto asyncUserData = get_local_async_user_data(data);

  auto status = resolveFunc(asyncUserData, pResult, resolveData);

  if (status == AGT_NOT_READY) {
    blocked_task_init(ctx, blockedTask);
    insert(ctx, data->blockedQueue, blockedTask);
    status = resolveFunc(asyncUserData, pResult, resolveData);
    if (status == AGT_NOT_READY) {
      suspend(ctx);
      status = resolveFunc(asyncUserData, pResult, resolveData);
      AGT_assert( status != AGT_NOT_READY );
    }
    else
      remove(data->blockedQueue, blockedTask);
  }

  try_release_async_data(async, status);

  return status;
}

agt_status_t agt::local_async_wait_for(async& async, agt_u64_t* pResult, agt_timeout_t timeout) noexcept {
  const auto ctx = async.ctx;

  if (!test(async.flags, eAsyncHasOwnershipOfData))
    return AGT_ERROR_NOT_BOUND;
  AGT_assert( async_data_is_attached(async.data) );
  const auto data = unsafe_handle_cast<local_async_data>(async.data);

  blocked_task blockedTask;

  auto resolveFunc = async.resolveCallback;
  auto resolveData = async.resolveCallbackData;

  auto asyncUserData = get_local_async_user_data(data);

  auto status = resolveFunc(asyncUserData, pResult, resolveData);

  if (status == AGT_NOT_READY) {
    blocked_task_init(ctx, blockedTask);
    insert(ctx, data->blockedQueue, blockedTask);
    status = resolveFunc(asyncUserData, pResult, resolveData);
    if (status == AGT_NOT_READY) {
      if (suspend_for(ctx, timeout))
        status = AGT_SUCCESS;
      else if (remove(data->blockedQueue, blockedTask)) [[likely]]
        status = AGT_TIMED_OUT;
      else
        status = resolveFunc(asyncUserData, pResult, resolveData);
      AGT_assert( status != AGT_NOT_READY );
    }
    else
      remove(data->blockedQueue, blockedTask);
  }

  if (status != AGT_TIMED_OUT)
    try_release_async_data(async, status);

  return status;
}





namespace agt {





  agt_status_t AGT_stdcall async_get_status_private(agt_async_t async, agt_u64_t* pResult) {
    if (async == AGT_FORGET || async == AGT_SYNCHRONIZE) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;
    return local_async_status(*static_cast<agt::async*>(async), pResult);
  }
  agt_status_t AGT_stdcall async_get_status_shared(agt_async_t async, agt_u64_t* pResult) {
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }


  agt_status_t AGT_stdcall async_wait_native_unit_private(agt_async_t async_, agt_u64_t* pResult, agt_timeout_t timeout) {
    if (async_ == AGT_FORGET || async_ == AGT_SYNCHRONIZE) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;

    const auto async = static_cast<agt::async*>(async_);

    if (!test(async->flags, eAsyncHasOwnershipOfData))
      return AGT_ERROR_NOT_BOUND;

    if (timeout == AGT_DO_NOT_WAIT)
      return local_async_status(*async, pResult);
    else if (timeout == AGT_WAIT)
      return local_async_wait(*async, pResult);
    else
      return local_async_wait_for(*async, pResult, timeout);
  }
  agt_status_t AGT_stdcall async_wait_foreign_unit_private(agt_async_t async, agt_u64_t* pResult, agt_timeout_t timeout) {
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }
  agt_status_t AGT_stdcall async_wait_native_unit_shared(agt_async_t async, agt_u64_t* pResult, agt_timeout_t timeout) {
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }
  agt_status_t AGT_stdcall async_wait_foreign_unit_shared(agt_async_t async, agt_u64_t* pResult, agt_timeout_t timeout) {
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }
}

