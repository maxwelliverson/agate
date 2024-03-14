//
// Created by Maxwell on 2024-03-10.
//

#include "core/exec.hpp"

#include "core/fiber.hpp"


AGT_final_object_type(local_event_eagent) {
  local_event_eagent*        next;
  agt_fiber_t                boundFiber;
  set<agent_self*>::iterator setPos;
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
  set<agent_self*>           agents;             // Set of agents attached to this executor. Should maybe hold a set of eagents?
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



  agt_fiber_param_t local_fiber_proc(agt_fiber_t fromFiber, agt_fiber_param_t action, void* userData);






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




  void init_fibers(local_event_executor* exec) noexcept {
    auto ctx = agt::get_ctx();
    auto self = agt::current_fiber();

    assert( self != nullptr );

    const auto initialFiberCount = exec->initialFiberCount;


    if (initialFiberCount) {
      exec->freeFibers.reserve(initialFiberCount - 1);

      const auto make_new_fiber = AGT_ctx_api(new_fiber, ctx);

      for (agt_u32_t i = 0; i < initialFiberCount - 1; ++i) {
        agt_fiber_t newFiber;
        auto result = make_new_fiber(&newFiber, local_fiber_proc, exec);
        if (result != AGT_SUCCESS) {
          // TODO: Handle failure to successfully create fibers
        }
        else {
          exec->freeFibers.push_back(newFiber);
        }
      }
    }
  }




  bool try_poll_message(local_event_executor* exec, fiber_proc_state& state, local_event_eagent* blockedAgent = nullptr) noexcept {
    message msg;
    switch (auto result = receive(exec->receiver, msg, state.receiveTimeout)) {
      case AGT_SUCCESS:
        execute_message(exec, state, msg, blockedAgent);
        state.receiveTimeout = exec->receiveTimeout;
        return true;
      case AGT_NOT_READY:
      case AGT_TIMED_OUT:
        state.receiveTimeout *= 2;
        return false;
      case AGT_ERROR_NO_SENDERS:
        state.shouldClose = true;
        state.status = AGT_ERROR_NO_SENDERS;
        return false;
        // then maybe disconnect??
      default:
        state.shouldCleanup = false; // ???
        state.shouldClose = true;
        state.status = result;
        return false;
        // ehhhhhhhhh
      }
  }

  [[noreturn]] void continue_ready_agent(local_event_executor* exec, local_event_eagent* readyAgent) noexcept {
    agt_fiber_param_t action;
    if (readyAgent->timedOut)
      action = FiberActionResumeTimedOut;
    else
      action = FiberActionResumeSuccess;

    AGT_ctx_api(fiber_jump, exec->ctx)(readyAgent->boundFiber, action);
  }


  agt_fiber_param_t local_fiber_proc(agt_fiber_t fromFiber, agt_fiber_param_t action, void* userData) {
    const auto exec = static_cast<local_event_executor*>(userData);

    fiber_proc_state state{  };

    state.receiveTimeout = exec->receiveTimeout;

    switch (action) {
      case FiberActionInitialize:
        state.isInitialLoop = true;
        init_fibers(exec);
        break;
      case FiberActionExecuteMessage:
        execute_message(exec, state, exec->messageToBeHandled);
        break;
      case FiberActionPoll:
        break;
      default:
        raise(AGT_ERROR_INVALID_ARGUMENT, &action, exec->ctx);
    }

    do {

      if (auto readyAgent = get_ready_agent(exec)) {
        exec->freeFibers.push_back(current_fiber());
        continue_ready_agent(exec, readyAgent);
      }

      try_poll_message(exec, state); // may ignore return value?

      make_timed_out_agents_ready(exec);
    } while(!state.shouldClose);


    if (state.shouldCleanup) {
      // blehhhhhhh
    }

  }



  void local_event_executor_proc(local_event_executor* exec) noexcept {
    auto ctx = exec->ctx;
    ctx->executor = (agt_executor_t)exec;

    agt_fctx_desc_t fctxDesc{
      .flags = 0,
      .stackSize = exec->fiberStackSize,
      .maxFiberCount = exec->maxAgentCount,
      .parent = nullptr,
      .proc = local_fiber_proc,
      .initialParam = FiberActionPoll,
      .userData = exec
    };

    int exitCode = 0;

    auto result = AGT_ctx_api(enter_fctx, ctx)(ctx, &fctxDesc, &exitCode);

    if (result != AGT_SUCCESS) {
      err::fiber_exit_info exitInfo{
        .returnCode = result,
        .exitCode = exitCode,
        .name = {}
      };
      AGT_raise(ctx, exitInfo);
    }

  }






agt_status_t start(basic_executor* exec_, bool startOnNewThread) noexcept {
  const auto exec = static_cast<local_event_executor*>(exec_);
  auto ctx = get_ctx();
  if (!startOnNewThread) {
    exec->ctx = ctx;
    set_flags(exec->flags, executor_flags::hasStarted);
    local_event_executor_proc(exec);
  }
  else {
    // TODO: Generate description string, that'll use the executor's name unless it is anonymous.

    agt_thread_desc_t threadDesc{
      .flags = 0,
      .stackSize = exec->fiberStackSize,
      .priority = AGT_PRIORITY_NORMAL,
      .description = {
        .data = nullptr,
        .length = 0,
      },
      .proc = [](agt_ctx_t ctx, void* userData) -> agt_u64_t {
        const auto exec = static_cast<local_event_executor*>(userData);
        exec->ctx = ctx; // bind the actual ctx now that it's started.
        local_event_executor_proc(exec);
        return exec->fiberExitResult;
      },
      .userData = exec
    };

    return AGT_ctx_api(new_thread, ctx)(ctx, nullptr, &threadDesc);
  }

  return AGT_SUCCESS;
}

agt_status_t bind_agent(basic_executor* exec_, agent_self* agent) noexcept {
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
}

agt_status_t unbind_agent(basic_executor* exec_, agent_self* agent) noexcept {
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
}

agt_status_t block(basic_executor* exec_, agent_self* self, async& async, agt_timestamp_t deadline) noexcept {
  const auto exec = static_cast<local_event_executor*>(exec_);
  const auto ctx  = exec->ctx;
  auto eagent = static_cast<local_event_eagent*>(self->execAgentTag);
  const auto flags = eagent->fiberFlags;

  struct data_t {
    agent_self*         pSelf;
    local_event_eagent* eagent;
    agt::async*         pAsync;
    agt_timestamp_t     deadline;
  } data { self, eagent, &async, deadline };


  const auto message = ctx->currentMessage;


  auto [ sourceFiber, forkAction ] = AGT_ctx_api(fiber_fork, ctx)(
    [](agt_fiber_t thisFiber, agt_fiber_param_t param, void* userData) -> agt_fiber_param_t {
      const auto data = reinterpret_cast<const data_t*>(param);
      const auto exec = static_cast<local_event_executor*>(userData);

      const auto ctx  = exec->ctx;

      auto& async = *data->pAsync;

      fiber_proc_state state{};
      state.receiveTimeout = exec->receiveTimeout;

      auto eagent = data->eagent;

      eagent->deadline   = data->deadline;
      eagent->boundFiber = thisFiber;
      eagent->isReady    = false;

      do {
        make_timed_out_agents_ready(exec);

        if (auto readyAgent = get_ready_agent(exec)) {
          if (readyAgent == eagent) { // This happens if the block's timeout has passed, in which case, return as though it timed out (cause it did lol)
            return FiberActionResumeTimedOut;
          }
          block_eagent(exec, data->eagent);
          continue_ready_agent(exec, readyAgent);
        }

        try_poll_message(exec, state, data->eagent);

      } while (!async_is_complete(async));

      return FiberActionFallthrough;

    }, reinterpret_cast<agt_fiber_param_t>(&data), flags);


  agt_status_t returnStatus = async.status;

  switch (forkAction) {
    case FiberActionResumeTimedOut:
      returnStatus = AGT_TIMED_OUT;
      [[fallthrough]];
    case FiberActionResumeSuccess:
      ctx->currentMessage = message;
      ctx->boundAgent     = self;
      eagent->isBlocked   = false;
      eagent->boundFiber  = nullptr;
      [[fallthrough]];
    case FiberActionFallthrough:
      break;
    default:
      std::unreachable();
  }

  return returnStatus;
}


void         yield(basic_executor* exec_, agent_self* self) noexcept {
  const auto exec = static_cast<local_event_executor*>(exec_);
  const auto ctx = exec->ctx;
  auto eagent = static_cast<local_event_eagent*>(self->execAgentTag);


  make_timed_out_agents_ready(exec);

  if (local_event_eagent* readyAgent = get_ready_agent(exec)) {
    fiber_action_t action;
    if (readyAgent->timedOut)
      action = FiberActionResumeTimedOut;
    else
      action = FiberActionResumeSuccess;
    exec->readyAgents.push_front(eagent);
    eagent->boundFiber = agt::current_fiber();
    const auto message = ctx->currentMessage; // Save current message, either on the stack, or in registers or smth. Full state will be restored, so it works either way :)
    auto [ sourceFiber, switchParam ] = AGT_ctx_api(fiber_switch, ctx)(readyAgent->boundFiber, action, eagent->fiberFlags);
    (void) sourceFiber;
    (void) switchParam;
    ctx->boundAgent = self;
    ctx->currentMessage = message;
  }
  else {
    if (auto msg = try_receive(&exec->receiver)) {
      eagent->isReady = true;
      struct data_t {
        fiber_proc_state    state;
        message             msg;
        local_event_eagent* eagent;
      } data{ { }, msg, eagent };

      const auto message = ctx->currentMessage;

      auto [forkSource, forkParam] = AGT_ctx_api(fiber_fork, ctx)([](agt_fiber_t thisFiber, agt_fiber_param_t param, void* userData) -> agt_fiber_param_t {

        const auto exec = static_cast<local_event_executor*>(userData);
        const auto data = reinterpret_cast<data_t*>(param);

        data->eagent->boundFiber = thisFiber;

        execute_message(exec, data->state, data->msg, data->eagent);
        return FiberActionFallthrough;

      }, reinterpret_cast<agt_fiber_param_t>(&data), eagent->fiberFlags);

      if (forkParam == FiberActionResumeSuccess) {
        // the fiber was actually jumped from in this case, and a little bit of state must be restored.
        ctx->currentMessage = message;
        ctx->boundAgent = self;

      }

      eagent->boundFiber = nullptr;
    }
  }
}


agt_status_t acquire_msg(basic_executor* exec_, const acquire_message_info& msgInfo, message& msg) noexcept {
  assert( exec_ != nullptr );
  auto exec = static_cast<local_event_executor*>(exec_);
  msg = acquire_message(exec->ctx, &exec->defaultPool, get_min_message_size(msgInfo));
  return AGT_SUCCESS;
}


void         release_msg(basic_executor* exec, message msg) noexcept {

}


agt_status_t commit_msg(basic_executor* exec, message msg) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}


  inline constexpr alignas(64) executor_vtable local_event_executor_vtable {
    .start             = &start,
    .bindAgent         = &bind_agent,
    .unbindAgent       = &unbind_agent,
    .block             = &block,
    .yield             = &yield,
    .acquireMessage    = &acquire_msg,
    .releaseMessage    = &release_msg,
    .commitMessage     = &commit_msg
  };
}



agt_status_t agt::create_local_event_executor(agt_ctx_t ctx, const event_executor_create_info& createInfo, executor& execOut) noexcept {
  assert( ctx != AGT_INVALID_CTX );

  auto exec = alloc<local_event_executor>(ctx);

  auto status = createInstance(exec->defaultPool, ctx, 0, 256);

  if (status != AGT_SUCCESS) {
    destroy(exec);
    return status;
  }

  status = createInstance(exec->selfPool, ctx, 64, 128);

  if (status != AGT_SUCCESS) {
    // TODO: destroy exec->defaultPool
    destroy(exec);
    return status;
  }

  exec->flags = { };
  exec->vptr  = &local_event_executor_vtable;
  exec->timeout = createInfo.timeout;
  // exec->linkedExecutors = {};
  exec->maxAgentCount = createInfo.maxAgentCount;
  exec->attachedAgents = 0;
  exec->ctx = ctx;

  exec->messageToBeHandled = nullptr;

  // exec->fiberStackSize    = createInfo.
  exec->initialFiberCount = createInfo.initialFiberCount;
  exec->maxFiberCount     = createInfo.maxFiberCount;

  exec->mainFiber    = nullptr;
  exec->currentFiber = nullptr;

  exec->fiberExitResult = 0;


  // local_mpsc_receiver        receiver;           // Primary receiver from which new messages are obtained.
  // private_sender             selfSender;         // Send messages to self
  // private_receiver           selfReceiver;       // Receive messages to self (this implemented a very efficient private message queue)
  // set<agent_self*>           agents;             // Set of agents attached to this executor. Should maybe hold a set of eagents?
  // agt_timeout_t              receiveTimeout;     // timeout waiting on message receive before trying something else.
  // agt_u32_t                  fiberStackSize;
  // vector<agt_fiber_t, 0>     freeFibers;         // These are unbound, idle fibers. Maybe there should be a maximum number of idle fibers so as to keep excess memory use low?
  // flist<local_event_eagent>  readyAgents;        // This needs to be a *queue*, I think it only needs to be agt_fiber_t objects?
  // flist<local_event_eagent>  timedBlockedAgents; // this should be a min heap, sorted by deadline
  // flist<local_event_eagent>  blockedAgents;      // this need only be a linked list. Does this even need to be here?



  execOut = exec;
  return AGT_SUCCESS;
}





