//
// Created by Maxwell on 2024-03-10.
//

#include "core/exec.hpp"


struct local_event_eagent : object {
  local_event_eagent*        next;
  agt_fiber_t                boundFiber;
  set<owner_ref<agent_self>>::iterator setPos;
  agent_self*                self;
  bool                       timedOut;
  bool                       isBlocked;
  bool                       isReady;
  agt_fiber_flags_t          fiberFlags;
  agt_timestamp_t            deadline;
};



AGT_final_object_type(local_event_executor, extends(agt::basic_executor)) {
  agt_ctx_t                  ctx;                // Reference to local context for efficient retrieval
  local_mpsc_receiver        receiver;           // Primary receiver from which new messages are obtained.
  private_sized_message_pool selfPool;           // Message pool if the need to send *new* messages to self arises
  private_sender             selfSender;         // Send messages to self
  private_receiver           selfReceiver;       // Receive messages to self (this implemented a very efficient private message queue)
  local_spmc_message_pool    defaultPool;        // Only this executor allocates from pool, others deallocate
  set<owner_ref<agent_self>> agents;             // Set of agents attached to this executor. Should maybe hold a set of eagents?
  agt_timeout_t              receiveTimeout;     // timeout waiting on message receive before trying something else.
  message                    messageToBeHandled; // A message that needs to be handled on a new fiber
  agt_u32_t                  fiberStackSize;
  agt_u32_t                  initialFiberCount;
  agt_u32_t                  maxFiberCount;
  agt_fiber_t                mainFiber;          // Ideally, I want to phase out the notion of a "main" fiber in favor of there being a fiber context, that is exited when the fibers all exit, or when fctx_exit is called.
  agt_fiber_t                currentFiber;
  vector<agt_fiber_t, 0>     freeFibers;         // These are unbound, idle fibers. Maybe there should be a maximum number of idle fibers so as to keep excess memory use low?
  flist<local_event_eagent>  readyAgents;        // This needs to be a *queue*, I think it only needs to be agt_fiber_t objects?
  flist<local_event_eagent>  timedBlockedAgents; // this should be a min heap, sorted by deadline
  flist<local_event_eagent>  blockedAgents;      // this need only be a linked list. Does this even need to be here?
  agt_fiber_param_t          fiberExitResult;    // result of the fiber exiting...
};




namespace {






  using fiber_action_t = agt_fiber_param_t;

  inline constexpr static fiber_action_t FiberActionInitialize     = 0;
  inline constexpr static fiber_action_t FiberActionPoll           = 1;
  inline constexpr static fiber_action_t FiberActionResumeSuccess  = 2;
  inline constexpr static fiber_action_t FiberActionResumeTimedOut = 3;
  inline constexpr static fiber_action_t FiberActionExecuteMessage = 4;
  inline constexpr static fiber_action_t FiberActionFallthrough    = 5;


local_event_eagent* get_ready_agent(local_event_executor* exec) noexcept {
  // TODO: Implement some shit to check whether a blocked agent is ready
  if (exec->readyAgents.empty())
    return nullptr;
  auto result = exec->readyAgents.front();
  exec->readyAgents.pop_front();
  return result;
}

agt_fiber_t get_free_fiber(local_event_executor* exec) noexcept {
  if (!exec->freeFibers.empty())
    return exec->freeFibers.pop_back_val();
  agt_fiber_t fiber;
  auto status = AGT_ctx_api(new_fiber, exec->ctx)(&fiber, local_fiber_proc, exec);
  assert( status == AGT_SUCCESS );
  return fiber;
}

agt_fiber_t get_next_fiber(local_event_executor* exec) noexcept {
  if (auto readyAgent = get_ready_agent(exec))
    return readyAgent->boundFiber;
  return get_free_fiber(exec);
}



struct fiber_proc_state {
  bool shouldClose = false;
  bool isInitialLoop = false;
  bool shouldCleanup = true;
  agt_timeout_t receiveTimeout;
  agt_status_t status = AGT_SUCCESS;
};


void block_eagent(local_event_executor* exec, local_event_eagent* agent) noexcept {

  if (agent->isReady) [[unlikely]] {
    exec->readyAgents.push_front(agent); // maybe try push back instead, depending...
    return;
  }

  if (agent->deadline != 0) {
    auto pos = exec->timedBlockedAgents.front();
    if (!pos) [[likely]] {
      exec->timedBlockedAgents.push_back(agent);
    }
    else if (agent->deadline < pos->deadline) {
      exec->timedBlockedAgents.push_front(agent);
    }
    else {
      local_event_eagent* prev;
      // walk down the list until we reach either the end of the list, or a blocked agent that has a later deadline. insert right before that point.
      do {
        prev = std::exchange(pos, pos->next);
      } while(pos && pos->deadline < agent->deadline);

      exec->timedBlockedAgents.insert(prev, agent);
    }
  }
  else {
    exec->blockedAgents.push_back(agent);
  }
  agent->isBlocked = true;
  agent->timedOut = false;
}


// currentFiberIsBlocked should be true when messages are dispatched to from within a "block" loop
void execute_message(local_event_executor* exec, fiber_proc_state& state, message msg, local_event_eagent* blockedAgent = nullptr) {
  auto ctx = exec->ctx;

  switch (msg.cmd()) {
    case AGT_ECMD_NOOP:
      break;
    case AGT_ECMD_KILL:
      state.shouldClose   = true;
      state.shouldCleanup = false;
    break;
    case AGT_ECMD_AGENT_MESSAGE:
      if (blockedAgent != nullptr) {
        auto fiber = get_free_fiber(exec);
        block_eagent(exec, blockedAgent);
        exec->messageToBeHandled = msg;
        AGT_ctx_api(fiber_jump, ctx)(fiber, FiberActionExecuteMessage);
      }
      else {
        auto agentMsg = msg.get_as<agent_message>();
        auto receiveAgent = agentMsg->receiver;
        ctx->boundAgent = receiveAgent;
        ctx->currentMessage = (agt_message_t)agentMsg;
        agentMsg->state = AGT_MSG_STATE_ACTIVE;
        receiveAgent->proc((agt_self_t)receiveAgent,
          receiveAgent->state,
          reinterpret_cast<std::byte*>(agentMsg->buffer) + agentMsg->userDataOffset,
          agentMsg->payloadSize);

        if (agentMsg->state != AGT_MSG_STATE_PINNED) [[likely]] {
          if (agentMsg->state != AGT_MSG_STATE_COMPLETE) [[likely]]
            msg.complete<false>(ctx);
          msg.release(ctx);
        }
      }
      break;

    default:
      abort();
  }
}

void        make_timed_out_agents_ready(local_event_executor* exec) noexcept {
  if (!exec->timedBlockedAgents.empty()) {
    const auto now = agt::now();
    auto from = exec->timedBlockedAgents.front();
    auto to = from;
    auto pos = from;
    uint32_t count = 0;

    while (pos->deadline <= now) {
      count += 1;
      to = pos;
      pos->timedOut = true;
      if (pos->next == nullptr)
        break;
      pos = pos->next;
    }

    // This loop ensures that the range [from, to] consists of #count agents that have timed out.

    if (count == 0)
      return;

    exec->timedBlockedAgents.pop_front(to, count);
    exec->readyAgents.push_front(from, to, count);
  }
}

















  inline constexpr alignas(64) executor_vtable local_event_executor_vtable {
    .start = [](basic_executor* exec_, bool startOnCurrentThread) -> agt_status_t {
      const auto exec = static_cast<local_event_executor*>(exec_);
      if (startOnCurrentThread) {
        set_flags(exec->flags, executor_flags::hasStarted);
        local_event_executor_proc(exec);
      }
      else {
        return AGT_ERROR_NOT_YET_IMPLEMENTED;
        // TODO: Implement starting a local_event_executor on a new thread
      }

      return AGT_SUCCESS;
    },
    .attachAgentDirect = [](basic_executor* exec_, agent_self* agent) -> agt_status_t {
      const auto exec = static_cast<local_event_executor*>(exec_);
      assert( agent->executor == nullptr );
      assert( exec->attachedAgents <= exec->maxAgentCount ); // attachedAgents should never be greater than maxAgentCount
      if (exec->attachedAgents == exec->maxAgentCount)
        return AGT_ERROR_AT_CAPACITY; // cannot attach another agent because the maximum number of agents has already been attached
      auto [pos, isNew] = exec->agents.insert_as(agent);
      assert( isNew );
      ++exec->attachedAgents;
      agent->executor = reinterpret_cast<agt_executor_t>(exec);
      advance_agent_epoch(agent);
      auto eagent = alloc<local_event_eagent>(exec->ctx);
      eagent->setPos      = pos;
      eagent->next        = nullptr;
      eagent->self        = agent;
      eagent->boundFiber  = nullptr;
      eagent->isBlocked   = false;
      eagent->isReady     = false;
      eagent->deadline    = 0;
      agent->execAgentTag = eagent;
      return AGT_SUCCESS;
    },
    .detachAgentDirect = [](basic_executor* exec_, agent_self* agent) -> agt_status_t {
      assert( exec_ != nullptr );
      assert( agent != nullptr );
      assert( agent->execAgentTag != nullptr );
      auto exec = static_cast<local_event_executor*>(exec_);
      auto eagent = static_cast<local_event_eagent*>(agent->execAgentTag);
      assert( exec->agents.contains(agent) );
      exec->agents.erase(eagent->setPos);
      --exec->attachedAgents;
      agent->executor = nullptr;
      agent->execAgentTag = nullptr;
      advance_agent_epoch(agent);
      release(eagent);
      return AGT_SUCCESS;
    },
    .blockAgent = [](basic_executor* exec_, agent_self* agent, async& async, agt_timestamp_t deadline) -> agt_status_t {
      assert( exec_ != nullptr );
      assert( agent != nullptr );
      assert( agent->execAgentTag != nullptr );
      auto exec = static_cast<local_event_executor*>(exec_);
      auto eagent = static_cast<local_event_eagent*>(agent->execAgentTag);
      exec->currentFiber->blockingAsync = (agt_async_t)&async;
      exec->currentFiber->blockedExecAgentTag = eagent;
      eagent->boundFiber = exec->currentFiber;
      agt_fiber_t       targetFiber = get_next_fiber(exec);
      auto transfer = AGT_ctx_api(fiber_switch, exec->ctx)(targetFiber, BlockFiber, 0);
      handle_fiber_transfer(transfer.source, transfer.param, exec);
      return AGT_SUCCESS;
    },
    .yield = [](basic_executor* exec_, agent_self* agent){
      assert( exec_ != nullptr );
      assert( agent != nullptr );
      assert( agent->execAgentTag != nullptr );
      auto exec = static_cast<local_event_executor*>(exec_);

      if (auto readyAgent = get_ready_agent(exec)) {
        auto transfer = AGT_ctx_api(fiber_switch, exec->ctx)(readyAgent->boundFiber, YieldFiber, AGT_FIBER_SAVE_EXTENDED_STATE);
        handle_fiber_transfer(transfer.source, transfer.param, exec);
      }
    },
    .acquireMessage = [](basic_executor* exec_, const acquire_message_info& msgInfo, message& msg) -> agt_status_t {
      assert( exec_ != nullptr );
      auto exec = static_cast<local_event_executor*>(exec_);
      msg = acquire_message(exec->ctx, &exec->defaultPool, get_min_message_size(msgInfo));
      return AGT_SUCCESS;
    },
    .releaseMessage = [](basic_executor* exec, message msg) {

    },
    .commitMessage = [](basic_executor* exec, message msg) -> agt_status_t {}
  };
}