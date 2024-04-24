//
// Created by Maxwell on 2024-03-10.
//

#include "core/exec.hpp"

#include "core/fiber.hpp"

#include "agate/cast.hpp"
#include "agate/vector.hpp"

#include "core/time.hpp"


#include "debug.hpp"

#include <algorithm>


enum self_state {
  SELF_IS_IDLE, // This indicates that no agent is currently running, eg. during the executor event loop.
  SELF_IS_RUNNING,
  SELF_IS_BLOCKED,
  SELF_IS_TIMED_OUT,
  SELF_IS_WOKEN_UP,
};



AGT_object(local_event_self, extends(agt_self_st)) {
  local_event_self*          next;
  agt_fiber_t                boundFiber;
  self_state                 state;
  agt_fiber_flags_t          fiberFlags;
  agt_timestamp_t            deadline;
};

AGT_object(local_event_eagent) {
  set<agt_agent_instance_t>::iterator setPos;
};


using deadline_queue     = agt::vector<local_event_self*, 0>;
using deadline_queue_ref = agt::any_vector<local_event_self*>&;

namespace {
  struct ready_queue {
    local_event_self*  head;
    local_event_self** tail;
  };

  AGT_forceinline void init_ready_queue(ready_queue& list) noexcept {
    list.head = nullptr;
    list.tail = &list.head;
    std::atomic_thread_fence(std::memory_order_release);
  }


  AGT_forceinline void push(ready_queue& queue, local_event_self* from, local_event_self* to) noexcept {
    to->next = nullptr;
    auto tail = atomic_exchange(queue.tail, &to->next);
    atomic_store(*tail, from);
  }

  AGT_forceinline void push(ready_queue& queue, local_event_self* self) noexcept {
    push(queue, self, self);
  }

  AGT_forceinline local_event_self* pop(ready_queue& queue) noexcept {
    auto head = atomic_relaxed_load(queue.head);
    if (!head)
      return nullptr;
    queue.head = atomic_relaxed_load(head->next);
    atomic_try_replace(queue.tail, &head->next, &head);
    return head;
  }


}


AGT_object(local_event_executor, extends(agt_executor_st)) {
  local_event_self*          boundTask;
  agt_ctx_t                  ctx;                // Reference to local context for efficient retrieval
  local_mpsc_receiver        receiver;           // Primary receiver from which new messages are obtained.
  private_sized_message_pool selfPool;           // Message pool if the need to send *new* messages to self arises
  private_sender             selfSender;         // Send messages to self
  private_receiver           selfReceiver;       // Receive messages to self (this implemented a very efficient private message queue)
  local_spmc_message_pool    defaultPool;        // Only this executor allocates from pool, others deallocate
  set<agt_agent_instance_t>  agents;             // Set of agents attached to this executor. Should maybe hold a set of eagents?
  agt_timeout_t              receiveTimeout;     // timeout waiting on message receive before trying something else.
  agt_message_t              messageToBeHandled; // A message that needs to be handled on a new fiber
  agt_u32_t                  fiberStackSize;
  agt_u32_t                  initialFiberCount;
  agt_u32_t                  maxFiberCount;
  agt_fiber_t                mainFiber;          // Ideally, I want to phase out the notion of a "main" fiber in favor of there being a fiber context, that is exited when the fibers all exit, or when fctx_exit is called.
  agt_fiber_t                currentFiber;
  vector<agt_fiber_t, 0>     freeFibers;         // These are unbound, idle fibers. Maybe there should be a maximum number of idle fibers so as to keep excess memory use low?
  ready_queue                readyAgents;        // This needs to be a *queue*, I think it only needs to be agt_fiber_t objects?
  deadline_queue             timedBlockedAgents; // this should be a min heap, sorted by deadline
  // flist<local_event_eagent>  blockedAgents;      // this need only be a linked list. Does this even need to be here?
  agt_fiber_param_t          fiberExitResult;    // result of the fiber exiting...
};




namespace {





  agt_fiber_param_t local_fiber_proc(agt_fiber_t fromFiber, agt_fiber_param_t action, void* userData);


  using fiber_action_t = agt_fiber_param_t;

  inline constexpr fiber_action_t FiberActionInitialize     = 0;
  inline constexpr fiber_action_t FiberActionPoll           = 1;
  inline constexpr fiber_action_t FiberActionResumeSuccess  = 2;
  inline constexpr fiber_action_t FiberActionResumeTimedOut = 3;
  inline constexpr fiber_action_t FiberActionExecuteMessage = 4;
  inline constexpr fiber_action_t FiberActionFallthrough    = 5;




agt_fiber_t get_free_fiber(local_event_executor* exec) noexcept {
  if (!exec->freeFibers.empty())
    return exec->freeFibers.pop_back_val();
  agt_fiber_t fiber;
  auto status = AGT_ctx_api(new_fiber, exec->ctx)(&fiber, local_fiber_proc, exec);
  assert( status == AGT_SUCCESS );
  return fiber;
}


struct fiber_proc_state {
  bool shouldClose = false;
  bool isInitialLoop = false;
  bool shouldCleanup = true;
  agt_timeout_t receiveTimeout;
  agt_status_t status = AGT_SUCCESS;
};




/*

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
*/


// currentFiberIsBlocked should be true when messages are dispatched to from within a "block" loop
void execute_message(local_event_executor* exec, fiber_proc_state& state, agt_message_t msg, local_event_self* blockedAgent = nullptr) {
  auto ctx = exec->ctx;

  switch (msg->cmd) {
    case AGT_ECMD_NOOP:
      break;
    case AGT_ECMD_KILL:
      state.shouldClose   = true;
      state.shouldCleanup = false;
    break;
    case AGT_ECMD_AGENT_MESSAGE: {
      if (blockedAgent != nullptr) {
        // If we can find some way to track whether or not an agent *may* block
        // If we can determine that an agent has no possibility of blocking, we don't have to switch fibers to execute.
        auto fiber = get_free_fiber(exec);
        exec->messageToBeHandled = msg;
        AGT_ctx_api(fiber_jump, ctx)(fiber, FiberActionExecuteMessage);
        std::unreachable();
      }

      auto receiveAgent = msg->agent;
      const auto task = exec->boundTask;
      task->message = msg;
      task->agent = msg->agent;
      task->deadline = 0;
      task->state = SELF_IS_RUNNING;
      msg->state = AGT_MSG_STATE_ACTIVE;
      receiveAgent->proc((agt_self_t) receiveAgent,
                         receiveAgent->state,
                         reinterpret_cast<std::byte *>(msg + 1) + msg->userDataOffset,
                         msg->payloadSize);
      task->state = SELF_IS_IDLE;
      finalize_agent_message(ctx, msg, 0);
      break;
    }

    default:
      abort();
  }
}


inline void  deadline_queue_adjust_up(deadline_queue_ref queue, local_event_self* self, agt_timestamp_t deadline, uint32_t pos) noexcept {
  AGT_assert( queue.size() > 1 );

  while ( 0 < pos ) {
    uint32_t parentIndex     = (pos - 1) >> 1;
    local_event_self* parent = queue[parentIndex];
    if ( parent->deadline <= deadline )
      break;
    queue[pos] = parent;
    pos        = parentIndex;
  }

  queue[pos] = self;
}

inline void  deadline_queue_adjust_down(deadline_queue_ref queue, local_event_self* self, agt_timestamp_t deadline, uint32_t pos) noexcept {
  AGT_assert( queue.size() > 1 );
  const auto size = queue.size();
  uint32_t leftPos = (pos * 2) + 1;
  uint32_t rightPos = leftPos + 1;

  while (rightPos < size) {
    bool     isLeft = false;
    local_event_self* left  = queue[leftPos];
    local_event_self* right = queue[rightPos];
    if (left->deadline < deadline) {
      if (left->deadline <= right->deadline)
        isLeft = true;
    }
    else if (deadline <= right->deadline) {
      queue[pos] = self;
      return;
    }

    if (isLeft) {
      queue[pos] = left;
      pos = leftPos;
    }
    else {
      queue[pos] = right;
      pos = rightPos;
    }

    leftPos = (pos * 2) + 1;
    rightPos = leftPos + 1;
  }

  if (rightPos == size) {
    // We still have to check if the left child needs to be switched
    auto left = queue[leftPos];
    if (left->deadline < deadline) {
      queue[pos] = left;
      pos        = leftPos;
    }
  }

  queue[pos] = self;
}

inline void  deadline_queue_pop(deadline_queue_ref queue) noexcept {
  /**
   * There are two ways to go about this:
   *
   * a)
   *    value = queue.pop_back()
   *    queue[0] = value
   *    if 1 < queue.size
   *        deadline_queue_adjust_down(queue, value, value.deadline, 0)
   *
   * b)
   *    value = queue.pop_back()
   *    if queue.size < 2
   *        if queue.size == 1
   *            queue[0] = value
   *        return
   *    pos = 0
   *    leftPos = 2pos + 1
   *    rightPos = leftPos + 1
   *    while rightPos < queue.size
   *        left = queue[leftPos]
   *        right = queue[rightPos]
   *        if left.deadline <= right.deadline
   *            pos = leftPos
   *        else
   *            pos = rightPos
   *        leftPos = 2pos + 1
   *        rightPos = leftPos + 1
   *    if rightPos == queue.size
   *        pos = leftPos
   *    queue[pos] = value
   *    deadline_queue_adjust_up(queue, value, value.deadline, pos)
   *
   *
   * While b) appears significantly more complex, the bulk of the code is essentially an
   * optimized version of deadline_queue_adjust_down.
   *
   * My guess is that b) will perform better, especially in cases where the queue is quite large,
   * as the expected number of iterations upwards at the end of b) should be only a fraction of
   * the height of the heap, whereas the expected number of iterations down in a) should be most
   * of the height of the heap, and iterations in adjust_down are quite a bit more complex than
   * iterations in adjust_up.
   * The only real question in my mind is whether the algorithmic performance gains of b) will be
   * slim enough that they'd be outweighed by the better cache usage of the smaller a).
   *
   * Need to benchmark to find out; for now we use b)
   * */
   AGT_invariant( !queue.empty() );
  const auto value = queue.pop_back_val();
  const auto size = queue.size();
  if (size < 2) {
    if (size == 1)
      queue[0] = value;
    return;
  }


  uint32_t pos = 0;
  uint32_t leftPos = (2 * pos) + 1;
  uint32_t rightPos = leftPos + 1;

  while (rightPos < size) {
    const auto left = queue[leftPos];
    const auto right = queue[rightPos];
    if ( left->deadline <= right->deadline )
      pos = leftPos;
    else
      pos = rightPos;
    leftPos  = (2 * pos) + 1;
    rightPos = leftPos + 1;
  }
  if (rightPos == size)
    pos = leftPos;
  queue[pos] = value;

  deadline_queue_adjust_up(queue, value, value->deadline, pos);
}

inline void  deadline_queue_modify_entry(deadline_queue_ref queue, local_event_self* self, agt_timestamp_t deadline) noexcept {
  const auto size = queue.size();
  AGT_assert( size > 0 );

  const auto oldDeadline = self->deadline;
  self->deadline = deadline;

  if (size > 1) {
    // if self->deadline is *NOT* zero, then it's still in the heap somewhere. This is *unlikely*, but possible.
    // It should be likely to be right near the start, so you shouldn't have to search through the whole array
    uint32_t pos = 0;


    // This loop is slightly convoluted so that we can place an assertion to ensure the search always succeeds.
    do {
      if (queue[pos] == self)
        break;
      ++pos;
      if (pos < size)
        continue;
      AGT_assert(false && "pos went out of bounds");
      std::unreachable();
    } while(true);

    // Most likely case is for the old deadline to preceed the new deadline.
    if (oldDeadline < deadline) [[likely]] {
      deadline_queue_adjust_down(queue, self, deadline, pos);
    }
    else {
      deadline_queue_adjust_up(queue, self, deadline, pos);
    }
  }


}

inline void  push_to_timeout_queue(local_event_executor* exec, local_event_self* self, agt_timestamp_t deadline) noexcept {
  auto& queue = exec->timedBlockedAgents;

  if (self->deadline == 0) [[likely]] {
    self->deadline = deadline;
    queue.push_back(self);
    const auto size = queue.size();

    if (1 < size) {
      uint32_t nodeIndex = size - 1;
      uint32_t parentIndex = (nodeIndex - 1) >> 1;
      local_event_self* parent = queue[parentIndex];

      while ( 0 < nodeIndex && parent->deadline < deadline ) {
        queue[nodeIndex] = parent;
        nodeIndex   = parentIndex;
        parentIndex = (nodeIndex - 1) >> 1;
        parent      = queue[parentIndex];
      }

      queue[nodeIndex] = self;
    }
  }
  else
    deadline_queue_modify_entry(queue, self, deadline);
}

inline local_event_self* try_pop_from_timeout_queue(local_event_executor* exec, agt_timestamp_t now) noexcept {
  if (!exec->timedBlockedAgents.empty()) {
    const auto front = exec->timedBlockedAgents.front();
    if (front->deadline <= now) {
      // Maybe set overshoot here??
      front->deadline = 0;
      deadline_queue_pop(exec->timedBlockedAgents);
      return front;
    }
  }
  return nullptr;
}




bool time_out(self_state& state) noexcept {
  self_state prev = SELF_IS_BLOCKED;
  return atomic_try_replace(state, prev, SELF_IS_TIMED_OUT);
}

AGT_noinline void make_timed_out_agents_ready(local_event_executor* exec) noexcept {
  if (!exec->timedBlockedAgents.empty()) {

    const auto now = agt::now(exec->ctx);


    local_event_self* head  = nullptr;
    local_event_self* tail  = nullptr;

    while (auto poppedEntry = try_pop_from_timeout_queue(exec, now)) {
      if (!time_out(poppedEntry->state)) // these are to be queued by someone else!
        continue;
      if (tail == nullptr)
        head = poppedEntry;
      else
        tail->next = poppedEntry;
      tail = poppedEntry;
    }

    // This loop ensures that the range [from, to] consists of #count agents that have timed out.

    if (head != nullptr)
      push(exec->readyAgents, head, tail);
  }
}

local_event_self* get_ready_agent(local_event_executor* exec) noexcept {
  if (auto readyAgent = pop(exec->readyAgents))
    return readyAgent;
  make_timed_out_agents_ready(exec);
  return pop(exec->readyAgents);
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




  bool try_poll_message(local_event_executor* exec, fiber_proc_state& state, local_event_self* blockedAgent = nullptr) noexcept {
    agt_message_t msg;
    switch (auto result = receive_local_mpsc(&exec->receiver, msg, state.receiveTimeout)) {
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

  [[noreturn]] void continue_ready_agent(local_event_executor* exec, local_event_self* readyAgent) noexcept {
    agt_fiber_param_t action;
    const auto prevState = atomic_exchange(readyAgent->state, SELF_IS_RUNNING);
    if (prevState == SELF_IS_WOKEN_UP)
      action = FiberActionResumeSuccess;
    else {
      AGT_invariant( prevState == SELF_IS_TIMED_OUT );
      action = FiberActionResumeTimedOut;
    }
    AGT_ctx_api(fiber_jump, exec->ctx)(readyAgent->boundFiber, action);
    std::unreachable(); // fiber_jump is noreturn, but neither _Noreturn or [[noreturn]] cannot be applied to a function pointer (gcc's __attribute__((noreturn)) can be, but simply placing an unreachable marker afterwards is an easier, more portable solution here).
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
    } while(!state.shouldClose);


    if (state.shouldCleanup) {
      // blehhhhhhh
    }

  }



  void local_event_executor_proc(local_event_executor* exec) noexcept {
    auto ctx = exec->ctx;
    ctx->uexec = nonnull_object_cast<agt_uexec_st>(exec);
    // ctx->executor = (agt_executor_t)exec;

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






  /*inline constexpr alignas(64) executor_vtable local_event_executor_vtable {
    .start             = &start,
    .bindAgent         = &bind_agent,
    .unbindAgent       = &unbind_agent,
    .block             = &block,
    .yield             = &yield,
    .acquireMessage    = &acquire_msg,
    .releaseMessage    = &release_msg,
    .commitMessage     = &commit_msg
  };*/

// Tries to set state to blocked if previous state was running,
// otherwise previous state must have been woken up, and we can instead resume immediately, resetting state to running.
inline bool try_block(self_state& state) noexcept {
  self_state cmpVal = SELF_IS_RUNNING;
  const auto result = atomic_try_replace(state, cmpVal, SELF_IS_BLOCKED);
  if (!result) {
    AGT_invariant( atomic_load(state) == SELF_IS_TIMED_OUT );
    atomic_relaxed_store(state, SELF_IS_RUNNING);
  }
  return result;
}

  inline constexpr struct {
    agt_uexec_vtable_t   uexecVtable = {
        .yield       = [](agt_ctx_t ctx, agt_ctxexec_t ctxexec) -> agt_bool_t {
          const auto exec = unsafe_nonnull_object_cast<local_event_executor>(ctx->uexec);
          auto self = exec->boundTask;
          (void)ctxexec;

          if (auto readyAgent = get_ready_agent(exec)) {

            fiber_action_t action;
            if (readyAgent->state == SELF_IS_TIMED_OUT)
              action = FiberActionResumeTimedOut;
            else
              action = FiberActionResumeSuccess;
            self->boundFiber = exec->currentFiber;
            push(exec->readyAgents, self);
            auto [ sourceFiber, switchParam ] = AGT_ctx_api(fiber_switch, ctx)(readyAgent->boundFiber, action, self->fiberFlags);
            (void) sourceFiber;
            (void) switchParam;
            exec->boundTask = self;
            return AGT_TRUE;
          }
          else if (auto msg = try_receive_local_mpsc(&exec->receiver)) {
            struct data_t {
              fiber_proc_state  state;
              agt_message_t     msg;
              local_event_self* self;
            } data{ { }, msg, self };

            auto [forkSource, forkParam] = AGT_ctx_api(fiber_fork, ctx)([](agt_fiber_t thisFiber, agt_fiber_param_t param, void* userData) -> agt_fiber_param_t {

              const auto exec = static_cast<local_event_executor*>(userData);
              const auto data = reinterpret_cast<data_t*>(param);

              data->self->boundFiber = thisFiber;

              execute_message(exec, data->state, data->msg, data->self);
              return FiberActionFallthrough;

            }, reinterpret_cast<agt_fiber_param_t>(&data), self->fiberFlags);

            if (forkParam == FiberActionResumeSuccess) {
              // the fiber was actually jumped from in this case, and a little bit of state must be restored.
              exec->boundTask = self;
            }

            return AGT_TRUE;
          }

          return AGT_FALSE;
        },
        .suspend     = [](agt_ctx_t ctx, agt_ctxexec_t execCtxData){
          const auto ctxexec = static_cast<local_event_self**>(execCtxData);
          const auto self   = *ctxexec;
          const auto exec = member_to_struct_cast<&local_event_executor::boundTask>(ctxexec);

          const auto flags = AGT_FIBER_SAVE_EXTENDED_STATE; // At some point, it might be worth trying to determine from the agent itself whether extended state *needs* to be saved?

          auto [ sourceFiber, forkAction ] = AGT_ctx_api(fiber_fork, ctx)(
              [](agt_fiber_t thisFiber, agt_fiber_param_t param, void* userData) -> agt_fiber_param_t {
                const auto agent = reinterpret_cast<local_event_self*>(param);
                const auto exec  = static_cast<local_event_executor*>(userData);

                const auto ctx  = exec->ctx;

                if (!try_block(agent->state)) {
                  // For now, resume immediately.
                  // In the future, we may wish to treat this as a 'yield' of sorts,
                  // where instead of resuming immediately, if there are any ready agents,
                  // self is queued in the ready list and another ready task is run.
                  return FiberActionFallthrough;
                }

                fiber_proc_state state{};
                state.receiveTimeout = exec->receiveTimeout;

                agent->boundFiber = thisFiber;
                agent->deadline   = 0;


                // eagent->deadline   = data->deadline;
                // eagent->boundFiber = thisFiber;
                // eagent->isReady    = false;

                self_state agentState;

                do {
                  make_timed_out_agents_ready(exec);

                  if (auto readyAgent = get_ready_agent(exec))
                    continue_ready_agent(exec, readyAgent);

                  try_poll_message(exec, state, agent);

                  agentState = atomic_relaxed_load(agent->state);

                } while (agentState == SELF_IS_BLOCKED);

                return FiberActionFallthrough;

              }, reinterpret_cast<agt_fiber_param_t>(self), flags);
          exec->boundTask = self;
/*
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
          agent->timedOut = false;*/
        },
        .suspend_for = [](agt_ctx_t ctx, agt_ctxexec_t execCtxData, agt_timeout_t timeout) -> agt_bool_t {
          const auto ctxexec = static_cast<local_event_self**>(execCtxData);
          const auto self   = *ctxexec;

          const auto flags = AGT_FIBER_SAVE_EXTENDED_STATE; // At some point, it might be worth trying to determine from the agent itself whether extended state *needs* to be saved?

          struct data_t {
            local_event_executor* exec;
            local_event_self*     self;
            // agt::hwtime           deadline;
            agt_timestamp_t       deadline;
          } data{
              nullptr,
              self,
              agt::now(ctx) + timeout
              // add_duration(ctx, hwnow(), timeout)
          };


          auto [ sourceFiber, forkAction ] = AGT_ctx_api(fiber_fork, ctx)(
              [](agt_fiber_t thisFiber, agt_fiber_param_t param, void* userData) -> agt_fiber_param_t {
                const auto data = reinterpret_cast<data_t*>(param);
                auto&& [
                    exec,
                    suspendedSelf,
                    deadline
                ] = *data;

                exec = static_cast<local_event_executor*>(userData);

                const auto ctx  = exec->ctx;

                if (!try_block(suspendedSelf->state)) {
                  // For now, resume immediately.
                  // In the future, we may wish to treat this as a 'yield' of sorts,
                  // where instead of resuming immediately, if there are any ready agents,
                  // self is queued in the ready list and another ready task is run.
                  return FiberActionResumeSuccess;
                }

                push_to_timeout_queue(exec, suspendedSelf, deadline);

                fiber_proc_state state{};
                state.receiveTimeout = exec->receiveTimeout;

                suspendedSelf->boundFiber = thisFiber;

                self_state agentState;

                do {
                  if (auto readyAgent = get_ready_agent(exec)) {
                    if (readyAgent == suspendedSelf) {
                      agentState = atomic_exchange(readyAgent->state, SELF_IS_RUNNING);
                      break;
                    }
                    continue_ready_agent(exec, readyAgent);
                  }


                  try_poll_message(exec, state, suspendedSelf);

                  agentState = atomic_relaxed_load(suspendedSelf->state);

                } while (agentState == SELF_IS_BLOCKED);

                if (agentState == SELF_IS_WOKEN_UP)
                  return FiberActionResumeSuccess;
                AGT_invariant( agentState == SELF_IS_TIMED_OUT );
                return FiberActionResumeTimedOut;

              }, reinterpret_cast<agt_fiber_param_t>(self), flags);

          auto exec = data.exec;
          exec->boundTask  = self;
          self->boundFiber = nullptr;
          // auto eagent = static_cast<local_event_eagent*>(self->agent->eagent);
          if (forkAction == FiberActionResumeSuccess)
            return AGT_TRUE;
          AGT_invariant( forkAction == FiberActionResumeTimedOut );
          return AGT_FALSE;
        },
        .resume      = [](agt_ctx_t ctx, agt_ctxexec_t prevCtxExec, agt_utask_t task){
          const auto ctxexec = static_cast<local_event_self**>(prevCtxExec);
          const auto agent   = static_cast<local_event_self*>(task);
          const auto exec = member_to_struct_cast<&local_event_executor::boundTask>(ctxexec);

          if (atomic_try_replace(agent->state, SELF_IS_BLOCKED, SELF_IS_WOKEN_UP)) {

          }
        }
    };
    agt::executor_vtable executorVtable = {
        .start = [](agt_executor_t exec_, bool startOnNewThread) -> agt_status_t {
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
        },
        .bindAgent = [](agt_executor_t exec_, agt_agent_instance_t agent) -> agt_status_t {
          const auto exec = static_cast<local_event_executor*>(exec_);
          AGT_invariant( agent->executor == nullptr );
          assert( exec->attachedAgents <= exec->maxAgentCount ); // attachedAgents should never be greater than maxAgentCount
          if (exec->attachedAgents == exec->maxAgentCount)
            return AGT_ERROR_AT_CAPACITY; // cannot attach another agent because the maximum number of agents has already been attached
          auto [pos, isNew] = exec->agents.insert_as(agent);
          AGT_invariant( isNew );
          ++exec->attachedAgents;
          agent->executor = reinterpret_cast<agt_executor_t>(exec);
          // advance_agent_epoch(agent);
          auto eagent = alloc<local_event_eagent>(exec->ctx);
          eagent->setPos      = pos;
          agent->eagent = eagent;
          // eagent->next        = nullptr;
          // eagent->self        = agent;
          // eagent->boundFiber  = nullptr;
          // eagent->isBlocked   = false;
          // eagent->isReady     = false;
          // eagent->deadline    = 0;
          // agent->execAgentTag = eagent;
          return AGT_SUCCESS;
        },
        .unbindAgent = [](agt_executor_t exec_, agt_agent_instance_t agent) -> agt_status_t {
          AGT_invariant( exec_ != nullptr );
          AGT_invariant( agent != nullptr );
          AGT_invariant( agent->eagent != nullptr );
          auto exec = static_cast<local_event_executor*>(exec_);
          auto eagent = static_cast<local_event_eagent*>(agent->eagent);
          AGT_invariant( exec->agents.contains(agent) );
          exec->agents.erase(eagent->setPos);
          --exec->attachedAgents;
          agent->executor = nullptr;
          agent->eagent = nullptr;
          // advance_agent_epoch(agent);
          release(eagent);
          return AGT_SUCCESS;
        },
        .acquireMessage = [](agt_executor_t exec_, const acquire_message_info& msgInfo, agt_message_t& msg) -> agt_status_t {
          AGT_invariant( exec_ != nullptr );
          auto exec = static_cast<local_event_executor*>(exec_);
          msg = acquire_message(exec->ctx, &exec->defaultPool, get_min_message_size(msgInfo));
          msg->cmd = msgInfo.cmd;
          msg->layout = msgInfo.layout;
          msg->state = AGT_MSG_STATE_INITIAL;
          return AGT_SUCCESS;
        },
        .releaseMessage = [](agt_executor_t exec, agt_message_t msg){
          AGT_not_implemented();
        },
        .commitMessage = [](agt_executor_t exec, agt_message_t msg) -> agt_status_t {
          AGT_return_not_implemented();
        }
    };
  } local_event_uexec_vtable;
}



agt_status_t agt::create_local_event_executor(agt_ctx_t ctx, const event_executor_create_info& createInfo, executor& execOut) noexcept {
  assert( ctx != AGT_INVALID_CTX );

  auto exec = alloc<local_event_executor>(ctx);

  auto status = createInstance(exec->defaultPool, ctx, 0, 256);

  if (status != AGT_SUCCESS) {
    release(exec);
    return status;
  }

  status = createInstance(exec->selfPool, ctx, 64, 128);

  if (status != AGT_SUCCESS) {
    // TODO: destroy exec->defaultPool
    release(exec);
    return status;
  }

  exec->flags = { };
  exec->vtable = &local_event_uexec_vtable.uexecVtable;
  exec->vptr   = &local_event_uexec_vtable.executorVtable;
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

  init_ready_queue(exec->readyAgents);


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




size_t agt::get_min_message_size(const acquire_message_info& msgInfo) noexcept {
  return 0;
}


