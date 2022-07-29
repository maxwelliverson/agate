//
// Created by maxwe on 2022-07-13.
//

#ifndef AGATE_AGENTS_STATE_HPP
#define AGATE_AGENTS_STATE_HPP

#include "fwd.hpp"


namespace agt {

  struct thread_cache_data {
    agt_ctx_t       context;
    agent_instance* boundAgent;
    agt_message_t   currentMessage;
    agt_agent_t     proxyAgent;
  };

  extern constinit thread_local thread_cache_data tl_state;
}

#endif//AGATE_AGENTS_STATE_HPP
