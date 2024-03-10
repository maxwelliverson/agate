//
// Created by maxwe on 2021-11-28.
//

#include "async.hpp"

#include "agate/atomic.hpp"
#include "ctx.hpp"

#include "agate/flags.hpp"
#include "channels/message.hpp"
#include "instance.hpp"
#include "object.hpp"


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
    agt::atomicExchangeAdd(data->refCount, AttachWaiter);
  }
  void      async_data_detach_waiter(agt::async_data* data) noexcept {
    agt::atomicExchangeAdd(data->refCount, DetachWaiter);
  }

  void      async_data_modify_ref_count(agt_ctx_t ctx, async_data_t data_, agt_u64_t diff) noexcept {
    if (auto data = handle_cast<local_async_data>(data_))
      agt::atomicExchangeAdd(data->refCount, diff);
    else
      agt::atomicExchangeAdd(get_shared_async_data(ctx, data_)->refCount, diff);
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
    agt::atomicRelaxedStore(data->responseCounter, 0);
  }




  agt_status_t     async_data_status(agt::async_data* data, agt_u32_t expectedCount) noexcept {
    if (expectedCount != 0) [[likely]] {
      agt_u64_t responseCount = agt::atomicLoad(data->responseCounter);
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
      agt_u64_t responseCount = agt::impl::atomicLoad(data->responseCounter);
      if (totalResponses(responseCount) >= expectedCount) {
        // FIXME: agt::impl::atomicExchangeAdd(asyncData->refCount, DetatchWaiter);
        //        The above line may or may not need to be here depending on when waiters are attached, so to speak
        return hasDroppedResponses(responseCount) ? AGT_ERROR_PARTNER_DISCONNECTED : AGT_SUCCESS;
      }
      return AGT_NOT_READY;
    }

    return AGT_ERROR_NOT_BOUND;*/
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }

  agt_status_t     async_data_wait(agt_ctx_t ctx, agt::async_data* data, agt_u32_t expectedCount, agt_timeout_t timeout) noexcept {
    if (expectedCount != 0) [[likely]] {

    }
    /*if (expectedCount != 0) [[likely]] {
      if (!data->responseCount.waitFor(expectedCount, timeout))
        return false;
    }
    return true;*/
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }

  agt_status_t     async_data_wait(agt_ctx_t ctx, agt::shared_async_data* data, agt_u32_t expectedCount, agt_timeout_t timeout) noexcept {
    /*if (expectedCount != 0) [[likely]] {
      if (!data->responseCount.waitFor(expectedCount, timeout))
        return false;
    }
    return true;*/
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }

  void             async_data_destroy(agt_ctx_t context, agt::async_data* data) noexcept {

    AGT_assert(atomicLoad(data->refCount) == 0);

    ctxReleaseLocalAsyncData(context, data);

    /*if (!data->isShared) {
      // ctxFreeAsyncData(context, data);
    }
    else {
      ctxRefFreeAsyncData(ctxGetContextById(context, data->contextId), data);
    }*/
  }

  void             async_data_destroy(agt_ctx_t context, agt::shared_async_data* data, async_data_t handle) noexcept {

    AGT_assert(atomicLoad(data->refCount) == 0);

    ctxReleaseSharedAsyncData(context, handle);

    /*if (!data->isShared) {
      // ctxFreeAsyncData(context, data);
    }
    else {
      ctxRefFreeAsyncData(ctxGetContextById(context, data->contextId), data);
    }*/
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


  // returns total responses
  template <bool Dropped>
  agt_u32_t async_respond(agt::async_data* asyncData) noexcept {
    constexpr static auto Diff = Dropped ? DroppedResponse : ArrivedResponse;
    return static_cast<agt_u32_t>(async_total_responses(atomicExchangeAdd(asyncData->responseCounter, Diff) + Diff));
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

    return atomicExchangeAdd(data->refCount, DecNonWaiterRef) == 0x1;
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
}






template <bool SharedIsEnabled>
async_key_t agt::async_data_attach(agt_ctx_t ctx, async_data_t data) noexcept {
  local_async_data* localData;

  if constexpr (SharedIsEnabled) {
    // if data is local, assign to localData and fallthrough
    if (!(localData = handle_cast<local_async_data>(data))) {
      auto sharedData = get_shared_async_data(ctx, data);
      atomicExchangeAdd(sharedData->refCount, IncNonWaiterRef);
      return static_cast<async_key_t>(sharedData->epoch);
    }
  }
  else {
    localData = reinterpret_cast<local_async_data*>(data);
  }

  atomicExchangeAdd(localData->refCount, IncNonWaiterRef);
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







void         agt::async_copy_to(const async& fromAsync, async& toAsync) noexcept {

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
    /*else if (fromAsync.desiredResponseCount != toAsync.desiredResponseCount) {
      toAsync.desiredResponseCount = fromAsync.desiredResponseCount;
      if (toAsync.status != AGT_NOT_READY) {
        asyncDataAttachWaiter(toAsync.ctx, toAsync.data);
        toAsync.status = AGT_NOT_READY;
      }
    }*/
    // Don't do anything otherwise. Because we already know fromAsync is still waiting,
    // toAsync is either already in the exact same state OR has already waited on the data object
    // and cached the results, which would be pointless and stupid to undo.
  }
  else {
    // Cheat a lil bit :^)
    async_clear(toAsync);
    toAsync.status = fromAsync.status;
  }

}

/*agt_async_data_t agt::asyncAttach(agt_async_t& async, agt_signal_t) noexcept {

}*/



void         agt::async_clear(async& async) noexcept {
  if (std::exchange(async.status, AGT_ERROR_NOT_BOUND) == AGT_NOT_READY)
    async_data_detach_waiter(async.ctx, async.data);
}

void         agt::async_destroy(async& async, bool wipeMemory) noexcept {

  const auto refOffset = async.status == AGT_NOT_READY ? DecWaiterRef : DecNonWaiterRef;

  if (auto data = handle_cast<local_async_data>(async.data)) [[likely]] {

    if (atomicExchangeAdd(data->refCount, refOffset) + refOffset == 0)
      async_data_destroy(async.ctx, data);
  }
  else {
    auto sharedData = get_shared_async_data(async.ctx, async.data);

    if (atomicExchangeAdd(sharedData->refCount, refOffset) + refOffset == 0)
      async_data_destroy(async.ctx, sharedData, async.data);
  }

  // Important for future backwards compatibility that it clears async.structSize bytes only,
  // given that someone could be using an older version of the header file but a newer version of the library
  // where agt_async_t has been expanded. In this case, clearing sizeof(async) bytes would cause an out of
  // bounds write. Hats off to Microsoft for this little trick.
  if (wipeMemory)
    std::memset(&async, 0, async.structSize);
}

std::pair<async_data_t, async_key_t> agt::async_attach_local(async& async, agt_u32_t expectedCount, agt_u32_t attachedCount) noexcept {

  // TODO: Optimize this function lmao

  const bool isWaiting = async.status == AGT_NOT_READY;
  const auto refOffset = isWaiting ? DecWaiterRef : DecNonWaiterRef;
  const auto ctx = async.ctx;

  bool shouldCreateNew = false;

  if (auto data = handle_cast<local_async_data>(async.data)) {

    bool shouldDetatchAndCreateNew = false;

    if (isWaiting) {
      if (async_total_waiters(atomicLoad(data->refCount)) != 1)
        shouldDetatchAndCreateNew = true;
    }
    else if (current_key(data) == async.dataKey) {
      // It is possible that the epochs don't match up but that it'd still be reusable (ie. the new operations have completed),
      // but it's not worth the effort to try. Chances are very likely that the entity that has already reused it will try to
      // attach again, in which case its best to let it for the sake of data locality. We wouldn't be avoiding new allocations
      // in that case, because if we reuse this data and the other entity tries to also reuse this data, it'll also have to
      // allocate a new object. TL;DR, while tempting to reuse every async_data object free for reuse, it's not always optimal.
      agt_u64_t refCount = atomicRelaxedLoad(data->refCount);
      if (async_total_waiters(refCount) || !atomicCompareExchange(data->refCount, refCount, refCount + AttachWaiter))
        shouldDetatchAndCreateNew = true;
    }
    else
      shouldDetatchAndCreateNew = true;


    if (shouldDetatchAndCreateNew) {
      // Detatch, but in the off chance that all other references were dropped before this, reuse.
      if (atomicExchangeAdd(data->refCount, refOffset) + refOffset == 0)
        atomicStore(data->refCount, IncWaiterRef);
      else
        shouldCreateNew = true;
    }


    if (!shouldCreateNew) {
      async_data_advance_epoch(data);
      async.dataKey = current_key(data);
      data->expectedResponses = expectedCount;
      data->errorCode = AGT_NOT_READY;
      data->isComplete = false;
      atomicStore(data->responseCounter, 0);
    }
  }
  else {
    auto sharedData = get_shared_async_data(ctx, async.data);
    if (atomicExchangeAdd(sharedData->refCount, refOffset) == 0)
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
    status = async_data_wait(async.ctx, get_shared_async_data(async.ctx, async.data), 1 /* what the fuck is this 1??? It is a placeholder */, timeout);

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
    status = async_data_status(get_shared_async_data(async.ctx, async.data), 1 /* ??? pls fix */);

  async.status = status;
  return status;
}

bool         agt::async_is_complete(async& async) noexcept {
  if (async.flags & ASYNC_IS_COMPLETE)
    return true;
  bool isComplete = ;
  if (isComplete)
    async.flags |= ASYNC_IS_COMPLETE;
  return isComplete;
}

void         agt::init_async(agt_ctx_t context, agt_async_t& async_, agt_async_flags_t flags) noexcept {
  auto&& async = (agt::async&)async_;
  auto data  = create_async_data(context, IncNonWaiterRef);

  async.ctx              = context;
  if (test(async.flags, eAsyncMemoryIsOwned))
    async.structSize           = sizeof(agt::async)/*inst_get_builtin(context->instance, builtin_value::async_struct_size)*/;
  async.data                 = asHandle(data);
  async.dataKey              = current_key(data);
  async.status               = AGT_ERROR_NOT_BOUND;
  async.resultValue          = 0;
  // async.desiredResponseCount = 0;
}


