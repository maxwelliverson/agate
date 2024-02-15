//
// Created by maxwe on 2023-07-28.
//

#include "ctx.hpp"

#include "init.hpp"



namespace {

  inline constinit thread_local agt_ctx_t tl_threadCtx = nullptr;


  using ctx_dtor_t = void(*)(agt_ctx_t, void* userData);

  class tls_ctx_destructor {
    ctx_dtor_t pfnDtor  = nullptr;
    void*      userData = nullptr;

  public:

    constexpr tls_ctx_destructor() = default;

    ~tls_ctx_destructor() {
      if (auto ctx = tl_threadCtx) {
        // assert( pfnDtor != nullptr );
        if (pfnDtor)
          pfnDtor(ctx, userData);
        tl_threadCtx = nullptr; // just to be sure :)
      }
    }

    void setCallback(ctx_dtor_t dtor, void* userData) noexcept {
      pfnDtor = dtor;
      this->userData = userData;
    }
  };


#if (_MSC_VER)
# define AGT_constinit_tls inline [[msvc::no_tls_guard]] thread_local
#else
# define AGT_constinit_tls inline constinit thread_local
#endif

  // constinit isn't working here on MSVC for some reason; it should. This seems to be a bug.
  // So instead, for now, we use the msvc specific attribute that disables tls guards.
  AGT_constinit_tls tls_ctx_destructor tl_ctxDtor{ };


  agt_ctx_t allocNewCtx(agt_instance_t instance, const agt_allocator_params_t* pAllocParams) noexcept {
    auto allocLookupTableSize = alloc_impl::ctx_allocator_lut_required_size(instance, pAllocParams);

    const size_t totalCtxSize = align_to<AGT_CACHE_LINE>(sizeof(agt_ctx_st) + allocLookupTableSize);

    const auto mem = instance_mem_alloc(instance, totalCtxSize, AGT_CACHE_LINE);

    std::memset(mem, 0, totalCtxSize);

    return new(mem) agt_ctx_st;
  }

}

// inline static constinit thread_local tls_ctx_destructor tl_ctxDtor;



[[nodiscard]] agt_ctx_t agt::acquire_ctx(agt_instance_t instance, const agt_allocator_params_t* pAllocParams) noexcept {
  agt_ctx_t localCtx = tl_threadCtx;

  if (localCtx != nullptr) [[likely]]
    return localCtx;

  if (!instance)
    return nullptr;

  const auto exports = instance->exports;

  agt_ctx_t newCtx = exports->_pfn_create_ctx(instance, pAllocParams);

  tl_threadCtx = newCtx;

  tl_ctxDtor.setCallback([](agt_ctx_t ctx, void* pData) {
    auto exports = static_cast<const export_table*>(pData);
    auto instance = ctx->instance;
    if ( exports->_pfn_release_ctx(ctx) )
      exports->_pfn_destroy_instance(instance, true);
  }, exports);

  return newCtx;
}

[[nodiscard]] agt_ctx_t agt::get_ctx() noexcept {
  return tl_threadCtx;
}


namespace agt {

  bool         AGT_stdcall release_ctx_private(agt_ctx_t ctx) {
    auto instance = ctx->instance;
    assert( ctx->pLocalCtxAddress );
    auto& tlVar = *ctx->pLocalCtxAddress;
    if (atomicDecrement(ctx->refCount) == 0) {
      destroy_ctx_allocator(instance, ctx->allocator);
      _aligned_free(ctx);
      tlVar = nullptr;
      return atomicDecrement(instance->refCount) == 0;
    }

    return false;
  }
  bool         AGT_stdcall release_ctx_shared(agt_ctx_t ctx) {
    return false;
  }

  agt_ctx_t    AGT_stdcall create_ctx_private(agt_instance_t instance, const agt_allocator_params_t* pAllocParams) {

    assert( instance != nullptr );

    auto ctx = allocNewCtx(instance, pAllocParams);

    ctx->refCount = 1;
    ctx->flags = 0;
    ctx->executor = nullptr;
    ctx->boundAgent = nullptr;
    ctx->currentMessage = nullptr;
    // ctx->boundFiber = nullptr;
    ctx->instance = instance;
    ctx->threadId = get_thread_id();
    ctx->exports = instance->exports;
    ctx->pLocalCtxAddress = &tl_threadCtx;
    ctx->timestampFrequencyRatio = instance->timeoutDivisor;


    atomicRelaxedIncrement(instance->refCount);

    ctx->registryCache = {};

    auto result = init_ctx_allocator(ctx->instance, ctx->allocator, pAllocParams);

    if (result != AGT_SUCCESS) {
      raise(instance, result, nullptr);
      // manager.reportError("agt_ctx_t allocator initialization failed.");
      return nullptr;
    }

    std::atomic_thread_fence(std::memory_order_release);

    // tl_threadCtx = ctx; // ???? is this was we want to do??????????

    return ctx;
  }
  agt_ctx_t    AGT_stdcall create_ctx_shared(agt_instance_t instance, const agt_allocator_params_t* pAllocParams) {
    return nullptr;
  }




  // TODO: Maybe implement reference counting for ctx?? This way, external libraries can finalize their own context handle when they're done with them without closing it for everyone else.
  /*agt_status_t AGT_stdcall finalize_ctx_private(agt_ctx_t ctx)  {
    if (!ctx) {
      ctx = tl_threadCtx;
      if (!ctx)
        return AGT_ERROR_INVALID_ARGUMENT;
    }
    tl_threadCtx = nullptr;
    destroy_ctx_private(ctx);
    return AGT_SUCCESS;
    // return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }

  agt_status_t AGT_stdcall finalize_ctx_shared(agt_ctx_t ctx)  {
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }*/
}








void *agt::ctxAcquireLocalAsyncData(agt_ctx_t ctx) noexcept {
  return nullptr;
}

void agt::ctxReleaseLocalAsyncData(agt_ctx_t ctx, void *data) noexcept {

}

void *agt::ctxAcquireSharedAsyncData(agt_ctx_t ctx, async_data_t &handle) noexcept {
  return nullptr;
}

void agt::ctxReleaseSharedAsyncData(agt_ctx_t ctx, async_data_t handle) noexcept {

}



