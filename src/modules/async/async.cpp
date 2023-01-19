//
// Created by maxwe on 2021-11-28.
//

#include "async.hpp"
#include "agate/atomic.hpp"

#include "agate/flags.hpp"
#include "modules/channels/message.hpp"
#include "modules/core/instance.hpp"
#include "modules/core/object.hpp"


#include <utility>
#include <cmath>


namespace agt {

  AGT_BITFLAG_ENUM(async_flags, agt_u32_t) {
      eUnbound   = 0x0,
      eBound     = 0x1,
      eReady     = 0x2,
      eWaiting   = 0x4
  };


  inline constexpr async_flags eAsyncNoFlags = { };
  inline constexpr async_flags eAsyncUnbound = async_flags::eUnbound;
  inline constexpr async_flags eAsyncBound   = async_flags::eBound;
  inline constexpr async_flags eAsyncReady   = async_flags::eBound | async_flags::eReady;
  inline constexpr async_flags eAsyncWaiting = async_flags::eBound | async_flags::eWaiting;


  inline constexpr agt_u64_t IncNonWaiterRef = 0x0000'0000'0000'0001ULL;
  inline constexpr agt_u64_t IncWaiterRef    = 0x0000'0001'0000'0001ULL;
  inline constexpr agt_u64_t DecNonWaiterRef = 0xFFFF'FFFF'FFFF'FFFFULL;
  inline constexpr agt_u64_t DecWaiterRef    = 0xFFFF'FFFE'FFFF'FFFFULL;
  inline constexpr agt_u64_t DetachWaiter    = 0xFFFF'FFFF'0000'0000ULL;
  inline constexpr agt_u64_t AttachWaiter    = 0x0000'0001'0000'0000ULL;

  inline constexpr agt_u64_t ArrivedResponse = 0x0000'0000'0000'0001ULL;
  inline constexpr agt_u64_t DroppedResponse = 0x0000'0001'0000'0001ULL;





  struct async {
    agt_ctx_t         context;
    agt_status_t      status;
    agt_async_flags_t flags;
    agt_u32_t         desiredResponseCount;
    async_key_t       dataKey;
    async_data_t      data;
    // agt_u32_t         asyncStructSize;
    // void* asyncDependents
    // Version 1.0 end
  };


  struct async_data {
    async_data* next;
    async_key_t currentKey;       // Current epoch
    // Empty 32 bits
    agt_u64_t   refCount;
    agt_u64_t   responseCounter;
    /*agt_u32_t              instanceRefCount; // Total number of references to this object in memory, regardless of epoch
    agt_u32_t              epochRefCount;    // Total number of active waiters for this epoch. This data is considered "active" so long as this is non-zero.
    ref_count refCount;

    agt_u32_t              waiterRefCount;
    ref_count attachedRefCount;
    // atomic_monotonic_counter responseCount;*/
  };

  struct shared_async_data {
    uintptr_t   nextOffset;
    async_key_t currentKey;       // Current epoch
    // Empty 32 bits
    agt_u64_t   refCount;
    agt_u64_t   responseCounter;
  };


  struct imported_async_data {

  };
}

using namespace agt;

namespace {

  inline agt::async_data*        localAsyncData(async_data_t asyncData) noexcept {
    return (static_cast<std::underlying_type_t<async_data_t>>(asyncData) & 0x1)
               ? nullptr
               : reinterpret_cast<agt::async_data*>(asyncData);
  }

  inline agt::shared_async_data* sharedAsyncData(agt_ctx_t context, async_data_t asyncData) noexcept {
    return (agt::shared_async_data*) ctxMapSharedAsyncData(context, asyncData);
  }


  inline agt_u64_t        droppedResponses(agt_u64_t i) noexcept {
    return i >> 32;
  }

  inline agt_u64_t        totalResponses(agt_u64_t i) noexcept {
    return i & 0xFFFF'FFFF;
  }

  inline agt_u64_t        successfulResponses(agt_u64_t i) noexcept {
    return totalResponses(i) - droppedResponses(i);
  }


  inline agt_u64_t totalWaiters(agt_u64_t refCount) noexcept {
    return refCount >> 32;
  }

  inline agt_u64_t totalReferences(agt_u64_t refCount) noexcept {
    return refCount & 0xFFFF'FFFF;
  }


  inline void      asyncDataAttachWaiter(agt::async_data* data) noexcept {
    impl::atomicExchangeAdd(data->refCount, AttachWaiter);
  }
  inline void      asyncDataDetachWaiter(agt::async_data* data) noexcept {
    impl::atomicExchangeAdd(data->refCount, DetachWaiter);
  }

  inline void      asyncDataModifyRefCount(agt_ctx_t ctx, async_data_t data_, agt_u64_t diff) noexcept {
    if (auto data = localAsyncData(data_))
      impl::atomicExchangeAdd(data->refCount, diff);
    else
      impl::atomicExchangeAdd(sharedAsyncData(ctx, data_)->refCount, diff);
  }

  inline void      asyncDataAttachWaiter(agt_ctx_t ctx, async_data_t data) noexcept {
    asyncDataModifyRefCount(ctx, data, AttachWaiter);
  }

  inline void      asyncDataDetatchWaiter(agt_ctx_t ctx, async_data_t data) noexcept {
    asyncDataModifyRefCount(ctx, data, DetatchWaiter);
  }


  inline void      asyncDataAdvanceEpoch(agt::async_data* data) noexcept {
    data->currentKey = (async_key_t)((std::underlying_type_t<async_key_t>)data->currentKey + 1);
    impl::atomicStore(data->responseCounter, 0);
  }




  agt_status_t     asyncDataStatus(agt::async_data* data, agt_u32_t expectedCount) noexcept {
    if (expectedCount != 0) [[likely]] {
      agt_u64_t responseCount = agt::impl::atomicLoad(data->responseCounter);
      if (totalResponses(responseCount) >= expectedCount) {
        // FIXME: (Probably fine) The following line may or may not need to be here depending on when waiters are attached, so to speak
        agt::impl::atomicExchangeAdd(data->refCount, DetatchWaiter);
        return hasDroppedResponses(responseCount) ? AGT_ERROR_PARTNER_DISCONNECTED : AGT_SUCCESS;
      }
      return AGT_NOT_READY;
    }

    return AGT_ERROR_NOT_BOUND;
  }

  agt_status_t     asyncDataStatus(agt::shared_async_data* data, agt_u32_t expectedCount) noexcept {
    if (expectedCount != 0) [[likely]] {
      agt_u64_t responseCount = agt::impl::atomicLoad(data->responseCounter);
      if (totalResponses(responseCount) >= expectedCount) {
        // FIXME: agt::impl::atomicExchangeAdd(asyncData->refCount, DetatchWaiter);
        //        The above line may or may not need to be here depending on when waiters are attached, so to speak
        return hasDroppedResponses(responseCount) ? AGT_ERROR_PARTNER_DISCONNECTED : AGT_SUCCESS;
      }
      return AGT_NOT_READY;
    }

    return AGT_ERROR_NOT_BOUND;
  }

  agt_status_t     asyncDataWait(agt::async_data* data, agt_u32_t expectedCount, agt_timeout_t timeout) noexcept {
    if (expectedCount != 0) [[likely]] {
      if (!data->responseCount.waitFor(expectedCount, timeout))
        return false;
    }
    return true;
  }

  agt_status_t     asyncDataWait(agt::shared_async_data* data, agt_u32_t expectedCount, agt_timeout_t timeout) noexcept {
    if (expectedCount != 0) [[likely]] {
      if (!data->responseCount.waitFor(expectedCount, timeout))
        return false;
    }
    return true;
  }

  void             asyncDataDestroy(agt_ctx_t context, agt::async_data* data) noexcept {

    AGT_assert(impl::atomicLoad(data->refCount) == 0);

    ctxReleaseLocalAsyncData(context, data);

    /*if (!data->isShared) {
      // ctxFreeAsyncData(context, data);
    }
    else {
      ctxRefFreeAsyncData(ctxGetContextById(context, data->contextId), data);
    }*/
  }

  void             asyncDataDestroy(agt_ctx_t context, agt::shared_async_data* data, async_data_t handle) noexcept {

    AGT_assert(impl::atomicLoad(data->refCount) == 0);

    ctxReleaseSharedAsyncData(context, handle);

    /*if (!data->isShared) {
      // ctxFreeAsyncData(context, data);
    }
    else {
      ctxRefFreeAsyncData(ctxGetContextById(context, data->contextId), data);
    }*/
  }

  agt::async_data* createAsyncData(agt_ctx_t context, agt_u64_t initialRefCount) noexcept {
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


  void      asyncDataSignalResponse(async_data_t data_, agt_ctx_t ctx, async_key_t key, agt_u64_t diff) noexcept {
    if (auto data = localAsyncData(data_)) [[likely]] {
      if (data->currentKey == key)
        impl::atomicExchangeAdd(data->responseCounter, diff);
      if (impl::atomicExchangeAdd(data->refCount, DecNonWaiterRef) == 0)
        asyncDataDestroy(ctx, data);
    }
    else {
      auto sharedData = sharedAsyncData(ctx, data_);
      if (sharedData->currentKey == key)
        impl::atomicExchangeAdd(sharedData->responseCounter, diff);
      if (impl::atomicExchangeAdd(sharedData->refCount, DecNonWaiterRef) == 0)
        asyncDataDestroy(ctx, sharedData, data_);
    }
  }
}






async_key_t  agt::asyncDataAttach(async_data_t data_, agt_ctx_t ctx) noexcept {
  if (auto data = localAsyncData(data_)) [[likely]] {
    impl::atomicExchangeAdd(data->refCount, IncNonWaiterRef);
    return data->currentKey;
  }
  else {
    auto sharedData = sharedAsyncData(ctx, data_);
    impl::atomicExchangeAdd(sharedData->refCount, IncNonWaiterRef);
    return sharedData->currentKey;
  }
}

async_key_t  agt::asyncDataGetKey(async_data_t data_, agt_ctx_t ctx) noexcept {
  if (auto data = localAsyncData(data_)) [[likely]]
    return data->currentKey;
  return sharedAsyncData(ctx, data_)->currentKey;
}

void         agt::asyncDataDrop(async_data_t data, agt_ctx_t ctx, async_key_t key) noexcept {
  asyncDataSignalResponse(data, ctx, key, DroppedResponse);
}

void         agt::asyncDataArrive(async_data_t data, agt_ctx_t ctx, async_key_t key) noexcept {
  asyncDataSignalResponse(data, ctx, key, DroppedResponse);
}











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



#define AGT_return_type agt_status_t

#undef AGT_exported_function_name
#define AGT_exported_function_name new_async

AGT_export_family {

  AGT_function_entry(pa40)(agt_ctx_t ctx, agt_async_t* pAsync, agt_async_flags_t flags) {

    AGT_assert(ctx != nullptr);

    if (pAsync == nullptr) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;



    auto&& async = (agt::async&)*pAsync;
    auto data    = createAsyncData(context, IncNonWaiterRef);

    async.context              = context;
    async.asyncStructSize      = ctxGetBuiltin(context, builtin_value::asyncStructSize);
    async.data                 = asHandle(data);
    async.dataKey              = data->currentKey;
    async.status               = AGT_ERROR_NOT_BOUND;
    async.desiredResponseCount = 0;
  }




}


#undef AGT_return_type
#define AGT_return_type void

#undef AGT_exported_function_name
#define AGT_exported_function_name copy_async


AGT_export_family {

  AGT_function_entry(pa40)(const agt_async_t* pFrom, agt_async_t* pTo) {

  }




}



#undef AGT_exported_function_name
#define AGT_exported_function_name move_async


AGT_export_family {

  AGT_function_entry(pa40)(agt_async_t* pFrom, agt_async_t* pTo) {

  }




}



#undef AGT_exported_function_name
#define AGT_exported_function_name clear_async


AGT_export_family {

  AGT_function_entry(pa40)(agt_async_t* async) {

  }




}



#undef AGT_exported_function_name
#define AGT_exported_function_name destroy_async


AGT_export_family {

  AGT_function_entry(pa40)(agt_async_t* async) {

  }




}

