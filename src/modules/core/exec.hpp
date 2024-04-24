//
// Created by Maxwell on 2024-03-10.
//

#ifndef AGATE_CORE_EXEC_HPP
#define AGATE_CORE_EXEC_HPP

#include "config.hpp"


#include "agate/list.hpp"
#include "agate/set.hpp"
#include "agate/vector.hpp"
#include "core/agents.hpp"
#include "core/msg/message_pool.hpp"
#include "core/msg/message_queue.hpp"
#include "core/uexec.hpp"


#include "core/object.hpp"
#include "core/ctx.hpp"



AGT_abstract_api_object(agt_executor_st, executor, extends(agt_uexec_st)) {
  executor_flags      flags;
  executor_vptr_t     vptr;
  agt_timeout_t       timeout;
  set<agt_executor_t> linkedExecutors;
  agt_u32_t           maxAgentCount;
  agt_u32_t           attachedAgents;
};


namespace agt {

  AGT_DEFINE_BITFLAG_ENUM(executor_flags, agt_u32_t) {
    isShared              = 0x01,
    isOverlaidByProxy     = 0x40,
    hasStarted            = 0x80,
    hasIntegratedMsgQueue = 0x100, ///< set by executors to indicate that a message passed to 'send_executor_msg' must have been acquired from a paired 'acquire_executor_msg'.
  };

  inline constexpr static executor_flags eExecutorHasIntegratedMsgQueue = executor_flags::hasIntegratedMsgQueue;

  struct agent_self;

  struct equeue;
  struct basic_executor;

  struct acquire_message_info {
    agt_ecmd_t           cmd;
    msg_layout_t         layout;
    agt_send_flags_t     flags;
    agt_u32_t            bufferSize;
  };


  size_t get_min_message_size(const acquire_message_info& msgInfo) noexcept;

  struct executor_vtable {
    agt_status_t (* AGT_stdcall start)(agt_executor_t exec, bool startOnCurrentThread);
    agt_status_t (* AGT_stdcall bindAgent)(agt_executor_t exec, agt_agent_instance_t agent);
    agt_status_t (* AGT_stdcall unbindAgent)(agt_executor_t exec, agt_agent_instance_t agent);
    agt_status_t (* AGT_stdcall acquireMessage)(agt_executor_t exec, const acquire_message_info& msgInfo, agt_message_t& msg);
    void         (* AGT_stdcall releaseMessage)(agt_executor_t exec, agt_message_t msg);
    agt_status_t (* AGT_stdcall commitMessage)(agt_executor_t exec, agt_message_t msg);
  };


  AGT_forceinline static agt_status_t start_executor(agt_executor_t exec, bool startOnCurrentThread = false) noexcept {
    return exec->vptr->start(exec, startOnCurrentThread);
  }

  AGT_forceinline static agt_status_t start_executor_on_current_thread(agt_executor_t exec) noexcept {
    return exec->vptr->start(exec, true);
  }

  AGT_forceinline static agt_status_t acquire_executor_msg(agt_executor_t exec, const acquire_message_info& msgInfo, agt_message_t& msg) noexcept {
    return exec->vptr->acquireMessage(exec, msgInfo, msg);
  }

  AGT_forceinline static void         release_executor_msg(agt_executor_t exec, agt_message_t msg) noexcept {
    exec->vptr->releaseMessage(exec, msg);
  }

  AGT_forceinline static agt_status_t send_executor_msg(agt_executor_t exec, agt_message_t msg) noexcept {
    return exec->vptr->commitMessage(exec, msg);
  }



  AGT_forceinline static agt_status_t acquire_basic_executor_msg() noexcept {}

  AGT_forceinline static agt_status_t acquire_signal_executor_msg() noexcept {}


  class executor {
    agt_executor_t exec;
  public:

    executor() = default;
    executor(agt_executor_t e) noexcept : exec(e) { }

    agt_status_t start(bool startOnCurrentThread) noexcept {
      if (has_started())
        return AGT_ERROR_OBJECT_IS_BUSY;
      return exec->vptr->start(exec, startOnCurrentThread);
    }

    [[nodiscard]] bool has_started() const noexcept {
      return test(exec->flags, executor_flags::hasStarted);
    }

    [[nodiscard]] bool is_bound_to(agt_ctx_t ctx) const noexcept {
      return exec == ctx->uexec;
    }

    [[nodiscard]] bool may_execute_direct(agt_ctx_t currentCtx = get_ctx()) const noexcept {
      // FIXME: This will need some fixing, depending on how proxy executors end up being implemented,
      //        but it's a good enough approximation for now
      return is_bound_to(currentCtx) ||
        !has_started() ||
        test(exec->flags, executor_flags::isOverlaidByProxy);
    }

    [[nodiscard]] bool is_bound_to(agt_self_t agent) const noexcept {
      return executor(agent->executor) == *this;
    }

    /*agt_status_t attach_agent(agt_self_t agent, async* optionalAsync = nullptr) noexcept {
      if (is_bound_to(agent))
        return AGT_SUCCESS; // While this is kind of an error state, it's one that has the same end result as is desired.

      if (executor agentExec = agent->executor)
        return agentExec.rebind_agent(agent, *this, optionalAsync);

      if (may_execute_direct())
        return exec->vptr->bindAgent(exec, agent);

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

    agt_status_t detach_agent(agt_self_t agent, async* optionalAsync = nullptr) noexcept {
      if (!is_bound_to(agent)) // ?? This shouldn't ever be the case, but just to be safe.....
        return AGT_ERROR_AGENT_IS_DETACHED;

      assert( executor(agent->executor) == *this );

      if (may_execute_direct())
        return exec->vptr->unbindAgent(exec, agent);

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
    }*/

    // Unbinds the agent from the current executor, and subsequently rebinds it to the target executor. The operation is complete when the agent is totally rebound.
    /*agt_status_t rebind_agent(agt_self_t  agent, executor targetExec, async* optionalAsync = nullptr) noexcept {

      if (!is_bound_to(agent)) // ?? This shouldn't ever be the case, but just to be safe.....
        return AGT_ERROR_AGENT_IS_DETACHED;

      assert( executor(agent->executor) == *this );

      if (may_execute_direct()) {
        if (auto status = exec->vptr->unbindAgent(exec, agent); status != AGT_SUCCESS)
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
      // msg.get_as<agent_binding_message>()->agent = agent;
      if (optionalAsync)
        msg.bind_async_local(*optionalAsync);
      msg.write_send_time();
      const auto status = commit_message(msg);
      if (status != AGT_SUCCESS)
        drop_message(msg);
      return status;
    }
*/

    agt_status_t acquire_message(const acquire_message_info& msgInfo, agt_message_t& msg) noexcept {
      return exec->vptr->acquireMessage(exec, msgInfo, msg);
    }

    void         drop_message(agt_message_t msg) noexcept {
      exec->vptr->releaseMessage(exec, msg);
    }

    agt_status_t commit_message(agt_message_t msg) noexcept {
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
    agt_timeout_t timeout;
    agt_u32_t     maxAgentCount;
    agt_u32_t     maxFiberCount;
    agt_u32_t     initialFiberCount;
  };


  agt_status_t create_local_busy_executor(agt_ctx_t ctx, executor& executor) noexcept;

  agt_status_t create_local_event_executor(agt_ctx_t ctx, const event_executor_create_info& createInfo, executor& exec) noexcept;
}

#endif //AGATE_CORE_EXEC_HPP
