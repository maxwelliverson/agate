//
// Created by maxwe on 2022-07-13.
//

#ifndef AGATE_AGENTS_H
#define AGATE_AGENTS_H

#include <agate/core.h>


AGT_begin_c_namespace

/* =================[ Types ]================= */


typedef struct agt_agent_st*      agt_agent_t;
typedef struct agt_pinned_msg_st* agt_pinned_msg_t;





typedef void* (*agt_agent_ctor_t)(void* userData);
typedef void  (*agt_agent_dtor_t)(void* agentState);
typedef void  (*agt_agent_start_t)(void* agentState);
typedef void  (*agt_agent_proc_t)(void* agentState, agt_message_id_t msgId, const void* message, agt_size_t messageSize);

typedef void  (*agt_free_agent_proc_t)(agt_message_id_t msgId, void* message, agt_size_t messageSize);


typedef enum agt_agent_create_flag_bits_t {
  AGT_AGENT_CREATE_SHARED = 0x1
} agt_agent_create_flag_bits_t;
typedef agt_flags32_t agt_agent_create_flags_t;

typedef enum agt_agent_permission_flag_bits_t {
  AGT_AP_CAN_SEND_MESSAGE = 0x1
} agt_agent_permission_flag_bits_t;
typedef agt_flags32_t agt_agent_permission_flags_t;


typedef enum agt_agent_flag_bits_t {
  AGT_AGENT_IS_NAMED               = 0x1,
  AGT_AGENT_IS_SHARED              = 0x2,
  AGT_AGENT_HAS_DYNAMIC_PROPERTIES = 0x1000
} agt_agent_flag_bits_t;
typedef agt_flags32_t agt_agent_flags_t;




typedef enum agt_agent_cmd_t {
  AGT_CMD_NOOP,                    ///< As the name would imply, this is a noop.
  AGT_CMD_KILL,                    ///< Command sent on abnormal termination. Minimal cleanup is performed, typically indicates some unhandled error

  AGT_CMD_PROC_MESSAGE,            ///< Process message

  AGT_CMD_PROC_INDIRECT_MESSAGE,   ///< Process indirect message

  AGT_CMD_NAKED_MESSAGE,           ///< Message type of messages sent using raw channel API

  AGT_CMD_CLOSE_QUEUE,             ///< Normal termination command. Any messages sent before this will be processed as normal, but no new messages will be sent. As soon as the queue is empty, the agent is destroyed.
  AGT_CMD_INVALIDATE_QUEUE,        ///< Current message queue is discarded without having been processed, but the queue is not closed, nor is the agent destroyed.

  AGT_CMD_BARRIER_ARRIVE,          ///< When this message is dequeued, the arrival count of the provided barrier is incremented. If the post-increment arrival count is equal to the expected arrival count, any agents waiting on the barrier are unblocked, and if the barrier was set with a continuation callback, it is called (in the context of the last agent to arrive).
  AGT_CMD_BARRIER_WAIT,            ///< If the arrival count of the provided barrier is less than the expected arrival count, the agent is blocked until they are equal. Otherwise, this is a noop.
  AGT_CMD_BARRIER_ARRIVE_AND_WAIT, ///< Equivalent to sending AGT_CMD_BARRIER_ARRIVE immediately followed by AGT_CMD_BARRIER_WAIT
  AGT_CMD_BARRIER_ARRIVE_AND_DROP, ///<
  AGT_CMD_ACQUIRE_SEMAPHORE,       ///<
  AGT_CMD_RELEASE_SEMAPHORE,       ///<

  AGT_CMD_QUERY_UUID,              ///<
  AGT_CMD_QUERY_NAME,              ///<
  AGT_CMD_QUERY_DESCRIPTION,       ///<
  AGT_CMD_QUERY_PRODUCER_COUNT,    ///<
  AGT_CMD_QUERY_METHOD,            ///<

  AGT_CMD_QUERY_PROPERTY,          ///<
  AGT_CMD_QUERY_SUPPORT,           ///<
  AGT_CMD_SET_PROPERTY,            ///<
  AGT_CMD_WRITE,                   ///<
  AGT_CMD_READ,                    ///<

  AGT_CMD_FLUSH,                   ///<
  AGT_CMD_START,                   ///< Sent as the initial message to an eager agent; invokes an agent's start routine
  AGT_CMD_INVOKE_METHOD_BY_ID,     ///<
  AGT_CMD_INVOKE_METHOD_BY_NAME,   ///<
  AGT_CMD_REGISTER_METHOD,         ///<
  AGT_CMD_UNREGISTER_METHOD,       ///<
  AGT_CMD_INVOKE_CALLBACK,         ///<
  AGT_CMD_INVOKE_COROUTINE,        ///<
  AGT_CMD_REGISTER_HOOK,           ///<
  AGT_CMD_UNREGISTER_HOOK          ///<
} agt_agent_cmd_t;



typedef struct agt_send_info_t {

  agt_message_id_t id;
  agt_size_t       size;
  const void*      buffer;
  agt_async_t*     asyncHandle;
  agt_send_flags_t flags;
} agt_send_info_t;



typedef struct agt_agent_create_info_t {
  agt_agent_create_flags_t flags;
  const char*              name;             ///< Optional name. Points to a char buffer of at least nameLength bytes, or, if nameLength=0, is null-terminated. If this parameter is null, the new agent will be anonymous.
  size_t                   nameLength;       ///< Length of name in characters. If 0, name must be null terminated.
  size_t                   fixedMessageSize; ///< If not 0, all messages sent to this agent must be equal to or less than fixedMessageSize. If 0, messages of any size may be sent.
  agt_agent_start_t        initializer;      ///< Initial callback executed in the agent's context before it starts receiving messages.
  void*                    userData;         ///< The argument passed to the agent's constructor. If the agent does not have a constructor, then this points to the agent state (NOTE: in this case, the memory pointed to by userData must remain valid for the entire lifetime of the agent. May be null if the agent type has no state).
} agt_agent_create_info_t;





/* =================[ Creation Functions ]================= */


/***/

AGT_api agt_status_t AGT_stdcall agt_create_busy_agent(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo, agt_agent_t* pAgent) AGT_noexcept;

/**
 * Surrenders the current thread to a newly created BusyAgent.
 *
 * A typical pattern would involve creating the initial ecosystem of agents in main,
 * followed by a call to this function to start the execution of the system.
 * This function may *not* be called within an agent context
 * */
AGT_api AGT_noreturn void AGT_stdcall agt_create_busy_agent_on_current_thread(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo) AGT_noexcept;

AGT_api agt_status_t AGT_stdcall agt_create_event_agent(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo, agt_agent_t* pAgent) AGT_noexcept;
AGT_api agt_status_t AGT_stdcall agt_create_free_event_agent(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo, agt_agent_t* pAgent) AGT_noexcept;





/* ========================= [ Agents ] ========================= */

AGT_agent_api agt_ctx_t    AGT_stdcall agt_current_context() AGT_noexcept;

AGT_agent_api agt_agent_t  AGT_stdcall agt_self() AGT_noexcept;

AGT_agent_api agt_status_t AGT_stdcall agt_retain(agt_agent_t* pNewAgent, agt_agent_t agent) AGT_noexcept;

AGT_agent_api agt_status_t AGT_stdcall agt_send(agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_agent_api agt_status_t AGT_stdcall agt_send_as(agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_agent_api agt_status_t AGT_stdcall agt_send_many(const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_agent_api agt_status_t AGT_stdcall agt_send_many_as(agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_agent_api void         AGT_stdcall agt_reply(const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_agent_api void         AGT_stdcall agt_delegate(agt_agent_t recipient) AGT_noexcept;

AGT_agent_api void         AGT_stdcall agt_release(agt_agent_t agent) AGT_noexcept;

AGT_agent_api agt_pinned_msg_t AGT_stdcall agt_pin() AGT_noexcept;

AGT_agent_api void         AGT_stdcall agt_unpin(agt_pinned_msg_t pinnedMsg) AGT_noexcept;

/**
 * Signals normal termination of an agent.
 *
 * */
AGT_agent_api void         AGT_stdcall agt_exit(int exitCode) AGT_noexcept;

/**
 * Signals abnormal termination
 * */
AGT_agent_api void         AGT_stdcall agt_abort() AGT_noexcept;





AGT_agent_api void         AGT_stdcall agt_resume_coroutine(agt_agent_t receiver, void* coroutine, agt_async_t* asyncHandle) AGT_noexcept;



AGT_end_c_namespace


#endif//AGATE_AGENTS_H
