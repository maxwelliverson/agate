//
// Created by maxwe on 2022-12-08.
//

#ifndef AGATE_CONTEXT_THREAD_CONTEXT_HPP
#define AGATE_CONTEXT_THREAD_CONTEXT_HPP

#include "fwd.hpp"

// #include "core/objects.hpp"
#include "core/object_pool.hpp"


#include <bit>
#include <concepts>

namespace agt {


  struct thread_state {
    agt_executor_t executor;
    agt_agent_t    boundAgent;
    agt_message_t  currentMessage;
    agt_u32_t      threadId;
    // object_pool    poolObj16;
    object_pool    poolObj32;
    object_pool    poolObj64;
    object_pool    poolObj128;
    object_pool    poolObj256;
  };

  agt_status_t                 init_thread_state(agt_ctx_t globalCtx, agt_executor_t executor) noexcept;

  [[nodiscard]] agt_ctx_t      get_ctx() noexcept;

  [[nodiscard]] thread_state * get_thread_state() noexcept;

  [[nodiscard]] bool           is_agent_execution_context() noexcept;

  agt_ctx_t                    set_global_ctx(agt_ctx_t ctx) noexcept;

  thread_state *               set_thread_state(thread_state * ctx) noexcept;

  void                         set_agent_execution_context(bool isAEC = true) noexcept;

}// namespace agt

#endif//AGATE_CONTEXT_THREAD_CONTEXT_HPP
