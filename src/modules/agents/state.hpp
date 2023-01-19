//
// Created by maxwe on 2022-07-13.
//

#ifndef AGATE_AGENTS_STATE_HPP
#define AGATE_AGENTS_STATE_HPP

#include "config.hpp"

#define AGT_assert_valid_aec(msg) assert(::agt::tl_state.context != nullptr && msg)

namespace agt {

  struct thread_cache_data {
    agt_ctx_t      context;
    agt_executor_t executor;
    agt_agent_t    boundAgent;
    agt_message_t  currentMessage;
  };

  extern constinit thread_local thread_cache_data tl_state;
}

#endif//AGATE_AGENTS_STATE_HPP
