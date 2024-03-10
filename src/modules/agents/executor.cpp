//
// Created by maxwe on 2022-08-07.
//

#include "executor.hpp"

#include "agents.hpp"
#include "core/attr.hpp"
#include "core/ctx.hpp"


#include <core/fiber.hpp>
#include <memory>









struct agt::local_event_eagent : object {
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



#if AGT_system_windows

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>



namespace {


  struct busy_thread_info {
    agt::local_busy_executor* executor;
  };

  struct single_thread_info {};

  struct thread_pool_info {};




  DWORD busy_thread_proc(_In_ LPVOID userData) noexcept {
    std::unique_ptr<busy_thread_info> args{static_cast<busy_thread_info*>(userData)};
    auto executor = args->executor;


  }

  DWORD single_thread_proc(_In_ LPVOID userData) noexcept {
    std::unique_ptr<single_thread_info> args{static_cast<single_thread_info*>(userData)};
  }

  DWORD thread_pool_proc(_In_ LPVOID userData) noexcept {
    std::unique_ptr<thread_pool_info> args{static_cast<thread_pool_info*>(userData)};

    auto tp = CreateThreadpool(nullptr);
    PTP_WORK work;
  }


  void start_busy_thread() {
    SECURITY_ATTRIBUTES securityAttributes{
        .nLength              = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle       = FALSE
    };
    // auto threadResult = CreateThread();
  }


  template <decltype(auto) PFN>
  struct get_thread_proc;
  template <typename T, void(* PFN)(T*)>
  struct get_thread_proc<PFN> {
    static DWORD fn(_In_ LPVOID userData) noexcept {
      PFN(static_cast<T*>(userData));
      return 0;
    }
  };
  template <typename T, void(* PFN)(T*) noexcept>
  struct get_thread_proc<PFN> {
    static DWORD fn(_In_ LPVOID userData) noexcept {
      PFN(static_cast<T*>(userData));
      return 0;
    }
  };

  template <decltype(auto) PFN>
  inline constexpr decltype(auto) thread_proc = &get_thread_proc<PFN>::fn;


  template <typename Fn>
  void create_thread(agt_thread_t* pThread, agt_ctx_t ctx, size_t stackSize, Fn&& fn) noexcept {
    auto inst = ctx->instance;
    using func_t = std::remove_cvref_t<Fn>;
    auto func = new func_t(std::forward<Fn>(fn));
    SECURITY_ATTRIBUTES securityAttr{
      .nLength = sizeof(SECURITY_ATTRIBUTES),
      .bInheritHandle = FALSE,
      .lpSecurityDescriptor = nullptr
    };
    if (stackSize == 0)
      stackSize = attr::default_thread_stack_size(inst);
    else
      stackSize = align_to(stackSize, static_cast<size_t>(attr::stack_size_alignment(inst)));
    DWORD threadId;
    auto result = CreateThread(&securityAttr, stackSize, [](void* userData) -> DWORD {

    }, func, );
  }
}




inline constexpr static agt_fiber_param_t ProcInitial     = 0;
inline constexpr static agt_fiber_param_t BlockFiber      = 1;
inline constexpr static agt_fiber_param_t BlockFiberUntil = 2;
inline constexpr static agt_fiber_param_t IdleFiber       = 3;
inline constexpr static agt_fiber_param_t DestroyFiber    = 4;
inline constexpr static agt_fiber_param_t YieldFiber      = 5;

using fiber_action_t = agt_fiber_param_t;

inline constexpr static fiber_action_t FiberActionInitialize     = 0;
inline constexpr static fiber_action_t FiberActionPoll           = 1;
inline constexpr static fiber_action_t FiberActionResumeSuccess  = 2;
inline constexpr static fiber_action_t FiberActionResumeTimedOut = 3;
inline constexpr static fiber_action_t FiberActionExecuteMessage = 4;
inline constexpr static fiber_action_t FiberActionFallthrough    = 5;




agt_fiber_param_t local_event_executor_proc(agt_fiber_t fromFiber, agt_fiber_param_t param, void* userData);


agt_fiber_param_t local_fiber_proc(agt_fiber_t fromFiber, agt_fiber_param_t action, void* userData);




void init_fibers(agt::local_event_executor* exec) noexcept {
  auto ctx = agt::get_ctx();
  auto self = current_fiber();

  assert( self != nullptr );

  const auto initialFiberCount = exec->initialFiberCount;


  if (initialFiberCount) {
    exec->freeFibers.reserve(initialFiberCount - 1);

    const auto make_new_fiber = AGT_ctx_api(new_fiber, ctx);

    for (agt_u32_t i = 0; i < initialFiberCount - 1; ++i) {
      agt_fiber_t newFiber;
      auto result = make_new_fiber(&newFiber, local_event_executor_proc, exec);
      if (result != AGT_SUCCESS) {
        // TODO: Handle failure to successfully create fibers
      }
      else {
        exec->freeFibers.push_back(newFiber);
      }
    }
  }
}

void block_fiber(agt::local_event_executor* exec, agt_fiber_t fiber) noexcept;

void block_fiber_until(agt::local_event_executor* exec, agt_fiber_t fiber) noexcept;

void idle_fiber(agt::local_event_executor* exec, agt_fiber_t fiber) noexcept;

void destroy_fiber(agt::local_event_executor* exec, agt_fiber_t fiber) noexcept;


void handle_fiber_transfer(agt_fiber_t fromFiber, agt_fiber_param_t param, agt::local_event_executor* exec) noexcept {
  switch (param) {
    case ProcInitial:
      init_fibers(exec);
    break;
    case BlockFiber:
      block_fiber(exec, fromFiber);
    break;
    case BlockFiberUntil:
      block_fiber_until(exec, fromFiber);
    break;
    case IdleFiber:
      idle_fiber(exec, fromFiber);
    break;
    case DestroyFiber:
      destroy_fiber(exec, fromFiber);
    break;
    default:
      hasError = true;
    shouldClose = true;
  }
}


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







#else
#endif




using namespace agt;

namespace {

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
    auto exec = (agt::local_event_executor*)userData;

    fiber_proc_state state{};

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

      if (auto readyAgent = get_ready_agent(exec))
        continue_ready_agent(exec, readyAgent);

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
      AGT_ctx_api(raise, ctx)(ctx, AGT_ERROR_FCTX_EXCEPTION, &exitInfo);
    }

  }

  inline constexpr executor_vtable local_event_executor_vtable {
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
  inline constexpr agt::executor_vtable local_busy_executor_vtable {
    .start = [](basic_executor* exec_, bool startOnCurrentThread) -> agt_status_t {},
    .attachAgentDirect = [](basic_executor* exec_, agent_self* agent) -> agt_status_t {},
    .detachAgentDirect = [](basic_executor* exec_, agent_self* agent) -> agt_status_t {

    },
    .blockAgent = [](basic_executor* exec_, agent_self* agent, async& async, agt_timestamp_t deadline) -> agt_status_t {},
    .yield = [](basic_executor* exec_, agent_self* agent) {},
    .acquireMessage = [](basic_executor* exec, const acquire_message_info& msgInfo, message& msg) -> agt_status_t {},
    .releaseMessage = [](basic_executor* exec, message msg) {},
    .commitMessage = [](basic_executor* exec, message msg) -> agt_status_t {}
  };
  inline constexpr agt::executor_vtable local_user_executor_vtable {
    .start = [](basic_executor* exec_, bool startOnCurrentThread) -> agt_status_t {},
    .attachAgentDirect = [](basic_executor* exec_, agent_self* agent) -> agt_status_t {},
    .detachAgentDirect = [](basic_executor* exec_, agent_self* agent) -> agt_status_t {

    },
    .blockAgent = [](basic_executor* exec_, agent_self* agent, async& async, agt_timestamp_t deadline) -> agt_status_t {},
    .yield = [](basic_executor* exec_, agent_self* agent) {},
    .acquireMessage = [](basic_executor* exec, const acquire_message_info& msgInfo, message& msg) -> agt_status_t {},
    .releaseMessage = [](basic_executor* exec, message msg) {},
    .commitMessage = [](basic_executor* exec, message msg) -> agt_status_t {}
  };

  inline constexpr agt::executor_vtable local_parallel_executor_vtable {
    .start = [](basic_executor* exec_, bool startOnCurrentThread) -> agt_status_t {},
    .attachAgentDirect = [](basic_executor* exec_, agent_self* agent) -> agt_status_t {},
    .detachAgentDirect = [](basic_executor* exec_, agent_self* agent) -> agt_status_t {

    },
    .blockAgent = [](basic_executor* exec_, agent_self* agent, async& async, agt_timestamp_t deadline) -> agt_status_t {},
    .yield = [](basic_executor* exec_, agent_self* agent) {},
    .acquireMessage = [](basic_executor* exec, const acquire_message_info& msgInfo, message& msg) -> agt_status_t {},
    .releaseMessage = [](basic_executor* exec, message msg) {},
    .commitMessage = [](basic_executor* exec, message msg) -> agt_status_t {}
  };

  inline constexpr agt::executor_vtable local_proxy_executor_vtable {
    .start = [](basic_executor* exec_, bool startOnCurrentThread) -> agt_status_t {},
    .attachAgentDirect = [](basic_executor* exec_, agent_self* agent) -> agt_status_t {},
    .detachAgentDirect = [](basic_executor* exec_, agent_self* agent) -> agt_status_t {

    },
    .blockAgent = [](basic_executor* exec_, agent_self* agent, async& async, agt_timestamp_t deadline) -> agt_status_t {},
    .yield = [](basic_executor* exec_, agent_self* agent) {},
    .acquireMessage = [](basic_executor* exec, const acquire_message_info& msgInfo, message& msg) -> agt_status_t {},
    .releaseMessage = [](basic_executor* exec, message msg) {},
    .commitMessage = [](basic_executor* exec, message msg) -> agt_status_t {}
  };




  agt_status_t start(local_event_executor* exec, bool startOnNewThread) noexcept {
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


  agt_status_t block(local_event_executor* localExec, agent_self* self, async& async, agt_timestamp_t deadline) noexcept {
    const auto ctx  = localExec->ctx;
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


  void         yield(local_event_executor* exec, agent_self* self) noexcept {
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

}







agt_status_t  agt::executor_wait(basic_executor* executor, agent_self* self, async& async, agt_timestamp_t deadline) noexcept {
  if (auto status = async_status(async); status != AGT_NOT_READY)
    return status;

  AGT_assert_is_a(executor, local_event_executor);

  return block(static_cast<local_event_executor*>(executor), self, async, deadline);
}



/*agt_status_t agt::executor_start(executor* exec) noexcept {

}

bool         agt::executor_has_started(executor* exec) noexcept {

}

agt_status_t agt::executor_block_agent(executor* exec, agt_agent_t agent, async& async) noexcept {

}

agt_status_t agt::executor_block_agent_until(executor* exec, agt_agent_t agent, async& async, agt_timestamp_t deadline) noexcept {

}

void         agt::yield(basic_executor* exec, agent_self* agent) noexcept {
  switch (exec->type) {
    case object_type::local_event_executor:
      return ::yield(static_cast<local_event_executor*>(exec), agent);
    case object_type::local_busy_executor:
      SwitchToThread();
      return;
    case object_type::local_user_executor:

    case object_type::local_proxy_executor:
      //
    case object_type::local_parallel_executor:
      return AGT_ERROR_NOT_YET_IMPLEMENTED;
    default:
      raise(AGT_ERROR_INVALID_HANDLE_TYPE, exec);
  }
}

agt_status_t agt::executor_acquire_message(executor* exec, const acquire_message_info& msgInfo, message*& msg) noexcept {
  switch(exec->type) {
    case object_type::local_event_executor: {
      const auto localExec = static_cast<local_event_executor*>(exec);
      auto result = acquire_message(agt::get_ctx(), &localExec->defaultPool, );

    }
    case object_type::local_busy_executor:
    case object_type::local_user_executor:
    case object_type::local_proxy_executor:
    case object_type::local_parallel_executor:
      return AGT_ERROR_NOT_YET_IMPLEMENTED;
    default:
      return AGT_ERROR_UNKNOWN;
  }
}

void         agt::executor_drop_message(executor* exec, message* msg) noexcept {

}

agt_status_t agt::executor_commit_message(executor* exec, message* msg) noexcept {

}*/