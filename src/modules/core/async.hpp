//
// Created by maxwe on 2021-11-28.
//

#ifndef JEMSYS_AGATE2_ASYNC_HPP
#define JEMSYS_AGATE2_ASYNC_HPP

#include "config.hpp"
#include "agate/cast.hpp"
#include "core/object.hpp"
#include "core/rc.hpp"

#include <utility>
#include <span>

namespace agt {

  AGT_BITFLAG_ENUM(async_flags, agt_u32_t) {
    eUnbound       = 0x0,
    eBound         = 0x100,
    eReady         = 0x200,
    eWaiting       = 0x400,
    eMemoryIsOwned = 0x8000,
};


  inline constexpr async_flags eAsyncNoFlags = { };
  inline constexpr async_flags eAsyncUnbound = async_flags::eUnbound;
  inline constexpr async_flags eAsyncBound   = async_flags::eBound;
  inline constexpr async_flags eAsyncReady   = async_flags::eBound | async_flags::eReady;
  inline constexpr async_flags eAsyncWaiting = async_flags::eBound | async_flags::eWaiting;
  inline constexpr async_flags eAsyncMemoryIsOwned = async_flags::eMemoryIsOwned;


  struct async {
    agt_ctx_t    ctx;
    agt_u32_t    structSize;
    async_flags  flags;
    agt_u64_t    resultValue;
    agt_status_t status;
    agt_u8_t     reserved[AGT_ASYNC_STRUCT_SIZE - 28];
  };


  AGT_virtual_object_type(async_data) {
    agt_u32_t epoch;           // key
    agt_u64_t refCount;
    agt_u64_t responseCounter;
    agt_u64_t resultValue;
  };

  AGT_final_object_type(local_async_data, extends(async_data)) {

  };

  AGT_final_object_type(shared_async_data, extends(async_data)) {

  };

  AGT_final_object_type(imported_async_data) {
    shared_async_data* ptr;
    agt_handle_t       sharedDataHandle;
  };


  template <>
  struct handle_casting<local_async_data, async_data_t> {
    AGT_forceinline static bool        isa(async_data_t handle) noexcept {
      return (static_cast<uintptr_t>(handle) & 0x1) == 0;
    }
    AGT_forceinline static local_async_data* cast(async_data_t handle) noexcept {
      return reinterpret_cast<local_async_data*>(static_cast<uintptr_t>(handle));
    }
  };
  template <>
  struct handle_casting<imported_async_data, async_data_t> {
    AGT_forceinline static bool        isa(async_data_t handle) noexcept {
      return (static_cast<uintptr_t>(handle) & 0x3) == 0x3;
    }
    AGT_forceinline static imported_async_data* cast(async_data_t handle) noexcept {
      return reinterpret_cast<imported_async_data*>(static_cast<uintptr_t>(handle) & ~0x3ULL);
    }
  };




  imported_async_data* get_imported_async_data() noexcept {

  }


  AGT_forceinline static bool async_data_is_attached(async_data_t asyncData) noexcept {
    return asyncData != async_data_t{};
  }




  async_key_t  async_data_attach(agt_ctx_t ctx, async_data_t asyncData) noexcept;
  async_key_t  async_data_get_key(agt_ctx_t ctx, async_data_t asyncData) noexcept;
  void         async_data_drop(agt_ctx_t ctx,   async_data_t asyncData, async_key_t key) noexcept;
  void         async_data_arrive(agt_ctx_t ctx, async_data_t asyncData, async_key_t key) noexcept;

  void         async_copy_to(const async& fromAsync, async& toAsync) noexcept;
  void         async_clear(async& async) noexcept;
  void         async_destroy(async& async, bool wipeMemory = true) noexcept;


  template <size_t AsyncSize>
  inline void async_init(agt_ctx_t ctx, agt_async_t& async, agt_async_flags_t flags) noexcept {

  }


  template <bool AgentsEnabled, bool SharedContext, size_t AsyncSize>
  inline agt_status_t async_wait(agt_async_t& async, agt_timeout_t timeout) noexcept {
    const auto desiredResponses = async.desiredResponseCount;

    if (!test(async.flags, agt::eAsyncBound)) [[unlikely]]
      return AGT_ERROR_NOT_BOUND;

    if (async.status != AGT_NOT_READY)
      return async.status;

    bool useLocalData;
    agt_status_t status = AGT_SUCCESS;

    if constexpr (SharedContext) {
      useLocalData = handle_isa<local_async_data>(async.data);
    }
    else {
      useLocalData = true;
    }

    if (useLocalData) {
      const auto data = unsafe_handle_cast<local_async_data>(async.data);
      AGT_assert( data->type == agt::object_type::local_async_data );


      AGT_assert( desiredResponses > 0);

      if (!atomicWaitFor(data->responseCounter, timeout, [&status, desiredResponses](agt_u64_t responses) mutable {
            const auto totalResponses = async_total_responses(responses);
            if (totalResponses >= desiredResponses) {
              if (totalResponses - async_dropped_responses(responses) < desiredResponses)
                status = AGT_ERROR_CONNECTION_DROPPED;
              return true;
            }
            return false;
          }))
      {
        status = AGT_NOT_READY;
      }
    }
    else {
      if constexpr (SharedContext) {

      }
      else {
        AGT_unreachable;
      }
    }

    async.status = status;

    // TODO: Answer question,
    //       As is, a wait timing out will return AGT_NOT_READY rather than AGT_TIMED_OUT. Is this what we want?
    //       Is there a meaningful difference between the two statuses that should be made clear?
    return status;
  }

  template <bool AgentsEnabled, bool SharedContext, size_t AsyncSize>
  inline agt_status_t async_wait_all(std::span<agt_async_t* const> asyncs, agt_timeout_t timeout) noexcept {
    switch (timeout) {
      case AGT_WAIT:
      case AGT_DO_NOT_WAIT:
      default: {
        auto deadline = deadline::fromTimeout(timeout);

      }
    }
  }







  std::pair<async_data_t, async_key_t> async_attach_shared(agt_async_t& async, agt_u32_t expectedCount, agt_u32_t attachedCount) noexcept;

  std::pair<async_data_t, async_key_t> async_attach_local(agt_async_t& async, agt_u32_t expectedCount, agt_u32_t attachedCount) noexcept;

  agt_status_t                         async_wait(agt_async_t& async, agt_timeout_t timeout) noexcept;

  // Functionally equivalent to calling "asyncWait" with a timeout of 0, but slightly optimized
  // TODO: Benchmark to see whether or not there is actually any speedup from this, or if the gains are outweighed by the optimized instruction caching
  agt_status_t async_status(agt_async_t& async) noexcept;

  void         init_async(agt_ctx_t ctx, agt_async_t& async, agt_async_flags_t flags) noexcept;

}

#endif//JEMSYS_AGATE2_ASYNC_HPP
