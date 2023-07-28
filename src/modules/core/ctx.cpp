//
// Created by maxwe on 2023-07-28.
//

#include "ctx.hpp"








namespace {

  inline constinit thread_local agt_ctx_t tl_threadCtx = nullptr;

  agt_ctx_t new_default_ctx(agt_instance_t instance) noexcept {

  }

  void      destroy_thread_ctx() noexcept {
    agt_instance_t instance;
    agt_ctx_t ctx = tl_threadCtx;

    if (ctx)
      instance  = agt::get_instance(ctx);
    else {
      instance  = agt::inst_get();
      if (!(ctx = agt::inst_get_thread_ctx(instance)))
        return;
    }


  }
}


[[nodiscard]] agt_ctx_t agt::acquire_ctx(agt_instance_t instance_) noexcept {
  agt_ctx_t& localCtx = tl_threadCtx;

  if (localCtx != nullptr)
    return localCtx;

  const auto instance = instance_ != nullptr ? instance_  : agt::inst_get();

  if (auto ctx = inst_get_thread_ctx(instance)) {
    localCtx = ctx;
    return ctx;
  }

  agt_ctx_t newCtx = new_default_ctx(instance);

  localCtx = newCtx;
  inst_set_thread_ctx(instance, newCtx);

  return newCtx;
}

[[nodiscard]] agt_ctx_t agt::get_ctx() noexcept {
  return tl_threadCtx;
}

agt_status_t            agt::new_ctx(agt_ctx_t& ctxRef, agt_instance_t instance, agt_executor_t executor, const agt_allocator_params_t *allocatorParams) noexcept {
  const size_t ctxStructSize = sizeof(agt_ctx_st) + alloc_impl::ctx_allocator_lut_required_size(instance, allocatorParams);
  auto ctx = (agt_ctx_t)std::malloc(ctxStructSize);
  std::memset(ctx, 0, ctxStructSize);
  auto status = alloc_impl::init_ctx_allocator(instance, ctx->allocator, allocatorParams);
  if (status != AGT_SUCCESS) {
    std::free(ctx);
    return status;
  }
  ctx->executor = executor;
  ctx->instance = instance;
  ctx->fastThreadId = agt::get_thread_id();
  ctx->exports = instance->exports;
  ctxRef = ctx;
  return AGT_SUCCESS;
}
