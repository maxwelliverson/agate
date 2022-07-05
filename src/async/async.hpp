//
// Created by maxwe on 2021-11-28.
//

#ifndef JEMSYS_AGATE2_ASYNC_HPP
#define JEMSYS_AGATE2_ASYNC_HPP

#include "fwd.hpp"

namespace agt {

  async_key_t  asyncDataAttach(async_data_t asyncData, agt_ctx_t ctx) noexcept;
  async_key_t  asyncDataGetKey(async_data_t asyncData, agt_ctx_t ctx) noexcept;
  void         asyncDataDrop(async_data_t asyncData,   agt_ctx_t ctx, async_key_t key) noexcept;
  void         asyncDataArrive(async_data_t asyncData, agt_ctx_t ctx, async_key_t key) noexcept;




  /*agt_u32_t    asyncDataGetEpoch(const AgtAsyncData_st* asyncData) noexcept;

  void         asyncDataAttach(agt_async_data_t data, agt_signal_t signal) noexcept;
  bool         asyncDataTryAttach(agt_async_data_t data, agt_signal_t signal) noexcept;

  void         asyncDataDetachSignal(agt_async_data_t data) noexcept;

  agt_u32_t    asyncDataGetMaxExpectedCount(const AgtAsyncData_st* data) noexcept;*/



  agt_ctx_t    asyncGetContext(const agt_async_t& async) noexcept;
  async_data_t asyncGetData(const agt_async_t& async) noexcept;

  void         asyncCopyTo(const agt_async_t& fromAsync, agt_async_t& toAsync) noexcept;
  void         asyncClear(agt_async_t& async) noexcept;
  void         asyncDestroy(agt_async_t& async, bool wipeMemory = true) noexcept;

  void         asyncAttachLocal(agt_async_t& async, agt_u32_t expectedCount, agt_u32_t attachedCount) noexcept;

  void         asyncAttachShared(agt_async_t& async, agt_u32_t expectedCount, agt_u32_t attachedCount) noexcept;

  agt_status_t asyncWait(agt_async_t& async, agt_timeout_t timeout) noexcept;

  // Functionally equivalent to calling "asyncWait" with a timeout of 0, but slightly optimized
  // TODO: Benchmark to see whether or not there is actually any speedup from this, or if the gains are outweighed by the optimized instruction caching
  agt_status_t asyncStatus(agt_async_t& async) noexcept;

  void         initAsync(agt_ctx_t context, agt_async_t& async, agt_async_flags_t flags) noexcept;

}

#endif//JEMSYS_AGATE2_ASYNC_HPP
