//
// Created by maxwe on 2022-05-27.
//

#ifndef AGATE_INTERNAL_AGENTS_HPP
#define AGATE_INTERNAL_AGENTS_HPP

#include "fwd.hpp"
#include "core/objects.hpp"
#include "channels/channel.hpp"

#include <csetjmp>

namespace agt {

  struct agency {

  };

  struct agent_group {

  };

  struct agent : handle_header {
    local_mpsc_channel*      channel;
    agent_group*             group;
    agt_type_id_t            type;
    agt_actor_message_proc_t proc;
    agt_agent_dtor_t         dtor;
    void*                    state;
  };


  enum agent_kind {
    free_agent_kind,
    standard_agent_kind
  };

  struct agent_instance {
    agent_kind  kind;
    union {
      struct {
        agt_free_agent_proc_t proc;
      } free;
      struct {
        agt_agent_proc_t proc;
        agt_agent_dtor_t destructor;
        void*            state;
      } standard;
    };
    message_pool_t        messagePool;
    message_queue_t       messageQueue;
  };




  struct agent_header {
    agent_kind kind;
  };




  struct free_agent {
    agt_free_agent_proc_t proc;
    message_pool_t        messagePool;
    message_queue_t       messageQueue;
  };

  struct standard_agent {
    agt_agent_proc_t proc;
    agt_agent_dtor_t destructor;
    void*            state;
    message_pool_t   messagePool;
    message_queue_t  messageQueue;
  };


  enum class block_kind : agt_u32_t {
    eNoBlock,                ///< Not blocked
    eSingleBlock,            ///< Blocked on a single async operation with no timeout (ie. infinite timeout)
    eSingleBlockWithTimeout, ///< Blocked on a single async operation with a timeout
    eAnyBlock,               ///< Blocked on multiple async operations with no timeout, is woken after any operation completes.
    eAnyBlockWithTimeout,    ///< Blocked on multiple async operations with a timeout, is woken after any operation completes.
    eManyBlock,              ///< Blocked on multiple async operations with no timeout, is woken after N operations completes (most frequently N == total number of async ops)
    eManyBlockWithTimeout    ///< Blocked on multiple async operations with no timeout, is woken after N operations completes (most frequently N == total number of async ops)
  };




  struct blocked_queue {
    agt::block_kind  blockKind;
    agt_u32_t        blockedMsgQueueSize;
    agt_message_t    blockedMsgQueueHead;
    agt_message_t*   blockedMsgQueueTail;
    agt_u64_t        blockTimeoutDeadline;
    union {
      agt_async_t* const * blockedOps; ///< Used if blocked on any/many
      agt_async_t*         blockedOp;  ///< Used if blocked on a single op
    };
    agt_u32_t        blockedManyCount;
    agt_u32_t        blockedManyFinishedCount;
    agt_u32_t*       asyncOpIndices;
    size_t*          anyIndexPtr;
    std::jmp_buf     contextStorage;
  };


  bool is_blocked(agt_agent_t agent) noexcept;



  bool processMessage(agt::agent_instance* receiver, agt_agent_t sender, agt_agent_t realSender, agt_message_t message) noexcept;

  bool dequeueAndProcessMessage(message_queue_t messageQueue, agt_timeout_t timeout) noexcept;



  agt_status_t wait(blocked_queue& queue, agt_async_t* async, agt_timeout_t timeout) noexcept;

  agt_status_t waitAny(blocked_queue& queue, agt_async_t* const * ppAsyncs, size_t asyncCount, agt_timeout_t timeout) noexcept;

  agt_status_t waitMany(blocked_queue& queue, agt_async_t* const * ppAsyncs, size_t asyncCount, size_t waitForCount, size_t& index, agt_timeout_t timeout) noexcept;
}

struct agt_agent_st {
  /*agt_dispatch_kind_t dispatchKind;
  union {
    agt_agent_no_dispatch_proc_t   noDispatchProc;
    agt_agent_id_dispatch_proc_t   idDispatchProc;
    agt_agent_name_dispatch_proc_t nameDispatchProc;
  };*/
  agt::agent_instance* instance;
  agt::message_pool_t  pool;
  agt_agent_dtor_t   destructor;
  void*              state;
  agt::blocked_queue blockedQueue;
};



inline bool agt::is_blocked(agt_agent_t agent) noexcept {
  return agent->blockedQueue.blockKind != block_kind::eNoBlock;
}

#endif//AGATE_INTERNAL_AGENTS_HPP
