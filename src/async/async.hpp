//
// Created by maxwe on 2021-11-28.
//

#ifndef JEMSYS_AGATE2_ASYNC_HPP
#define JEMSYS_AGATE2_ASYNC_HPP

#include "fwd.hpp"
#include "support/wrapper.hpp"

namespace Agt {

  /*enum class AsyncType {
    eUnbound,
    eDirect,
    eMessage,
    eSignal
  };
  enum class AsyncState {
    eEmpty,
    eBound,
    eReady
  };
  enum class AsyncDataState {
    eUnbound,
    eNotReady,
    eReady,
    eReadyAndRecyclable
  };*/




  void         asyncDataAttach(AgtAsyncData asyncData, AgtContext ctx, AgtUInt32& key) noexcept;
  void         asyncDataDrop(AgtAsyncData asyncData, AgtContext ctx, AgtUInt32 key) noexcept;
  void         asyncDataArrive(AgtAsyncData asyncData, AgtContext ctx, AgtUInt32 key) noexcept;




  /*AgtUInt32    asyncDataGetEpoch(const AgtAsyncData_st* asyncData) noexcept;

  void         asyncDataAttach(AgtAsyncData data, AgtSignal signal) noexcept;
  bool         asyncDataTryAttach(AgtAsyncData data, AgtSignal signal) noexcept;

  void         asyncDataDetachSignal(AgtAsyncData data) noexcept;

  AgtUInt32    asyncDataGetMaxExpectedCount(const AgtAsyncData_st* data) noexcept;*/






  AgtContext   asyncGetContext(const AgtAsync_st* async) noexcept;
  AgtAsyncData asyncGetData(const AgtAsync_st* async) noexcept;

  void         asyncCopyTo(const AgtAsync_st* fromAsync, AgtAsync toAsync) noexcept;
  void         asyncClear(AgtAsync async) noexcept;
  void         asyncDestroy(AgtAsync async) noexcept;

  AgtAsyncData asyncAttach(AgtAsync async, AgtSignal) noexcept;

  void         asyncReset(AgtAsync async, AgtUInt32 targetExpectedCount, AgtUInt32 maxExpectedCount) noexcept;

  AgtStatus    asyncWait(AgtAsync async, AgtTimeout timeout) noexcept;

  AgtAsync     createAsync(AgtContext context) noexcept;

}

#endif//JEMSYS_AGATE2_ASYNC_HPP
