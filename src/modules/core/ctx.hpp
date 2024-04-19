//
// Created by maxwe on 2022-12-08.
//

#ifndef AGATE_CONTEXT_THREAD_CONTEXT_HPP
#define AGATE_CONTEXT_THREAD_CONTEXT_HPP

#include "config.hpp"

#include "agate/align.hpp"
#include "error.hpp"
#include "instance.hpp"
#include "agate/integer_division.hpp"



#include <bit>
#include <type_traits>
#include <concepts>
#include <array>
#include <algorithm>


namespace agt {

  struct fctx;

  enum {
    CTX_FIBERS_ARE_LOCKED = 0x10,
    CTX_HAS_USER_UEXEC    = 0x100,
  };


  // If a context was already initialized on the current thread, return it. Otherwise, create a new one with the given params (may be null), initialize, and return it.
  [[nodiscard]] agt_ctx_t    acquire_ctx(agt_instance_t instance, const agt_allocator_params_t* pAllocParams) noexcept;

  // Returns nullptr if ctx has not been initialized
  // Should only really be called if it is already known that the local context has been initialized within the calling module
  // Note that this is specific to the calling module
  [[nodiscard]] agt_ctx_t    get_ctx() noexcept;




  [[nodiscard]] inline agt_instance_t get_instance(agt_ctx_t ctx) noexcept;

  [[nodiscard]] inline bool           is_agent_execution_context(agt_ctx_t ctx) noexcept;

  [[nodiscard]] inline agt_executor_t get_executor(agt_ctx_t ctx) noexcept;

  [[nodiscard]] inline agt_self_t     get_bound_agent(agt_ctx_t ctx) noexcept;

  [[nodiscard]] inline agt_message_t  get_current_message(agt_ctx_t ctx) noexcept;




}// namespace agt

extern "C" {

struct agt_ctx_st {
  agt_u32_t                      refCount;
  agt_flags32_t                  flags;
  agt_uexec_t                    uexec;
  const agt_uexec_vtable_t*      uexecVPtr;
  agt_ctxexec_t                  ctxexec;
  agt_utask_t                    task; // used by default uexec or by agent executors. Not used by user defined uexecs.
  // agt_executor_t                 executor;
  // agt_self_t                     boundAgent;
  // agt_message_t                  currentMessage;
  agt::fctx*                     fctx; // null if this thread isn't using fibers
  agt_instance_t                 instance;
  uintptr_t                      threadId;
  agt::impl::hazptr_ctx_cache    hazptrCache;
  bool                           fibersAreLocked;
  agt_ctx_t*                     pLocalCtxAddress;
  const agt::export_table*       exports;
  agt::reg_impl::registry_cache  registryCache;
  agt::alloc_impl::ctx_allocator allocator;
};

}


template<size_t Size>
agt::impl::sized_pool& agt::impl::get_ctx_pool(agt_ctx_t ctx) noexcept {
  // constexpr static size_t PoolIndex = alloc_impl::get_pool_index(Size);
  AGT_invariant( ctx != nullptr );

#if defined(AGT_STATIC_ALLOCATOR_SIZES)
  static_assert( Size <= alloc_impl::MaxAllocSize, "Size parameter to get_ctx_pool<Size>(ctx) exceeds the maximum possible allocation size" );
#else
  if ( Size > alloc_impl::get_max_alloc_size(ctx->allocator) ) [[unlikely]] {
    // TODO: throw error of some sort....
    abort();
  }
#endif
  return *alloc_impl::get_ctx_pool_unchecked(ctx->allocator, Size);
}

agt::impl::sized_pool& agt::impl::get_ctx_pool_dyn(agt_ctx_t ctx, size_t size) noexcept {
  // constexpr static size_t PoolIndex = alloc_impl::get_pool_index(Size);
  AGT_invariant( ctx != nullptr );
  if ( size > alloc_impl::get_max_alloc_size(ctx->allocator) ) [[unlikely]] {
    // TODO: throw error of some sort....
    abort();
  }
  return *alloc_impl::get_ctx_pool_unchecked(ctx->allocator, size);
}



AGT_forceinline agt_instance_t agt::get_instance(agt_ctx_t ctx) noexcept {
  return ctx->instance;
}


namespace agt {
  inline static void lock_fibers(agt_ctx_t ctx) noexcept {
    ctx->flags |= CTX_FIBERS_ARE_LOCKED;
  }

  inline static void unlock_fibers(agt_ctx_t ctx) noexcept {
    ctx->flags &= ~CTX_FIBERS_ARE_LOCKED;
  }

  inline static bool fibers_are_locked(agt_ctx_t ctx) noexcept {
    return ctx->flags & CTX_FIBERS_ARE_LOCKED;
  }


  inline static bool         yield(agt_ctx_t ctx) noexcept {
    const auto yield = ctx->uexecVPtr->yield;
    if (!yield)
      return false;
    return yield(ctx, ctx->ctxexec);
  }

  inline static void         suspend(agt_ctx_t ctx) noexcept {
    ctx->uexecVPtr->suspend(ctx, ctx->ctxexec);
  }

  inline static agt_status_t suspend_for(agt_ctx_t ctx, agt_timeout_t timeout) noexcept {
    return ctx->uexecVPtr->suspend_for(ctx, ctx->ctxexec, timeout);
  }
}




/*
AGT_forceinline agt_executor_t agt::get_executor(agt_ctx_t ctx) noexcept {
  return ctx->executor;
}

AGT_forceinline agt_self_t agt::get_bound_agent(agt_ctx_t ctx) noexcept {
  return ctx->boundAgent;
}
*/

/*AGT_forceinline agt_message_t  agt::get_current_message(agt_ctx_t ctx) noexcept {
  return ctx->currentMessage;
}*/



// Use if the current function returns an agt_status_t
#define AGT_try_resolve_ctx(_ctx)        \
  do { if (_ctx == AGT_CURRENT_CTX) {    \
    _ctx = agt::get_ctx();               \
    if (!_ctx)                           \
      return AGT_ERROR_CTX_NOT_ACQUIRED; \
  } } while(false)

#define AGT_resolve_ctx(_ctx)            \
  do { if (_ctx == AGT_CURRENT_CTX) {    \
    _ctx = agt::get_ctx();               \
    AGT_invariant( _ctx != nullptr );    \
  } } while(false)

#endif//AGATE_CONTEXT_THREAD_CONTEXT_HPP
