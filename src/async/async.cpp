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


using namespace Agt;

namespace {

  AGT_BITFLAG_ENUM(AsyncFlags, AgtUInt32) {
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
        AgtUInt32     arrivedCount;
        AgtUInt32     droppedCount;
      };
      AgtUInt64       totalCount;
    };
  };

  class ResponseQuery {
    enum {
      eRequireAny      = 0x1,
      eRequireMinCount = 0x2,
      eAllowDropped    = 0x4
    };

    ResponseQuery(AgtUInt32 minResponseCount, AgtUInt32 flags) noexcept
        : minDesiredResponseCount(minResponseCount), flags(flags){}

  public:


    // ResponseQuery() noexcept : minDesiredResponseCount(), flags(){}



    static ResponseQuery requireAny(bool countDropped = false) noexcept {
      return { 0, eRequireAny | (countDropped ? eAllowDropped : 0u) };
    }
    static ResponseQuery requireAll(bool countDropped = false) noexcept {
      return { 0, countDropped ? eAllowDropped : 0u };
    }
    static ResponseQuery requireAtLeast(AgtUInt32 minCount, bool countDropped = false) noexcept {
      return { minCount, eRequireMinCount | (countDropped ? eAllowDropped : 0u) };
    }

  private:

    friend class ResponseCount;

    AgtUInt32 minDesiredResponseCount;
    AgtUInt32 flags;
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

    void reset(AgtUInt32 maxExpectedResponses) noexcept {
      AgtUInt32 capturedValue = 0;
      while ((capturedValue = _InterlockedCompareExchange(&arrivedCount_, 0, capturedValue)));

    }

    bool waitFor(ResponseQuery query, AgtTimeout timeout) const noexcept {
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

    bool waitUntil(ResponseQuery query, Deadline deadline) const noexcept {
      if (!deadline.isLong()) [[likely]] {
        return shallowWaitUntil(expectedValue, deadline);
      } else {
        return deepWaitUntil(expectedValue, deadline);
      }
    }



  private:

    bool      doDeepWait(AgtUInt32& capturedValue, AgtUInt32 timeout) const noexcept;
    void      notifyWaiters() noexcept;


    AGT_forceinline AgtUInt32 fastLoad() const noexcept {
      return __iso_volatile_load32(&reinterpret_cast<const int&>(arrivedCount_));
    }

    AGT_forceinline AgtUInt32 orderedLoad() const noexcept {
      return _InterlockedCompareExchange(&const_cast<volatile AgtUInt32&>(arrivedCount_), 0, 0);
    }

    AGT_forceinline bool      isLessThan(AgtUInt32 value) const noexcept {
      return orderedLoad() < value;
    }
    AGT_forceinline bool      isAtLeast(AgtUInt32 value) const noexcept {
      return orderedLoad() >= value;
    }

    AGT_forceinline bool      isLessThan(ResponseQuery query) const noexcept {
      // return orderedLoad() < query.;
    }
    AGT_forceinline bool      isAtLeast(ResponseQuery query) const noexcept {
      return orderedLoad() >= value;
    }

    AGT_forceinline bool      shallowWaitFor(ResponseQuery query, AgtTimeout timeout) const noexcept {
      return shallowWaitUntil(query, Deadline::fromTimeout(timeout));
    }

    AGT_forceinline bool      shallowWaitUntil(ResponseQuery query, Deadline deadline) const noexcept {
      AgtUInt32 backoff = 0;
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
      AgtUInt32 capturedValue = fastLoad();
      while (capturedValue < expectedValue) {
        doDeepWait(capturedValue, 0xFFFF'FFFF);
      }
    }

    AGT_noinline    bool      deepWaitFor(ResponseQuery query, AgtTimeout timeout) const noexcept {
      Deadline deadline = Deadline::fromTimeout(timeout);
      AgtUInt32 capturedValue = fastLoad();

      while (capturedValue < expectedValue) {
        if (!doDeepWait(capturedValue, deadline.toTimeoutMs()))
          return false;
      }
      return true;
    }

    AGT_noinline    bool      deepWaitUntil(ResponseQuery query, Deadline deadline) const noexcept {
      AgtUInt32 capturedValue = fastLoad();

      while (capturedValue < expectedValue) {
        if (!doDeepWait(capturedValue, deadline.toTimeoutMs()))
          return false;
      }
      return true;
    }



    union {
      struct {
        AgtUInt32     arrivedCount_;
        AgtUInt32     droppedCount_;
      };
      AgtUInt64       totalCount_ = 0;
    };

    AgtUInt32         expectedResponses_ = 0;
    mutable AgtUInt32 deepSleepers_ = 0;
  };*/
}


extern "C" {

struct AgtLocalAsyncData_st {
  AgtUInt32 attachedRefCount;
  AgtUInt32 waiterRefCount;
};

struct AgtAsyncData_st {
  union {
    AgtAsyncData next;                     //
    AgtSize      nextOffset;               //
  };
  AgtUInt32              currentKey;       // Current epoch
  AgtUInt32              instanceRefCount; // Total number of references to this object in memory, regardless of epoch
  AgtUInt32              epochRefCount;    // Total number of active waiters for this epoch. This data is considered "active" so long as this is non-zero.
  ReferenceCount         refCount;
  AgtUInt32              waiterRefCount;
  ReferenceCount         attachedRefCount;
  AtomicMonotonicCounter responseCount;
};

struct AgtSharedAsyncData_st {
  ObjectType type;
  ContextId  ownerCtx;
  ReferenceCount refCount;
  ReferenceCount waiterRefCount;
  AgtUInt32  currentKey;
};

struct AgtAsync_st {
  AgtContext   context;
  AgtUInt32    desiredResponseCount;
  AsyncFlags   flags;
  AgtUInt32    dataKey;
  AgtAsyncData data;
};

}


namespace {







  void        asyncDataDoReset(AgtAsyncData data, AgtUInt32& key, AgtUInt32 maxExpectedCount) noexcept {
    key = ++data->currentKey;

    data->attachedRefCount = ReferenceCount(maxExpectedCount);
    data->responseCount    = AtomicMonotonicCounter();
    data->maxResponseCount = maxExpectedCount;
  }


  bool         asyncDataResetWaiter(AgtAsyncData data, AgtUInt32& key, AgtUInt32 maxExpectedCount) noexcept {

    AgtUInt32 expectedWaiterCount = 1;
    AgtUInt32 expectedNextWaiterCount = 1;

    while ( !Impl::atomicCompareExchange(data->waiterRefCount, expectedWaiterCount, expectedNextWaiterCount) ) {
      expectedNextWaiterCount = std::max(expectedWaiterCount - 1, 1u);
    }

    if (expectedNextWaiterCount != 1)
      return false;

    asyncDataDoReset(data, key, maxExpectedCount);

    return true;
  }

  bool         asyncDataResetNonWaiter(AgtAsyncData data, AgtUInt32& key, AgtUInt32 maxExpectedCount) noexcept {

    AgtUInt32 expectedWaiterCount = 0;

    if ( !Impl::atomicCompareExchange(data->waiterRefCount, expectedWaiterCount, 1) )
      return false;

    asyncDataDoReset(data, key, maxExpectedCount);

    return true;
  }


  bool         asyncDataWait(AgtAsyncData data, AgtUInt32 expectedCount, AgtTimeout timeout) noexcept {
    if (expectedCount != 0) [[likely]] {
      if (!data->responseCount.waitFor(expectedCount, timeout))
        return false;
    }
    return true;
  }

  void         asyncDataDestroy(AgtAsyncData data, AgtContext context) noexcept {

    AGT_assert(data->waiterRefCount == 0);
    AGT_assert(data->attachedRefCount.get() == 0);

    if (!data->isShared) {
      ctxFreeAsyncData(context, data);
    }
    else {
      ctxRefFreeAsyncData(ctxGetContextById(context, data->contextId), data);
    }
  }

  AgtAsyncData createUnboundAsyncData(AgtContext context) noexcept {
    auto memory = ctxAllocAsyncData(context);

    auto data = new (memory) AgtAsyncData_st {
      .contextId        = ctxGetContextId(context),
      .refCount         = ReferenceCount(1),
      .waiterRefCount   = 0,
      .attachedRefCount = ReferenceCount(0),
      .responseCount    = AtomicMonotonicCounter(),
      .currentKey       = 0,
      .maxResponseCount = 0,
      .isShared         = false
    };

    return data;
  }

  AgtAsyncData createAsyncData(AgtContext context, AgtUInt32 maxExpectedCount) noexcept {
    auto memory = ctxAllocAsyncData(context);

    auto data = new (memory) AgtAsyncData_st {
      .contextId        = ctxGetContextId(context),
      .refCount         = ReferenceCount(1),
      .waiterRefCount   = 1,
      .attachedRefCount = ReferenceCount(maxExpectedCount),
      .responseCount    = AtomicMonotonicCounter(),
      .currentKey       = 0,
      .maxResponseCount = maxExpectedCount,
      .isShared         = false
    };

    return data;
  }



}






void         Agt::asyncDataAttach(AgtAsyncData data, AgtContext ctx, AgtUInt32& key) noexcept {
  // data->attachedRefCount.acquire();
  data->refCount.acquire();
  if (data->contextId != ctxGetContextId(ctx)) {
    data->isShared = true;
  }
  key = data->currentKey;
}

void         Agt::asyncDataDrop(AgtAsyncData data, AgtContext ctx, AgtUInt32 key) noexcept {
  if (data->currentKey == key) {
    ++data->responseCount;
  }

  if (data->refCount.release() == 0) {
    asyncDataDestroy(data, ctx);
  }
}

void         Agt::asyncDataArrive(AgtAsyncData data, AgtContext ctx, AgtUInt32 key) noexcept {
  if (data->currentKey == key) {
    ++data->responseCount;
  }

  if (data->refCount.release() == 0) {
    asyncDataDestroy(data, ctx);
  }
}











AgtContext   Agt::asyncGetContext(const AgtAsync_st* async) noexcept {
  return async->context;
}
AgtAsyncData Agt::asyncGetData(const AgtAsync_st* async) noexcept {
  return async->data;
}

void         Agt::asyncCopyTo(const AgtAsync_st* fromAsync, AgtAsync toAsync) noexcept {

}

AgtAsyncData Agt::asyncAttach(AgtAsync async, AgtSignal) noexcept {

}



void         Agt::asyncClear(AgtAsync async) noexcept {
  if ( static_cast<AgtUInt32>(async->flags & AsyncFlags::eWaiting) ) {
    --async->data->waiterRefCount;
  }

  async->flags = async->flags & eAsyncBound;
}

void         Agt::asyncDestroy(AgtAsync async) noexcept {

  if ( static_cast<AgtUInt32>(async->flags & AsyncFlags::eWaiting) ) {
    --async->data->waiterRefCount;
  }

  if ( async->data->refCount.release() == 0 ) {
    asyncDataDestroy(async->data, async->context);
  }

  delete async;
}

void         Agt::asyncReset(AgtAsync async, AgtUInt32 targetExpectedCount, AgtUInt32 maxExpectedCount) noexcept {

  const bool isWaiting = static_cast<AgtUInt32>(async->flags & AsyncFlags::eWaiting);

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

AgtStatus    Agt::asyncWait(AgtAsync async, AgtTimeout timeout) noexcept {

  if ( static_cast<AgtUInt32>(async->flags & eAsyncReady) )
    return AGT_SUCCESS;

  if ( !static_cast<AgtUInt32>(async->flags & eAsyncBound) )
    return AGT_ERROR_NOT_BOUND;

  if (asyncDataWait(async->data, async->desiredResponseCount, timeout)) {
    async->flags = async->flags | eAsyncReady;
    return AGT_SUCCESS;
  }

  return AGT_NOT_READY;
}

AgtAsync     Agt::createAsync(AgtContext context) noexcept {
  auto async = new AgtAsync_st;
  auto data = createUnboundAsyncData(context);

  async->context   = context;
  async->data      = data;
  async->dataKey   = data->currentKey;
  async->flags     = eAsyncUnbound;
  async->desiredResponseCount = 0;

  return async;
}
