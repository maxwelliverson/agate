//
// Created by maxwe on 2022-08-07.
//

#include "executor.hpp"

#include "agents.hpp"
#include "core/attr.hpp"
#include "core/ctx.hpp"


#include <core/fiber.hpp>
#include <memory>


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
    auto threadResult = CreateThread();
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
}




inline constexpr static agt_fiber_param_t ProcInitial     = 0;
inline constexpr static agt_fiber_param_t BlockFiber      = 1;
inline constexpr static agt_fiber_param_t BlockFiberUntil = 2;
inline constexpr static agt_fiber_param_t IdleFiber       = 3;
inline constexpr static agt_fiber_param_t DestroyFiber    = 4;



agt_fiber_param_t local_event_executor_proc(agt_fiber_t fromFiber, agt_fiber_param_t param, void* userData);

void init_fibers(agt::local_event_executor* exec) noexcept {
  auto ctx = agt::get_ctx();
  auto self = ctx->boundFiber;

  assert( self != nullptr );

  const auto initialFiberCount = exec->initialFiberCount;


  if (initialFiberCount) {
    exec->freeFibers.reserve(initialFiberCount - 1);

    const auto make_new_fiber = AGT_api(new_fiber);

    for (agt_u32_t i = 0; i < initialFiberCount - 1; ++i) {
      agt_fiber_t newFiber;
      auto result = make_new_fiber(ctx, &newFiber, local_event_executor_proc, exec);
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


agt_fiber_param_t local_fiber_proc(agt_fiber_t fromFiber, agt_fiber_param_t param, void* userData) {
  auto exec = (agt::local_event_executor*)userData;

  bool hasError    = false;
  bool shouldClose = false;
  bool shouldCleanup = true;

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

  while (!shouldClose) {
    agt::message* msg;
    switch(receive(&exec->receiver, msg, exec->timeout)) {
      case AGT_SUCCESS:
        switch(static_cast<agt_ecmd_t>(msg->cmd)) {
          case AGT_ECMD_NOOP:
            break;
          case AGT_ECMD_KILL:
            shouldClose   = true;
            shouldCleanup = false;
          break;
          case AGT_ECMD_AGENT_MESSAGE:

        }
      case AGT_NOT_READY:
      default:
    }

  }

  if (shouldCleanup) {




  }
}

void executor_proc(agt::local_event_executor* exec) {




  // TODO: Handle possible error
}




#else
#endif


struct agt::local_event_eagent : object {
  local_event_eagent*        next;
  agt_fiber_t                boundFiber;
  set<owner_ref<agent_self>>::iterator setPos;
  agent_self*                self;
  bool                       isBlocked;
  bool                       isReady;
  agt_timestamp_t            deadline;
};


using namespace agt;

namespace {



  void local_event_executor_proc(local_event_executor* exec) noexcept {
    auto ctx = get_ctx();

    agt_fctx_desc_t fctxDesc;
    fctxDesc.flags         = 0;
    fctxDesc.stackSize     = attr::default_fiber_stack_size();
    fctxDesc.maxFiberCount = exec->maxAgentCount;
    fctxDesc.parent        = nullptr;
    fctxDesc.proc          = local_fiber_proc;
    fctxDesc.initialParam  = ProcInitial;
    fctxDesc.userData      = exec;

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
        // TODO: Implement starting a local_event_executor on a new thread
      }
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
      agent->
    },
    .blockAgent = [](basic_executor* exec_, agent_self* agent, async& async) -> agt_status_t {
      assert( exec_ != nullptr );
      assert( agent != nullptr );
      assert( agent->execAgentTag != nullptr );
      auto exec = static_cast<local_event_executor*>(exec_);
      auto eagent = static_cast<local_event_eagent*>(agent->execAgentTag);

    },
    .blockAgentUntil = [](basic_executor* exec_, agent_self* agent, async& async, agt_timestamp_t deadline) -> agt_status_t {},
    .yield = [](basic_executor* exec_, agent_self* agent) {},
    .acquireMessage = [](basic_executor* exec, const acquire_message_info& msgInfo, message& msg) -> agt_status_t {},
    .releaseMessage = [](basic_executor* exec, message msg) {},
    .commitMessage = [](basic_executor* exec, message msg) -> agt_status_t {}
  };
  inline constexpr agt::executor_vtable local_busy_executor_vtable {
    .start = [](basic_executor* exec_, bool startOnCurrentThread) -> agt_status_t {},
    .attachAgentDirect = [](basic_executor* exec_, agent_self* agent) -> agt_status_t {},
    .detachAgentDirect = [](basic_executor* exec_, agent_self* agent) -> agt_status_t {

    },
    .blockAgent = [](basic_executor* exec_, agent_self* agent, async& async) -> agt_status_t {},
    .blockAgentUntil = [](basic_executor* exec_, agent_self* agent, async& async, agt_timestamp_t deadline) -> agt_status_t {},
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
    .blockAgent = [](basic_executor* exec_, agent_self* agent, async& async) -> agt_status_t {},
    .blockAgentUntil = [](basic_executor* exec_, agent_self* agent, async& async, agt_timestamp_t deadline) -> agt_status_t {},
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
    .blockAgent = [](basic_executor* exec_, agent_self* agent, async& async) -> agt_status_t {},
    .blockAgentUntil = [](basic_executor* exec_, agent_self* agent, async& async, agt_timestamp_t deadline) -> agt_status_t {},
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
    .blockAgent = [](basic_executor* exec_, agent_self* agent, async& async) -> agt_status_t {},
    .blockAgentUntil = [](basic_executor* exec_, agent_self* agent, async& async, agt_timestamp_t deadline) -> agt_status_t {},
    .yield = [](basic_executor* exec_, agent_self* agent) {},
    .acquireMessage = [](basic_executor* exec, const acquire_message_info& msgInfo, message& msg) -> agt_status_t {},
    .releaseMessage = [](basic_executor* exec, message msg) {},
    .commitMessage = [](basic_executor* exec, message msg) -> agt_status_t {}
  };
}






agt_status_t agt::executor_start(executor* exec) noexcept {

}

bool         agt::executor_has_started(executor* exec) noexcept {

}

agt_status_t agt::executor_block_agent(executor* exec, agt_agent_t agent, async& async) noexcept {

}
agt_status_t agt::executor_block_agent_until(executor* exec, agt_agent_t agent, async& async, agt_timestamp_t deadline) noexcept {

}

void         agt::executor_yield(executor* exec, agt_agent_t agent) noexcept {

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

}