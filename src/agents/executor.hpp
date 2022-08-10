//
// Created by maxwe on 2022-08-07.
//

#ifndef AGATE_AGENT_EXECUTOR_HPP
#define AGATE_AGENT_EXECUTOR_HPP

#include "fwd.hpp"

#include "channels/message_pool.hpp"
#include "channels/message_queue.hpp"

#include "support/set.hpp"

namespace agt {

  enum executor_kind {
    busy_executor,
    single_thread_executor,
    thread_pool_executor,

  };

  struct executor_instance {

  };


  struct local_executor {
    executor_kind              kind;
    private_sized_message_pool selfPool;
    private_queue              selfQueue;
    local_mpsc_queue           queue;
    local_spmc_message_pool    defaultPool; // Only this executor allocates from pool, others deallocate
    set<agent_instance*>       agents;
    set<local_executor*>       linkedExecutors;
    agt_timeout_t              timeout;
  };





}


#endif//AGATE_AGENT_EXECUTOR_HPP
