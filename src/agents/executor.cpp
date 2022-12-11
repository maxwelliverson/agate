//
// Created by maxwe on 2022-08-07.
//

#include "executor.hpp"


namespace {

  struct local_busy_executor {
    agt::executor_kind           kind;
    agt::local_mpsc_queue        queue;
    agt::local_spmc_message_pool defaultPool;
    agt::agent_instance*         agent;
    agt::set<agt_executor_t>     linkedExecutors;
    agt_timeout_t                timeout;
  };

  struct local_single_thread_executor {
    agt::executor_kind              kind;
    agt_u32_t                       maxAgentCount;
    agt::private_sized_message_pool selfPool;
    agt::private_queue              selfQueue;
    agt::local_mpsc_queue           queue;
    agt::local_spmc_message_pool    defaultPool; // Only this executor allocates from pool, others deallocate
    agt::set<agt::agent_instance*>  agents;
    agt::set<agt_executor_t>        linkedExecutors;
    agt_timeout_t                   timeout;
  };

  struct local_pool_executor {
    agt::executor_kind              kind;
  };

  struct shared_busy_executor {
    agt::executor_kind              kind;
  };

  struct shared_single_thread_executor {
    agt::executor_kind              kind;
  };

  struct shared_pool_executor {
    agt::executor_kind              kind;
  };

  struct proxy_executor {
    agt::executor_kind              kind;
  };
}


agt_status_t agt::createSingleThreadExecutor(agt_ctx_t ctx, agt_executor_t &executor) noexcept {

}

agt_status_t agt::attachToExecutor(agt_executor_t executor, agt_agent_t agent) noexcept {

}