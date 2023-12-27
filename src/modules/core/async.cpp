//
// Created by maxwe on 2021-11-28.
//

#include "async.hpp"
#include "agate/atomic.hpp"

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
  inline constexpr agt_u64_t DetachWaiter    = 0xFFFF'FFFF'0000'0000ULL;
  inline constexpr agt_u64_t AttachWaiter    = 0x0000'0001'0000'0000ULL;

  inline constexpr agt_u64_t ArrivedResponse = 0x0000'0000'0000'0001ULL;
  inline constexpr agt_u64_t DroppedResponse = 0x0000'0001'0000'0001ULL;


  agt::shared_async_data* get_shared_async_data(agt_ctx_t ctx, async_data_t data) noexcept;


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

    data->next = nullptr; // TODO: the next parameter isn't actually used while the data is allocated,
                          //       perhaps it could be used to implement ordering of some sort? Or the
                          //       allocator could cache a memo there while the data is in use for
                          //       faster allocation/deallocation.
    data->currentKey = {}; // key is reset between allocations;
    data->refCount   = initialRefCount;
    data->responseCounter = 0;

    return data;
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
  async_data_signal_response<SharedIsEnabled, true>(ctx, data, key, status);
}
template <bool SharedIsEnabled>
bool        agt::async_arrive(agt_ctx_t ctx, async_data_t data, async_key_t key) noexcept {
  async_data_signal_response<SharedIsEnabled, false>(ctx, data, key);
}
template <bool SharedIsEnabled>
bool        agt::async_arrive_with_result(agt_ctx_t ctx, async_data_t data, async_key_t key, agt_u64_t result) noexcept {
  async_data_signal_response<SharedIsEnabled, false>(ctx, data, key, result);
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








agt_ctx_t    agt::asyncGetContext(const agt_async_t& async) noexcept {
  return ((const agt::async&)async).context;
}

async_data_t agt::asyncGetData(const agt_async_t& async) noexcept {
  return ((const agt::async&)async).data;
}

void         agt::asyncCopyTo(const agt_async_t& fromAsync_, agt_async_t& toAsync_) noexcept {



  auto&& fromAsync = (const agt::async&)fromAsync_;
  auto&& toAsync   = (agt::async&)toAsync_;

  AGT_assert(fromAsync.context == toAsync.context && "Cannot copy between agt_async_t objects from different contexts");

  if (fromAsync.status == AGT_NOT_READY) [[likely]] {

    if (fromAsync.data != toAsync.data ) [[likely]] {
      asyncDestroy(toAsync_, false);
      toAsync.data                 = fromAsync.data;
      toAsync.dataKey              = fromAsync.dataKey;
      toAsync.desiredResponseCount = fromAsync.desiredResponseCount;
      toAsync.status               = AGT_NOT_READY;
      asyncDataModifyRefCount(toAsync.context, toAsync.data, IncWaiterRef);
    }
    else if (fromAsync.dataKey != toAsync.dataKey) {
      // We can determine that fromAsync.dataKey > toAsync.dataKey because we know fromAsync is waiting,
      // And a data's epoch can only be advanced by an entity if there are no other entities waiting on it.
      // Following the same logic, we can determine that toAsync is not waiting.

      toAsync.dataKey              = fromAsync.dataKey;
      toAsync.desiredResponseCount = fromAsync.desiredResponseCount;
      toAsync.status               = AGT_NOT_READY;
      asyncDataAttachWaiter(toAsync.context, toAsync.data);
    }
    else if (fromAsync.desiredResponseCount != toAsync.desiredResponseCount) {
      toAsync.desiredResponseCount = fromAsync.desiredResponseCount;
      if (toAsync.status != AGT_NOT_READY) {
        asyncDataAttachWaiter(toAsync.context, toAsync.data);
        toAsync.status = AGT_NOT_READY;
      }
    }
    // Don't do anything otherwise. Because we already know fromAsync is still waiting,
    // toAsync is either already in the exact same state OR has already waited on the data object
    // and cached the results, which would be pointless and stupid to undo.
  }
  else {
    // Cheat a lil bit :^)
    asyncClear(toAsync_);
    toAsync.status = fromAsync.status;
  }

}

/*agt_async_data_t agt::asyncAttach(agt_async_t& async, agt_signal_t) noexcept {

}*/



void         agt::asyncClear(agt_async_t& async_) noexcept {

  auto& async = (agt::async&)async_;

  if (std::exchange(async.status, AGT_ERROR_NOT_BOUND) == AGT_NOT_READY)
    asyncDataDetatchWaiter(async.context, async.data);
}

void         agt::asyncDestroy(agt_async_t& async_, bool wipeMemory) noexcept {

  auto& async = (agt::async&)async_;

  const auto refOffset = async.status == AGT_NOT_READY ? DecWaiterRef : DecNonWaiterRef;

  if (auto data = localAsyncData(async.data)) [[likely]] {

    if (impl::atomicExchangeAdd(data->refCount, refOffset) + refOffset == 0)
      asyncDataDestroy(async.context, data);
  }
  else {
    auto sharedData = sharedAsyncData(async.context, async.data);

    if (impl::atomicExchangeAdd(sharedData->refCount, refOffset) + refOffset == 0)
      asyncDataDestroy(async.context, sharedData, async.data);
  }

  // Important for future backwards compatibility that it clears async.asyncStructSize bytes only,
  // given that someone could be using an older version of the header file but a newer version of the library
  // where agt_async_t has been expanded. In this case, clearing sizeof(async) bytes would cause an out of
  // bounds write. Hats off to Microsoft for this little trick.
  if (wipeMemory)
    std::memset(&async_, 0, async.asyncStructSize);
}

std::pair<async_data_t, async_key_t> agt::asyncAttachLocal(agt_async_t& async_, agt_u32_t expectedCount, agt_u32_t attachedCount) noexcept {

  // TODO: Optimize this function lmao

  auto& async = (agt::async&)async_;

  const bool isWaiting = async.status == AGT_NOT_READY;
  const auto refOffset = isWaiting ? DecWaiterRef : DecNonWaiterRef;
  const auto ctx = async.context;

  bool shouldCreateNew = false;

  if (auto data = localAsyncData(async.data)) {

    bool shouldDetatchAndCreateNew = false;

    if (isWaiting) {
      agt_u64_t refCount = impl::atomicRelaxedLoad(data->refCount);
      // TODO: Implement
    }
    else if (data->currentKey == async.dataKey) {
      // It is possible that the epochs don't match up but that it'd still be reusable (ie. the new operations have completed),
      // but it's not worth the effort to try. Chances are very likely that the entity that has already reused it will try to
      // attach again, in which case its best to let it for the sake of data locality. We wouldn't be avoiding new allocations
      // in that case, because if we reuse this data and the other entity tries to also reuse this data, it'll also have to
      // allocate a new object. TL;DR, while tempting to reuse every async_data object free for reuse, it's not always optimal.
      agt_u64_t refCount = impl::atomicRelaxedLoad(data->refCount);
      if (!totalWaiters(refCount) && impl::atomicCompareExchange(data->refCount, refCount, refCount + AttachWaiter)) {
        asyncDataAdvanceEpoch(data);
        async.dataKey = data->currentKey;
      }
      else
        shouldDetatchAndCreateNew = true;
    }
    else
      shouldDetatchAndCreateNew = true;


    if (shouldDetatchAndCreateNew) {
      if (impl::atomicExchangeAdd(data->refCount, refOffset) + refOffset == 0) {
        impl::atomicStore(data->refCount, IncWaiterRef);
        asyncDataAdvanceEpoch(data);
        async.dataKey = data->currentKey;
      }
      else
        shouldCreateNew = true;
    }
  }
  else {
    auto sharedData = sharedAsyncData(ctx, async.data);
    if (impl::atomicExchangeAdd(sharedData->refCount, refOffset) == 0)
      asyncDataDestroy(ctx, sharedData, async.data);
    shouldCreateNew = true;
  }

  if (shouldCreateNew) {
    const auto initialRefCount = IncWaiterRef + attachedCount;
    auto newData  = createAsyncData(ctx, initialRefCount);
    async.data    = asHandle(newData);
    async.dataKey = newData->currentKey;
  }

  async.desiredResponseCount = expectedCount;
}



agt_status_t agt::asyncWait(agt_async_t& async_, agt_timeout_t timeout) noexcept {

  auto& async = (agt::async&)async_;

  if (async.status != AGT_NOT_READY)
    return async.status;

  agt_status_t status;

  if (auto data = localAsyncData(async.data))
    status = asyncDataWait(data, async.desiredResponseCount, timeout);
  else
    status = asyncDataWait(sharedAsyncData(async.context, async.data), async.desiredResponseCount, timeout);

  async.status = status;

  // TODO: Answer question,
  //       As is, a wait timing out will return AGT_NOT_READY rather than AGT_TIMED_OUT. Is this what we want?
  //       Is there a meaningful difference between the two statuses that should be made clear?
  return status;
}

agt_status_t agt::asyncStatus(agt_async_t& async_) noexcept {
  auto& async = (agt::async&)async_;

  if (async.status != AGT_NOT_READY)
    return async.status;

  agt_status_t status;

  if (auto data = localAsyncData(async.data))
    status = asyncDataStatus(data, async.desiredResponseCount);
  else
    status = asyncDataStatus(sharedAsyncData(async.context, async.data), async.desiredResponseCount);

  async.status = status;
  return status;
}

void         agt::initAsync(agt_ctx_t context, agt_async_t& async_, agt_async_flags_t flags) noexcept {
  auto&& async = (agt::async&)async_;
  auto data  = createAsyncData(context, IncNonWaiterRef);

  async.context              = context;
  async.asyncStructSize      = ctxGetBuiltin(context, builtin_value::asyncStructSize);
  async.data                 = asHandle(data);
  async.dataKey              = data->currentKey;
  async.status               = AGT_ERROR_NOT_BOUND;
  async.desiredResponseCount = 0;
}


