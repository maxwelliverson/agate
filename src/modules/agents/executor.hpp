//
// Created by maxwe on 2022-08-07.
//

#ifndef AGATE_AGENT_EXECUTOR_HPP
#define AGATE_AGENT_EXECUTOR_HPP

#include "config.hpp"

#include "channels/message_pool.hpp"
#include "channels/message_queue.hpp"
#include "agate/set.hpp"

#include "core/object.hpp"

namespace agt {

  struct agent_self;

  AGT_virtual_object_type(executor) {
    set<executor*> linkedExecutors;
    agt_u32_t      maxAgents;
    agt_u32_t      attachedAgents;
    agt_timeout_t  timeout;
  };


  AGT_final_object_type(local_busy_executor, extends(executor)) {
    local_mpsc_receiver     receiver;
    agent_self*             agent;
    local_spmc_message_pool defaultPool;
  };

  AGT_final_object_type(local_single_thread_executor, extends(executor)) {
    local_mpsc_receiver        receiver;
    private_sized_message_pool selfPool;
    private_sender             selfSender;
    private_receiver           selfReceiver;
    local_spmc_message_pool    defaultPool; // Only this executor allocates from pool, others deallocate
    set<agent_self*>           agents;
    set<agt_executor_t>        linkedExecutors;
    agt_timeout_t              timeout;
  };

  AGT_final_object_type(local_pool_executor, extends(executor)) {

  };

  AGT_final_object_type(local_proxy_executor, extends(executor)) {

  };


  agt_status_t create_local_busy_executor(agt_ctx_t ctx, local_busy_executor*& executor) noexcept;



  struct acquire_raw_info {
    agt_raw_msg_t rawMsg;
    void*         buffer;
  };



  agt_status_t startExecutor(agt_executor_t executor) noexcept;

  bool         executorHasStarted(agt_executor_t executor) noexcept;

  std::pair<agt_status_t, agt_message_t> getMessageForSendManyFromExecutor(agt_executor_t executor, agt_agent_t sender, const agt_send_info_t& sendInfo, bool isShared) noexcept;

  void releaseMessageToExecutor(agt_executor_t executor, agt_message_t message) noexcept;

  agt_status_t sendToExecutor(agt_executor_t executor, agt_agent_t receiver, agt_agent_t sender, const agt_send_info_t& sendInfo) noexcept;

  agt_status_t sendOneOfManyToExecutor(agt_executor_t executor, agt_agent_t receiver, agt_agent_t sender, agt_message_t message) noexcept;

  agt_status_t acquireRawFromExecutor(agt_executor_t executor, agt_agent_t receiver, size_t messageSize, acquire_raw_info& rawInfo) noexcept;

  agt_status_t sendRawToExecutor(agt_executor_t executor, agt_agent_t receiver, agt_agent_t sender, const agt_raw_send_info_t& rawInfo) noexcept;

  void         delegateToExecutor(agt_executor_t executor, agt_agent_t receiver, agt_message_t message) noexcept;




  agt_status_t createDefaultExecutor(agt_ctx_t ctx, agt_executor_t& executor) noexcept;

  agt_status_t createSingleThreadExecutor(agt_ctx_t ctx, agt_executor_t& executor) noexcept;


  agt_status_t attachToExecutor(agt_executor_t executor, agt_agent_t agent) noexcept;



  agt_executor_t importExecutor(agt_ctx_t ctx, shared_handle sharedHandle) noexcept;

}


extern "C" {

struct agt_executor_st {

};


}


#endif//AGATE_AGENT_EXECUTOR_HPP
