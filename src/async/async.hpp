//
// Created by maxwe on 2021-11-28.
//

#ifndef JEMSYS_AGATE2_ASYNC_HPP
#define JEMSYS_AGATE2_ASYNC_HPP

#include "fwd.hpp"

namespace agt {

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

  enum class async_key : agt_u32_t;



  void         asyncDataAttach(agt_async_data_t asyncData, agt_ctx_t ctx, async_key& key) noexcept;
  void         asyncDataDrop(agt_async_data_t asyncData,   agt_ctx_t ctx, async_key key) noexcept;
  void         asyncDataArrive(agt_async_data_t asyncData, agt_ctx_t ctx, async_key key) noexcept;




  /*agt_u32_t    asyncDataGetEpoch(const AgtAsyncData_st* asyncData) noexcept;

  void         asyncDataAttach(agt_async_data_t data, agt_signal_t signal) noexcept;
  bool         asyncDataTryAttach(agt_async_data_t data, agt_signal_t signal) noexcept;

  void         asyncDataDetachSignal(agt_async_data_t data) noexcept;

  agt_u32_t    asyncDataGetMaxExpectedCount(const AgtAsyncData_st* data) noexcept;*/






  agt_ctx_t   asyncGetContext(const agt_async_st* async) noexcept;
  agt_async_data_t asyncGetData(const agt_async_st* async) noexcept;

  void         asyncCopyTo(const agt_async_st* fromAsync, agt_async_t toAsync) noexcept;
  void         asyncClear(agt_async_t async) noexcept;
  void         asyncDestroy(agt_async_t async) noexcept;

  agt_async_data_t asyncAttach(agt_async_t async, agt_signal_t) noexcept;

  void         asyncReset(agt_async_t async, agt_u32_t targetExpectedCount, agt_u32_t maxExpectedCount) noexcept;

  agt_status_t    asyncWait(agt_async_t async, agt_timeout_t timeout) noexcept;

  agt_async_t     createAsync(agt_ctx_t context) noexcept;

}

#endif//JEMSYS_AGATE2_ASYNC_HPP
