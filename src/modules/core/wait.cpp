//
// Created by maxwe on 2022-12-20.
//

#include "config.hpp"
#include "agate/align.hpp"
#include "core/wait.hpp"

#include "core/instance.hpp"
#include "core/exec.hpp"

#include <bit>


namespace {
  AGT_forceinline bool is_valid_size(size_t size) noexcept {
    return is_pow2(size) && size <= 8;
  }

  agt_status_t do_wait(agt_ctx_t ctx, const void* address, agt_timeout_t timeout, agt::impl::monitor_waiter & monitorExec) noexcept {
    const auto instance = ctx->instance;
    const auto exec = ctx->ctxexec;


    impl::attach_monitor(ctx, instance->monitorList, address, monitorExec);


    if (timeout == AGT_WAIT) {
      suspend(ctx);
      return AGT_SUCCESS;
    }

    agt_status_t status = suspend_for(ctx, timeout);

    // if status == AGT_SUCCESS
    if (status != AGT_SUCCESS) {
      if (!impl::remove_monitor(ctx, monitorExec) && status == AGT_TIMED_OUT) // If it fails to remove the monitor, that means it was *already* removed, in which case this was actually a *success*, even though it may have reached the timeout. If the status code is something else, there was an actual error, and we wish to report this back.
        status = AGT_SUCCESS;
    }

    return status;
  }
}




agt_status_t agt::impl::wait_on_local_address_nocheck(agt_ctx_t ctx, const void* address, agt_timeout_t timeout, agt_u64_t value, size_t valueSize) noexcept {
  // NOTE: We *don't* do the initial check here to short circuit,
  // cause we do that in the typed wrapper functions. This is so
  // that the initial comparison can be optimized to 1 to 2 instructions,
  // rather than this whole type-erased machinery that is not actually
  // needed given that the callers know what type this is being called on.
  // We do still offer a type-erased wait_on_local_address overload that *does*
  // do all the comparisons in case it is needed.

  AGT_try_resolve_ctx(ctx);

  AGT_invariant( is_valid_size(valueSize) );
  AGT_invariant( ctx != nullptr );

  impl::monitor_waiter monitorExec;
  impl::init_monitor(ctx, monitorExec, valueSize, value);
  return do_wait(ctx, address, timeout, monitorExec);
}

agt_status_t agt::wait_on_local_address(agt_ctx_t ctx, const void* address, agt_timeout_t timeout, const void* cmpValue, size_t valueSize) noexcept {
  agt_u64_t atomicValue;
  agt_u64_t cmp;

  switch (valueSize) {
    case 8:
      atomicValue = atomic_relaxed_load(*static_cast<const agt_u64_t*>(address));
      cmp         = *static_cast<const agt_u64_t*>(cmpValue);
      break;
    case 4:
      atomicValue = atomic_relaxed_load(*static_cast<const agt_u32_t*>(address));
      cmp         = *static_cast<const agt_u32_t*>(cmpValue);
      break;
    case 2:
      atomicValue = atomic_relaxed_load(*static_cast<const agt_u16_t*>(address));
      cmp         = *static_cast<const agt_u16_t*>(cmpValue);
      break;
    case 1:
      atomicValue = atomic_relaxed_load(*static_cast<const agt_u8_t*>(address));
      cmp         = *static_cast<const agt_u8_t*>(cmpValue);
      break;
    AGT_no_default;
  }

  if (atomicValue != cmp) // a simple comparison here suffices; If valueSize is less than 8, the above loads will have zero-extended to fill the 64 bit value, meaning equality comparisons still hold.
    return AGT_SUCCESS;
  else if (timeout == 0)
    return AGT_NOT_READY;

  impl::monitor_waiter monitorExec;
  impl::init_monitor(ctx, monitorExec, valueSize, cmp);
  return do_wait(ctx, address, timeout, monitorExec);
}

agt_status_t agt::impl::wait_on_local_address_nocheck(agt_ctx_t ctx, const void* address, agt_timeout_t timeout, address_predicate_t predicate, void* userData) noexcept {

  if (ctx == AGT_CURRENT_CTX) {
    ctx = agt::get_ctx();
    if (!ctx)
      return AGT_ERROR_CTX_NOT_ACQUIRED;
  }

  AGT_invariant( ctx != nullptr );
  AGT_invariant( predicate != nullptr ); // userData *is* optional though

  impl::monitor_waiter monitorExec;
  impl::init_monitor(ctx, monitorExec, predicate, userData);
  return do_wait(ctx, address, timeout, monitorExec);
}

agt_status_t agt::wait_on_local_address(agt_ctx_t ctx, const void* address, agt_timeout_t timeout, address_predicate_t predicate, void* userData) noexcept {
  AGT_invariant( ctx != nullptr );
  AGT_invariant( predicate != nullptr ); // userData *is* optional though

  if (predicate(ctx, address, userData))
    return AGT_SUCCESS;
  else if (timeout == 0) // if timeout is 0, simply return the result of the predicate as is.
    return AGT_NOT_READY;

  impl::monitor_waiter monitorExec;
  impl::init_monitor(ctx, monitorExec, predicate, userData);
  return do_wait(ctx, address, timeout, monitorExec);
}



void agt::wake_single_local(agt_ctx_t ctx, const void* address) noexcept {
  AGT_invariant( ctx != nullptr );
  const auto instance = ctx->instance;
  impl::wake_one_on_monitored_address(ctx, instance->monitorList, address);
}

void agt::wake_all_local(agt_ctx_t ctx, const void* address) noexcept {
  AGT_invariant( ctx != nullptr );
  const auto instance = ctx->instance;
  impl::wake_all_on_monitored_address(ctx, instance->monitorList, address);
}