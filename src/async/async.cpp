//
// Created by maxwe on 2021-11-28.
//

#include "async.hpp"
#include "support/atomic.hpp"

#include "context/context.hpp"
#include "core/objects.hpp"
#include "messages/message.hpp"
#include "support/flags.hpp"


#include <utility>
#include <cmath>


using namespace agt;

namespace {

  AGT_BITFLAG_ENUM(AsyncFlags, agt_u32_t) {
    eUnbound   = 0x0,
    eBound     = 0x1,
    eReady     = 0x2,
    eWaiting   = 0x4
  };


  inline constexpr AsyncFlags eAsyncNoFlags          = {};
  inline constexpr AsyncFlags eAsyncUnbound          = AsyncFlags::eUnbound;
  inline constexpr AsyncFlags eAsyncBound            = AsyncFlags::eBound;
  inline constexpr AsyncFlags eAsyncReady            = AsyncFlags::eBound | AsyncFlags::eReady;
  inline constexpr AsyncFlags eAsyncWaiting          = AsyncFlags::eBound | AsyncFlags::eWaiting;

  /*struct ArrivalCount {
    union {
      struct {
        agt_u32_t     arrivedCount;
        agt_u32_t     droppedCount;
      };
      agt_u64_t       totalCount;
    };
  };

  class ResponseQuery {
    enum {
      eRequireAny      = 0x1,
      eRequireMinCount = 0x2,
      eAllowDropped    = 0x4
    };

    ResponseQuery(agt_u32_t minResponseCount, agt_u32_t flags) noexcept
        : minDesiredResponseCount(minResponseCount), flags(flags){}

  public:


    // ResponseQuery() noexcept : minDesiredResponseCount(), flags(){}



    static ResponseQuery requireAny(bool countDropped = false) noexcept {
      return { 0, eRequireAny | (countDropped ? eAllowDropped : 0u) };
    }
    static ResponseQuery requireAll(bool countDropped = false) noexcept {
      return { 0, countDropped ? eAllowDropped : 0u };
    }
    static ResponseQuery requireAtLeast(agt_u32_t minCount, bool countDropped = false) noexcept {
      return { minCount, eRequireMinCount | (countDropped ? eAllowDropped : 0u) };
    }

  private:

    friend class ResponseCount;

    agt_u32_t minDesiredResponseCount;
    agt_u32_t flags;
  };

  enum class ResponseQueryResult {
    eNotReady,
    eReady,
    eFailed
  };

  class ResponseCount {
  public:

    ResponseCount() = default;


    void arrive() noexcept {
      _InterlockedIncrement(&arrivedCount_);
      notifyWaiters();
    }

    void drop() noexcept {
      _InterlockedIncrement(&droppedCount_);
      notifyWaiters();
    }

    void reset(agt_u32_t maxExpectedResponses) noexcept {
      agt_u32_t capturedValue = 0;
      while ((capturedValue = _InterlockedCompareExchange(&arrivedCount_, 0, capturedValue)));

    }

    bool waitFor(ResponseQuery query, agt_timeout_t timeout) const noexcept {
      switch (timeout) {
        case AGT_WAIT:
          deepWait(expectedValue);
          return true;
        case AGT_DO_NOT_WAIT:
          return tryWaitOnce(expectedValue);
        default:
          if (timeout < TIMEOUT_US_LONG_WAIT_THRESHOLD)
            return shallowWaitFor(expectedValue, timeout);
          else
            return deepWaitFor(expectedValue, timeout);
      }
    }

    bool waitUntil(ResponseQuery query, deadline deadline) const noexcept {
      if (!deadline.isLong()) [[likely]] {
        return shallowWaitUntil(expectedValue, deadline);
      } else {
        return deepWaitUntil(expectedValue, deadline);
      }
    }



  private:

    bool      doDeepWait(agt_u32_t& capturedValue, agt_u32_t timeout) const noexcept;
    void      notifyWaiters() noexcept;


    AGT_forceinline agt_u32_t fastLoad() const noexcept {
      return __iso_volatile_load32(&reinterpret_cast<const int&>(arrivedCount_));
    }

    AGT_forceinline agt_u32_t orderedLoad() const noexcept {
      return _InterlockedCompareExchange(&const_cast<volatile agt_u32_t&>(arrivedCount_), 0, 0);
    }

    AGT_forceinline bool      isLessThan(agt_u32_t value) const noexcept {
      return orderedLoad() < value;
    }
    AGT_forceinline bool      isAtLeast(agt_u32_t value) const noexcept {
      return orderedLoad() >= value;
    }

    AGT_forceinline bool      isLessThan(ResponseQuery query) const noexcept {
      // return orderedLoad() < query.;
    }
    AGT_forceinline bool      isAtLeast(ResponseQuery query) const noexcept {
      return orderedLoad() >= value;
    }

    AGT_forceinline bool      shallowWaitFor(ResponseQuery query, agt_timeout_t timeout) const noexcept {
      return shallowWaitUntil(query, deadline::fromTimeout(timeout));
    }

    AGT_forceinline bool      shallowWaitUntil(ResponseQuery query, deadline deadline) const noexcept {
      agt_u32_t backoff = 0;
      while (isLessThan(expectedValue)) {
        if (deadline.hasPassed())
          return false;
        DUFFS_MACHINE(backoff);
      }
      return true;
    }

    AGT_forceinline bool      tryWaitOnce(ResponseQuery query) const noexcept {
      return orderedLoad() >= expectedValue;
    }


    AGT_noinline    void      deepWait(ResponseQuery query) const noexcept {
      agt_u32_t capturedValue = fastLoad();
      while (capturedValue < expectedValue) {
        doDeepWait(capturedValue, 0xFFFF'FFFF);
      }
    }

    AGT_noinline    bool      deepWaitFor(ResponseQuery query, agt_timeout_t timeout) const noexcept {
      deadline deadline = deadline::fromTimeout(timeout);
      agt_u32_t capturedValue = fastLoad();

      while (capturedValue < expectedValue) {
        if (!doDeepWait(capturedValue, deadline.toTimeoutMs()))
          return false;
      }
      return true;
    }

    AGT_noinline    bool      deepWaitUntil(ResponseQuery query, deadline deadline) const noexcept {
      agt_u32_t capturedValue = fastLoad();

      while (capturedValue < expectedValue) {
        if (!doDeepWait(capturedValue, deadline.toTimeoutMs()))
          return false;
      }
      return true;
    }



    union {
      struct {
        agt_u32_t     arrivedCount_;
        agt_u32_t     droppedCount_;
      };
      agt_u64_t       totalCount_ = 0;
    };

    agt_u32_t         expectedResponses_ = 0;
    mutable agt_u32_t deepSleepers_ = 0;
  };*/
}


extern "C" {

struct AgtLocalAsyncData_st {
  agt_u32_t attachedRefCount;
  agt_u32_t waiterRefCount;
};

struct agt_async_data_st {
  union {
    agt_async_data_t next;                     //
    size_t      nextOffset;               //
  };
  async_key              currentKey;       // Current epoch
  agt_u32_t              instanceRefCount; // Total number of references to this object in memory, regardless of epoch
  agt_u32_t              epochRefCount;    // Total number of active waiters for this epoch. This data is considered "active" so long as this is non-zero.
  ref_count refCount;
  agt_u32_t              waiterRefCount;
  ref_count attachedRefCount;
  atomic_monotonic_counter responseCount;
};

struct AgtSharedAsyncData_st {
  ObjectType type;
  ContextId  ownerCtx;
  ref_count refCount;
  ref_count waiterRefCount;
  agt_u32_t  currentKey;
};

struct agt_async_st {
  agt_ctx_t        context;
  agt_u32_t        desiredResponseCount;
  AsyncFlags       flags;
  async_key        dataKey;
  agt_async_data_t data;
};

}


namespace {







  void             asyncDataDoReset(agt_async_data_t data, async_key& key, agt_u32_t maxExpectedCount) noexcept {
    key = ++data->currentKey;

    data->attachedRefCount = ref_count(maxExpectedCount);
    data->responseCount    = atomic_monotonic_counter();
    data->maxResponseCount = maxExpectedCount;
  }


  bool             asyncDataResetWaiter(agt_async_data_t data, async_key& key, agt_u32_t maxExpectedCount) noexcept {

    agt_u32_t expectedWaiterCount = 1;
    agt_u32_t expectedNextWaiterCount = 1;

    while ( !Impl::atomicCompareExchange(data->waiterRefCount, expectedWaiterCount, expectedNextWaiterCount) ) {
      expectedNextWaiterCount = std::max(expectedWaiterCount - 1, 1u);
    }

    if (expectedNextWaiterCount != 1)
      return false;

    asyncDataDoReset(data, key, maxExpectedCount);

    return true;
  }

  bool             asyncDataResetNonWaiter(agt_async_data_t data, async_key& key, agt_u32_t maxExpectedCount) noexcept {

    agt_u32_t expectedWaiterCount = 0;

    if ( !Impl::atomicCompareExchange(data->waiterRefCount, expectedWaiterCount, 1) )
      return false;

    asyncDataDoReset(data, key, maxExpectedCount);

    return true;
  }


  bool             asyncDataWait(agt_async_data_t data, agt_u32_t expectedCount, agt_timeout_t timeout) noexcept {
    if (expectedCount != 0) [[likely]] {
      if (!data->responseCount.waitFor(expectedCount, timeout))
        return false;
    }
    return true;
  }

  void             asyncDataDestroy(agt_async_data_t data, agt_ctx_t context) noexcept {

    AGT_assert(data->waiterRefCount == 0);
    AGT_assert(data->attachedRefCount.get() == 0);

    if (!data->isShared) {
      ctxFreeAsyncData(context, data);
    }
    else {
      ctxRefFreeAsyncData(ctxGetContextById(context, data->contextId), data);
    }
  }

  agt_async_data_t createUnboundAsyncData(agt_ctx_t context) noexcept {
    auto memory = ctxAllocAsyncData(context);

    auto data = new (memory) AgtAsyncData_st {
      .contextId        = ctxGetContextId(context),
      .refCount         = ref_count(1),
      .waiterRefCount   = 0,
      .attachedRefCount = ref_count(0),
      .responseCount    = atomic_monotonic_counter(),
      .currentKey       = 0,
      .maxResponseCount = 0,
      .isShared         = false
    };

    return data;
  }

  agt_async_data_t createAsyncData(agt_ctx_t context, agt_u32_t maxExpectedCount) noexcept {
    auto memory = ctxAllocAsyncData(context);

    auto data = new (memory) AgtAsyncData_st {
      .contextId        = ctxGetContextId(context),
      .refCount         = ref_count(1),
      .waiterRefCount   = 1,
      .attachedRefCount = ref_count(maxExpectedCount),
      .responseCount    = atomic_monotonic_counter(),
      .currentKey       = 0,
      .maxResponseCount = maxExpectedCount,
      .isShared         = false
    };

    return data;
  }



}






void         Agt::asyncDataAttach(agt_async_data_t data, agt_ctx_t ctx, async_key& key) noexcept {
  // data->attachedRefCount.acquire();
  data->refCount.acquire();
  if (data->contextId != ctxGetContextId(ctx)) {
    data->isShared = true;
  }
  key = data->currentKey;
}

void         Agt::asyncDataDrop(agt_async_data_t data, agt_ctx_t ctx, async_key key) noexcept {
  if (data->currentKey == key) {
    ++data->responseCount;
  }

  if (data->refCount.release() == 0) {
    asyncDataDestroy(data, ctx);
  }
}

void         Agt::asyncDataArrive(agt_async_data_t data, agt_ctx_t ctx, async_key key) noexcept {
  if (data->currentKey == key) {
    ++data->responseCount;
  }

  if (data->refCount.release() == 0) {
    asyncDataDestroy(data, ctx);
  }
}











agt_ctx_t   Agt::asyncGetContext(const agt_async_st* async) noexcept {
  return async->context;
}
agt_async_data_t Agt::asyncGetData(const agt_async_st* async) noexcept {
  return async->data;
}

void         Agt::asyncCopyTo(const agt_async_st* fromAsync, agt_async_t toAsync) noexcept {

}

agt_async_data_t Agt::asyncAttach(agt_async_t async, agt_signal_t) noexcept {

}



void         Agt::asyncClear(agt_async_t async) noexcept {
  if ( static_cast<agt_u32_t>(async->flags & AsyncFlags::eWaiting) ) {
    --async->data->waiterRefCount;
  }

  async->flags = async->flags & eAsyncBound;
}

void         Agt::asyncDestroy(agt_async_t async) noexcept {

  if ( static_cast<agt_u32_t>(async->flags & AsyncFlags::eWaiting) ) {
    --async->data->waiterRefCount;
  }

  if ( async->data->refCount.release() == 0 ) {
    asyncDataDestroy(async->data, async->context);
  }

  delete async;
}

void         Agt::asyncReset(agt_async_t async, agt_u32_t targetExpectedCount, agt_u32_t maxExpectedCount) noexcept {

  const bool isWaiting = static_cast<agt_u32_t>(async->flags & AsyncFlags::eWaiting);

  if ( !( isWaiting ? asyncDataResetWaiter : asyncDataResetNonWaiter )(async->data, async->dataKey, maxExpectedCount) ) {
    if ( --async->data->refCount == 0 ) {
      Impl::atomicStore(async->data->waiterRefCount, 1);
      asyncDataDoReset(async->data, async->dataKey, maxExpectedCount);
    }
    else {
      async->data = createAsyncData(async->context, maxExpectedCount);
    }
  }

  async->flags = AsyncFlags::eBound | AsyncFlags::eWaiting;
  async->desiredResponseCount = targetExpectedCount;
}

agt_status_t    Agt::asyncWait(agt_async_t async, agt_timeout_t timeout) noexcept {

  if ( static_cast<agt_u32_t>(async->flags & eAsyncReady) )
    return AGT_SUCCESS;

  if ( !static_cast<agt_u32_t>(async->flags & eAsyncBound) )
    return AGT_ERROR_NOT_BOUND;

  if (asyncDataWait(async->data, async->desiredResponseCount, timeout)) {
    async->flags = async->flags | eAsyncReady;
    return AGT_SUCCESS;
  }

  return AGT_NOT_READY;
}

agt_async_t     Agt::createAsync(agt_ctx_t context) noexcept {
  auto async = new agt_async_st;
  auto data  = createUnboundAsyncData(context);

  async->context   = context;
  async->data      = data;
  async->dataKey   = data->currentKey;
  async->flags     = eAsyncUnbound;
  async->desiredResponseCount = 0;

  return async;
}
