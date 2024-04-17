//
// Created by maxwe on 2022-05-27.
//

#ifndef AGATE_INTERNAL_AGENTS_HPP
#define AGATE_INTERNAL_AGENTS_HPP

#include "config.hpp"

#include "agate/flags.hpp"
#include "channels/channel.hpp"
#include "core/object.hpp"




AGT_api_object(agt_agent_instance_st, agent_instance, ref_counted) {
  agt_agent_proc_t  proc;
  agt_agent_dtor_t  dtor;
  void*             state;
  agt_executor_t    executor;
  // agt_i32_t         concurrencyCount; // if 0, this agent has no active operations. If -1, there is an active operation with exclusive ownership. If N > 0, there are N active operations with concurrent ownership (ie. read only)
  // agt_i32_t         blockedCount; // Maybe these two fields go into eagent??
  agt_handle_t      handle;
  agt_eagent_t      eagent;  // additional instance data specific to the bound executor.
  agt_name_t        name;
  // agent_properties* properties;
  // agent_methods*    methods;
};

AGT_abstract_api_object(agt_self_st, agent_self) {
  agt::self_flags      flags;
  agt_ctx_t            ctx;       // This field is invalid if is_blocked(self) is true
  agt_executor_t       executor;
  agt_agent_instance_t agent;
  agt_message_t        message;   // This field is always valid :)
  // agt_status_t         errorCode; // should we??
};

AGT_abstract_api_object(agt_agent_st, agent) {
  agt::agent_flags     flags;
  agt_agent_instance_t instance;
  agt_executor_t       executor;
  // agt_sender_t         sender;
};





namespace agt {

  enum {
    AGT_AGENT_INSTANCE_DEFAULT_CONCURRENT = 0x1
  };

  // agent structures:
  //   agent:          open handle to an agent instance, owned by other agents for the purpose of sending messages to it.
  //   agent_instance: stores the agent's internal data, including the agent proc, destructor, state, etc.
  //   agent_self:     agt_self_t
  //   eagent_tag:     extends agent_self, an opaque type owned for use by the bound executor.

  AGT_DEFINE_BITFLAG_ENUM(agent_flags, uint32_t) {
      isProxy    = 0x1,
      isShared   = 0x2,
      isBusy     = 0x4,
      isBlocked  = 0x8,
      isLiteral  = 0x10,
      isImported = 0x20,
      isAbandonned = 0x40,
      useAgentMsgPool = 0x80,
  };


  AGT_DEFINE_BITFLAG_ENUM(self_flags, uint32_t) {
      eIsConcurrent       = 0x1,
      eConcurrencyEnabled = 0x2, // Whether or not this is set depends on the bound executor; this indicates whether the bound executor supports concurrent agent execution. If this is not set, all agent execution has, in effect, exclusive access.
      eIsBlocked          = 0x4,
  };

  inline constexpr static self_flags eSelfIsConcurrent       = self_flags::eIsConcurrent;
  inline constexpr static self_flags eSelfConcurrencyEnabled = self_flags::eConcurrencyEnabled;
  inline constexpr static self_flags eSelfIsBlocked          = self_flags::eIsBlocked;

  /*AGT_object(agent_self, ref_counted) {
    agt_agent_proc_t proc;
    agt_agent_dtor_t dtor;
    void*            state;
    agt_executor_t   executor;
    // agt_message_t     currentMessage;
    agt_u32_t        agentEpoch;
    agt_u32_t        padding;    // ???
    agt_handle_t     selfHandle; // Exporting and such
    // agt_fiber_t       boundFiber;
    agt_eagent_t     execAgentTag;
    agt_name_t       name;
    // agent_properties* properties;
    // agent_methods*    methods;
  };

  inline void destroy(agent_self* self) noexcept {
    if (self->dtor) {
      self->dtor(reinterpret_cast<agt_self_t>(self), self->state);
    }
  }*/

  /*AGT_abstract_object(agent) {
    agent_flags      flags;
    agent_self*      self;
    agent_self*      refOwner;
    basic_executor*  executor;
    sender_t         sender;
    // const agt_u32_t* pAgentEpoch; // points to self->agentEpoch; compare against value of cached agentEpoch to check if the cached epoch is out of date
    // agt_u32_t        agentEpoch;
  };

  AGT_object(local_agent,    extends(agent)) {

  };
  AGT_object(proxy_agent,    extends(agent)) {

  };
  AGT_object(imported_agent, extends(agent)) {

  };*/


  /*enum class block_kind : agt_u32_t {
    eNoBlock,                ///< Not blocked
    eSingleBlock,            ///< Blocked on a single async operation with no timeout (ie. infinite timeout)
    eSingleBlockWithTimeout, ///< Blocked on a single async operation with a timeout
    eAnyBlock,               ///< Blocked on multiple async operations with no timeout, is woken after any operation completes.
    eAnyBlockWithTimeout,    ///< Blocked on multiple async operations with a timeout, is woken after any operation completes.
    eManyBlock,              ///< Blocked on multiple async operations with no timeout, is woken after N operations completes (most frequently N == total number of async ops)
    eManyBlockWithTimeout    ///< Blocked on multiple async operations with no timeout, is woken after N operations completes (most frequently N == total number of async ops)
  };*/




  /*struct blocked_queue {
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
    // std::jmp_buf     contextStorage;
  };*/


  AGT_forceinline static bool is_blocked(agt_self_t self) noexcept {
    return test(self->flags, eSelfIsBlocked);
  }


  shared_handle agent_get_shared_handle(agt_agent_t agent) noexcept;

  agt_agent_t  import_agent(agt_ctx_t ctx, shared_handle handle) noexcept;


  /*
  agt_status_t wait(blocked_queue& queue, agt_async_t* async, agt_timeout_t timeout) noexcept;

  agt_status_t waitAny(blocked_queue& queue, agt_async_t* const * ppAsyncs, size_t asyncCount, agt_timeout_t timeout) noexcept;

  agt_status_t waitMany(blocked_queue& queue, agt_async_t* const * ppAsyncs, size_t asyncCount, size_t waitForCount, size_t& index, agt_timeout_t timeout) noexcept;
  */




}

/*struct agt_agent_st {
  agt::agent_flags     flags;
  agt_executor_t       executor;
  agt_type_id_t        type;
  agt_agent_dtor_t     destructor;
  agt_agent_proc_t     proc;
  void*                state;
  agt::blocked_queue   blockedQueue;
};



inline bool agt::is_blocked(agt_agent_t agent) noexcept {
  return agent->blockedQueue.blockKind != block_kind::eNoBlock;
}*/

#endif//AGATE_INTERNAL_AGENTS_HPP
