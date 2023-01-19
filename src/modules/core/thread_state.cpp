//
// Created by maxwe on 2022-12-08.
//

#include "ctx.hpp"


#if AGT_system_windows
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif AGT_system_linux
#include <unistd.h>
#endif

#include <utility>



namespace {
  struct agt_thread_state {
    agt_ctx_t          ctx   = nullptr;
    agt::thread_state* state = nullptr;
    bool               isAEC = false;
  };

  inline constinit thread_local agt_thread_state _tl_state{};


  inline agt_u32_t _get_thread_id() noexcept {
#if AGT_system_windows
    return static_cast<agt_u32_t>(GetCurrentThreadId());
#elif AGT_system_linux
    return static_cast<agt_u32_t>(gettid());
#endif
  }
}




agt_status_t       agt::init_thread_state(agt_ctx_t globalCtx, agt_executor_t executor) noexcept {
  if (!globalCtx) [[unlikely]]
    return AGT_ERROR_INVALID_ARGUMENT;

  auto& state = _tl_state;
  state.ctx = globalCtx;

  auto threadId = _get_thread_id();

  auto threadCtx            = new thread_state{};
  threadCtx->threadId       = threadId;
  threadCtx->boundAgent     = nullptr;
  threadCtx->currentMessage = nullptr;
  threadCtx->executor       = executor;

  // 16 byte  -> 1024 slots per chunk
  // 32 byte  -> 2048 slots per chunk
  // 64 byte  -> 1024 slots per chunk
  // 128 byte -> 128 slots per chunk
  // 256 byte -> 64 slots per chunk

  // init_pool(globalCtx, threadId, &threadCtx->poolObj16,  16,  1024);
  init_pool(globalCtx, threadId, &threadCtx->poolObj32,  32,  2048);
  init_pool(globalCtx, threadId, &threadCtx->poolObj64,  64,  1024);
  init_pool(globalCtx, threadId, &threadCtx->poolObj128, 128, 128);
  init_pool(globalCtx, threadId, &threadCtx->poolObj256, 256, 64);

  return AGT_SUCCESS;
}


agt_ctx_t          agt::get_ctx() noexcept {
  return _tl_state.ctx;
}

agt::thread_state* agt::get_thread_state() noexcept {
  return _tl_state.state;
}

agt_ctx_t          agt::set_global_ctx(agt_ctx_t ctx) noexcept {
  return std::exchange(_tl_state.ctx, ctx);
}

agt::thread_state* agt::set_thread_state(thread_state * ctx) noexcept {
  return std::exchange(_tl_state.state, ctx);
}

bool               agt::is_agent_execution_context() noexcept {
  return _tl_state.isAEC;
}

void               agt::set_agent_execution_context(bool isAEC) noexcept {
  _tl_state.isAEC = isAEC;
}