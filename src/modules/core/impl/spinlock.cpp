//
// Created by maxwe on 2024-04-01.
//

#include "spinlock.hpp"

#include "core/ctx.hpp"
#include "core/exec.hpp"

void agt::impl::spinlock_yield() noexcept {
  const auto ctx = get_ctx();
  agt::yield_executor(ctx->executor, ctx->boundAgent);
}