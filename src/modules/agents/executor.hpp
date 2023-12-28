//
// Created by maxwe on 2022-08-07.
//

#ifndef AGATE_AGENT_EXECUTOR_HPP
#define AGATE_AGENT_EXECUTOR_HPP

#include "agents.hpp"
#include "config.hpp"

#include "channels/message_pool.hpp"
#include "channels/message_queue.hpp"
#include "agate/list.hpp"
#include "agate/set.hpp"
#include "agate/vector.hpp"

#include "core/object.hpp"


#include <core/ctx.hpp>

namespace agt {


  AGT_BITFLAG_ENUM(executor_flags, agt_u32_t) {
    isShared          = 0x01,
    isOverlaidByProxy = 0x40,
    hasStarted        = 0x80,
  };

  struct agent_self;

  struct equeue;
  struct basic_executor;

  struct acquire_message_info {
    agt_ecmd_t       cmd;
    msg_layout_t     layout;
    agt_send_flags_t flags;
    agt_u32_t        bufferSize;
    agent_self*      sender;
  };


  size_t get_min_message_size(const acquire_message_info& msgInfo) noexcept;

  struct executor_vtable {
    agt_status_t (* AGT_stdcall start)(basic_executor* exec, bool startOnCurrentThread);
    agt_status_t (* AGT_stdcall attachAgentDirect)(basic_executor* exec, agent_self* agent);
    agt_status_t (* AGT_stdcall detachAgentDirect)(basic_executor* exec, agent_self* agent);
    agt_status_t (* AGT_stdcall blockAgent)(basic_executor* exec, agent_self* agent, async& async);
    agt_status_t (* AGT_stdcall blockAgentUntil)(basic_executor* exec, agent_self* agent, async& async, agt_timestamp_t deadline);
    void         (* AGT_stdcall yield)(basic_executor* exec, agent_self* agent);
    agt_status_t (* AGT_stdcall acquireMessage)(basic_executor* exec, const acquire_message_info& msgInfo, message& msg);
    void         (* AGT_stdcall releaseMessage)(basic_executor* exec, message msg);
    agt_status_t (* AGT_stdcall commitMessage)(basic_executor* exec, message msg);
  };

  using executor_vptr_t = const executor_vtable*;


  struct basic_executor : object {
    executor_flags       flags;
    executor_vptr_t      vptr;
    agt_timeout_t        timeout;
    set<basic_executor*> linkedExecutors;
    agt_u32_t            maxAgentCount;
    agt_u32_t            attachedAgents;
  };

  template <>
  struct impl::obj_types::object_type_range<basic_executor> {
    inline constexpr static object_type minValue = object_type::executor_begin;
    inline constexpr static object_type maxValue = object_type::executor_end;
  };

  /*AGT_virtual_object_type(executor) {
    set<executor*> linkedExecutors;
    agt_u32_t      maxAgents;
    agt_u32_t      attachedAgents;
    agt_timeout_t  timeout;
  };*/


  AGT_final_object_type(local_user_executor, extends(basic_executor)) {
    const agt_executor_vtable_t* userVTable;
    agt_equeue_t                 equeue;
    agt_receiver_t               receiver;
    void*                        userData;
  };


  AGT_final_object_type(local_busy_executor, extends(basic_executor)) {
    local_mpsc_receiver     receiver;
    agent_self*             agent;
    local_spmc_message_pool defaultPool;
  };


  struct local_event_eagent;

  AGT_final_object_type(local_event_executor, extends(basic_executor)) {
    agt_ctx_t                  ctx;
    local_mpsc_receiver        receiver;
    private_sized_message_pool selfPool;
    private_sender             selfSender;
    private_receiver           selfReceiver;
    local_spmc_message_pool    defaultPool; // Only this executor allocates from pool, others deallocate
    set<owner_ref<agent_self>> agents;
    agt_u32_t                  initialFiberCount;
    agt_u32_t                  maxFiberCount;
    agt_fiber_t                mainFiber;
    agt_fiber_t                currentFiber;
    vector<agt_fiber_t, 0>     freeFibers;
    flist<local_event_eagent>  readyAgents;
    flist<local_event_eagent>  timedBlockedAgents;
    flist<local_event_eagent>  blockedAgents;
  };

  AGT_final_object_type(local_parallel_executor, extends(basic_executor)) {

  };

  AGT_final_object_type(local_proxy_executor, extends(basic_executor)) {

  };


  class executor {
    basic_executor* exec;
  public:

    executor() = default;
    executor(basic_executor* e) noexcept : exec(e) { }
    executor(agt_executor_t e) noexcept : exec(reinterpret_cast<basic_executor*>(e)) { }

    agt_status_t start(bool startOnCurrentThread) noexcept {
      if (has_started())
        return AGT_ERROR_OBJECT_IS_BUSY;
      return exec->vptr->start(exec, startOnCurrentThread);
    }

    [[nodiscard]] bool has_started() const noexcept {
      return test(exec->flags, executor_flags::hasStarted);
    }

    [[nodiscard]] bool is_bound_to_current_thread() const noexcept {
      return *this == get_ctx()->executor;
    }

    [[nodiscard]] bool may_execute_direct() const noexcept {
      // FIXME: This will need some fixing, depending on how proxy executors end up being implemented,
      //        but it's a good enough approximation for now
      return is_bound_to_current_thread() ||
        !has_started() ||
        test(exec->flags, executor_flags::isOverlaidByProxy);
    }

    [[nodiscard]] bool is_bound_to(agent_self* agent) const noexcept {
      return executor(agent->executor) == *this;
    }

    agt_status_t attach_agent(agent_self* agent, async* optionalAsync = nullptr) noexcept {
      if (is_bound_to(agent))
        return AGT_SUCCESS; // While this is kind of an error state, it's one that has the same end result as is desired.

      if (executor agentExec = agent->executor)
        return agentExec.rebind_agent(agent, *this, optionalAsync);

      if (may_execute_direct())
        return exec->vptr->attachAgentDirect(exec, agent);

      message msg;
      acquire_message_info msgInfo;

      msgInfo.cmd   = AGT_ECMD_ATTACH_AGENT;
      msgInfo.flags = 0;

      if (const auto status = acquire_message(msgInfo, msg); status != AGT_SUCCESS)
        return status;
      msg.get_as<agent_binding_message>()->agent = agent;
      if (optionalAsync)
        msg.bind_async_local(*optionalAsync);
      msg.write_send_time();
      const auto status = commit_message(msg);
      if (status != AGT_SUCCESS)
        drop_message(msg);
      return status;
    }

    agt_status_t detach_agent(agent_self* agent, async* optionalAsync = nullptr) noexcept {
      if (!is_bound_to(agent)) // ?? This shouldn't ever be the case, but just to be safe.....
        return AGT_ERROR_AGENT_IS_DETACHED;

      assert( executor(agent->executor) == *this );

      if (may_execute_direct())
        return exec->vptr->detachAgentDirect(exec, agent);

      message msg;
      acquire_message_info msgInfo;

      msgInfo.cmd   = AGT_ECMD_DETACH_AGENT;
      msgInfo.flags = 0;

      if (const auto status = acquire_message(msgInfo, msg); status != AGT_SUCCESS)
        return status;
      auto agentMsg = msg.get_as<agent_binding_message>();
      agentMsg->agent = agent;
      msg.get_as<agent_binding_message>()->agent = agent;
      if (optionalAsync)
        msg.bind_async_local(*optionalAsync);
      msg.write_send_time();
      const auto status = commit_message(msg);
      if (status != AGT_SUCCESS)
        drop_message(msg);
      return status;
    }

    // Unbinds the agent from the current executor, and subsequently rebinds it to the target executor. The operation is complete when the agent is totally rebound.
    agt_status_t rebind_agent(agent_self* agent, executor targetExec, async* optionalAsync = nullptr) noexcept {

      if (!is_bound_to(agent)) // ?? This shouldn't ever be the case, but just to be safe.....
        return AGT_ERROR_AGENT_IS_DETACHED;

      assert( executor(agent->executor) == *this );

      if (may_execute_direct()) {
        if (auto status = exec->vptr->detachAgentDirect(exec, agent); status != AGT_SUCCESS)
          return status;
        assert( agent->executor == nullptr ); // detachAgentDirect *should* set this field to null, and if it does not, an infinite loop would occur, so make sure this doesn't happen...
        return targetExec.attach_agent(agent, optionalAsync);
      }

      message msg;
      acquire_message_info msgInfo;

      msgInfo.cmd   = AGT_ECMD_REBIND_AGENT;
      msgInfo.flags = 0;

      if (const auto status = acquire_message(msgInfo, msg); status != AGT_SUCCESS)
        return status;
      auto agentMsg = msg.get_as<agent_binding_message>();
      agentMsg->agent = agent;
      agentMsg->targetExecutor = targetExec.c_type();
      msg.get_as<agent_binding_message>()->agent = agent;
      if (optionalAsync)
        msg.bind_async_local(*optionalAsync);
      msg.write_send_time();
      const auto status = commit_message(msg);
      if (status != AGT_SUCCESS)
        drop_message(msg);
      return status;
    }

    agt_status_t block_agent(agent_self* agent, async& async) noexcept {
      return exec->vptr->blockAgent(exec, agent, async);
    }

    agt_status_t block_agent_until(agent_self* agent, async& async, agt_timestamp_t deadline) noexcept {
      return exec->vptr->blockAgentUntil(exec, agent, async, deadline);
    }

    void         yield(agent_self* agent) noexcept {
      exec->vptr->yield(exec, agent);
    }

    agt_status_t acquire_message(const acquire_message_info& msgInfo, message& msg) noexcept {
      return exec->vptr->acquireMessage(exec, msgInfo, msg);
    }

    void         drop_message(message msg) noexcept {
      exec->vptr->releaseMessage(exec, msg);
    }

    agt_status_t commit_message(message msg) noexcept {
      return exec->vptr->commitMessage(exec, msg);
    }

    [[nodiscard]] agt_executor_t c_type() const noexcept {
      return reinterpret_cast<agt_executor_t>(exec);
    }


    explicit operator bool() const noexcept {
      return exec != nullptr;
    }

    [[nodiscard]] bool operator==(const executor &) const noexcept = default;
  };


  struct event_executor_create_info {
    agt_u32_t maxAgentCount;
    agt_u32_t maxFiberCount;
    agt_u32_t initialFiberCount;
  };


  agt_status_t create_local_busy_executor(agt_ctx_t ctx, local_busy_executor*& executor) noexcept;

  agt_status_t create_local_event_executor(agt_ctx_t ctx, const event_executor_create_info& createInfo, local_event_executor*& exec) noexcept;







  // void         executor_attach_agent() noexcept;
  // void         executor_detach_agent() noexcept;


  /*agt_status_t executor_start(executor* exec) noexcept;

  bool         executor_has_started(executor* exec) noexcept;

  agt_status_t executor_block_agent(executor* exec, agt_agent_t agent, async& async) noexcept;
  agt_status_t executor_block_agent_until(executor* exec, agt_agent_t agent, async& async, agt_timestamp_t deadline) noexcept;

  void         executor_yield(executor* exec, agt_agent_t agent) noexcept;

  agt_status_t executor_acquire_message(executor* exec, const acquire_message_info& msgInfo, message*& msg) noexcept;

  void         executor_drop_message(executor* exec, message* msg) noexcept;

  agt_status_t executor_commit_message(executor* exec, message* msg) noexcept;*/

  /*agt_status_t startExecutor(agt_executor_t executor) noexcept;

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



  agt_executor_t importExecutor(agt_ctx_t ctx, shared_handle sharedHandle) noexcept;*/

}


extern "C" {

struct agt_executor_st {

};


}


#endif//AGATE_AGENT_EXECUTOR_HPP
