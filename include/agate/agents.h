//
// Created by maxwe on 2022-07-13.
//

#ifndef AGATE_AGENTS_H
#define AGATE_AGENTS_H

#include <agate/core.h>


AGT_begin_c_namespace

/* =================[ Types ]================= */




typedef struct agt_self_st*       agt_self_t;
typedef struct agt_agent_st*      agt_agent_t;
typedef struct agt_pinned_msg_st* agt_pinned_msg_t;
typedef struct agt_executor_st*   agt_executor_t;
typedef agt_u64_t                 agt_agent_handle_t; // For sending shared handles across shared channels
typedef agt_u64_t                 agt_typeid_t;
typedef agt_u64_t                 agt_raw_msg_t;



typedef AGT_transparent_union_2(agt_self_t, agt_agent_t) agt_agent_or_self_t;





typedef void* (*agt_agent_ctor_t)(const void* ctorUserData, void* params);
typedef void  (*agt_agent_dtor_t)(agt_self_t self, void* agentState);
typedef void  (*agt_agent_init_t)(agt_self_t self, void* agentState);
typedef void  (*agt_agent_proc_t)(agt_self_t self, void* agentState, const void* message, agt_size_t messageSize);


typedef enum agt_enumeration_action_t {
  AGT_CONTINUE_ENUMERATION,
  AGT_FINISH_ENUMERATION,
} agt_enumeration_action_t;



typedef agt_enumeration_action_t (* agt_type_enumerator_t)(void* userData, const char* name, size_t nameLength, agt_typeid_t id);


typedef enum agt_agent_create_flag_bits_t {
  AGT_AGENT_CREATE_SHARED            = 0x01,
  AGT_AGENT_CREATE_DETACHED          = 0x02, ///< A detached agent is not reference counted and is responsible for its own lifetime. This can be useful for breaking reference cycles for instance.
  AGT_AGENT_CREATE_BUSY              = 0x04,
  AGT_AGENT_CREATE_WITH_NEW_EXECUTOR = 0x08
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


typedef enum agt_send_flag_bits_t {

} agt_send_flag_bits_t;
typedef agt_flags32_t agt_send_flags_t;


typedef struct agt_send_info_t {
  agt_size_t       size;
  const void*      buffer;
  agt_async_t*     async;
  agt_send_flags_t flags;
} agt_send_info_t;

typedef struct agt_raw_send_info_t {
  agt_raw_msg_t    msg;
  agt_async_t*     async;
  agt_send_flags_t flags;
} agt_raw_send_info_t;




/*typedef struct agt_agent_type_create_info_t {
  agt_scope_t      scope;        ///< Scope; optional. If not set, scope is local.
  agt_name_t       name;         ///< Type Name
  agt_agent_ctor_t ctor;         ///< Constructor callback; optional. If null, userData provided during agent creation is used as is as state (and as such, must point to valid memory for the entire lifetime of the agent.)
  agt_agent_proc_t proc;         ///< Agent process; required
  agt_agent_dtor_t dtor;         ///< Agent destructor; optional. As the destructor is responsible for freeing the state memory, if this field is null, the program must ensure that state memory is freed in some other fashion.
  void*            ctorUserData; ///< Agent constructor data; optional. This field is saved and passed to the ctor parameter ctorUserData. The memory pointed to must not be modified, and must remain valid so long as new agents of this type are created.
} agt_agent_type_create_info_t;

typedef struct agt_agent_type_literal_info_t {
  agt_agent_proc_t proc;
  agt_agent_dtor_t dtor;
  // Maybe include some indication of what messages it can process??
  // I think message filtering/typing is probably beyond the scope of the core library
} agt_agent_type_literal_info_t;*/

typedef struct agt_executor_create_info_t {

} agt_executor_create_info_t;

typedef struct agt_thread_executor_create_info_t {

} agt_thread_executor_create_info_t;

typedef struct agt_proxy_executor_create_info_t {

} agt_proxy_executor_create_info_t;

typedef struct agt_thread_pool_executor_create_info_t {

} agt_thread_pool_executor_create_info_t;

typedef struct agt_agent_take_ownership_info_t {
  const agt_agent_t* agents;
  size_t             agentCount;
} agt_agent_take_ownership_info_t;

typedef struct agt_agent_inherit_ownership_info_t {

} agt_agent_inherit_ownership_info_t;


typedef struct agt_agent_type_info_t {

  agt_agent_init_t         initFn;           ///< [optional] Initial callback executed in the agent's context before it starts receiving messages.
  agt_agent_proc_t         procFn;           ///< Main callback executed in the agent's context
  agt_agent_dtor_t         dtorFn;           ///< [optional] Destructor, called on state when agent is destroyed
} agt_agent_type_info_t;

typedef struct agt_agent_create_info_t {
  agt_agent_create_flags_t flags;
  agt_name_t               name;             ///< [optional] Agent name; token must have previously been acquired from a call to agt_reserve_name. If set to AGT_ANONYMOUS, agent will remain anonymous.
  size_t                   fixedMessageSize; ///< [optional] If not 0, all messages sent to this agent must be equal to or less than fixedMessageSize. If 0, messages of any size may be sent.
  agt_agent_t              owner;            ///< [optional] The agent who owns the returned reference. If null and within an agent execution context, the owner defaults to agt_self(). If null and NOT within such a context, the agent has no defined owner.
  agt_executor_t           executor;         ///< [optional] The executor responsible for executing agent actions. If null and within an agent execution context, will try to use the current executor. If that fails for some reason, or if this field is null and not within an agent execution context, a default executor will be created as per context settings.
  agt_agent_init_t         initFn;           ///< [optional] Initial callback executed in the agent's context before it starts receiving messages.
  agt_agent_proc_t         procFn;           ///< Main callback executed in the agent's context
  agt_agent_dtor_t         dtorFn;           ///< [optional] Destructor, called on state when agent is destroyed
  void*                    state;            ///< [optional] The argument passed to the agent's constructor. If the agent does not have a constructor, then this points to the agent state (NOTE: in this case, the memory pointed to by userData must remain valid for the entire lifetime of the agent. May be null if the agent type has no state).
} agt_agent_create_info_t;



/* =================[ Agent Types ]================= */


// AGT_api agt_status_t AGT_stdcall agt_register_type(agt_ctx_t ctx, const agt_agent_type_create_info_t* cpCreateInfo, agt_typeid_t* pId) AGT_noexcept;

// AGT_api size_t       AGT_stdcall agt_enumerate_types(agt_ctx_t ctx, const char* pattern, size_t patternLength, agt_type_enumerator_t enumerator, void* userData) AGT_noexcept;



/* =================[ Creation Functions ]================= */


/***/

AGT_api agt_status_t AGT_stdcall agt_create_agent(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo, agt_agent_t* pAgent) AGT_noexcept;

AGT_api agt_status_t AGT_stdcall agt_create_agent_from_type(agt_ctx_t ctx, ) AGT_noexcept;


/**
 * Surrenders the current thread to a newly created BusyAgent.
 *
 * A typical pattern would involve creating the initial ecosystem of agents in main,
 * followed by a call to this function to start the execution of the system.
 * This function may *not* be called within an agent context
 * */
AGT_noreturn AGT_api void AGT_stdcall agt_create_busy_agent_on_current_thread(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo) AGT_noexcept;

AGT_api agt_status_t AGT_stdcall agt_create_executor(agt_ctx_t ctx, const agt_executor_create_info_t* cpCreateInfo, agt_executor_t* pExecutor) AGT_noexcept;


// If agentHandle is null, this is a noop.
// If agentHandle is detached (ie. does not, and cannot have an owner), this fails with AGT_ERROR_AGENT_IS_DETACHED
AGT_api       agt_status_t AGT_stdcall agt_transfer_owner(agt_ctx_t ctx, agt_agent_t agentHandle, agt_agent_t newOwner) AGT_noexcept;

// If agent
AGT_agent_api agt_status_t AGT_stdcall agt_take_ownership(agt_self_t self, agt_agent_t agent) AGT_noexcept;

/* =============[ Queries ]============= */

AGT_agent_api agt_status_t AGT_stdcall   agt_get_executor(agt_agent_or_self_t object, agt_executor_t* pResult) AGT_noexcept;


/* ========================= [ Agents ] ========================= */

AGT_agent_api agt_self_t   AGT_stdcall agt_self(agt_ctx_t ctx) AGT_noexcept;

AGT_agent_api agt_ctx_t    AGT_stdcall agt_self_ctx(agt_self_t self) AGT_noexcept;

/**
 * Optimized equivalent to the following:
 *
 * \code
 * agt_handle_t handle;
 * agt_export_handle(agt_self_ctx(self), self, &handle);
 * return handle;
 * \endcode
 *
 * Obviously, only works within an agent execution context.
 * Calling this function outside of an agent execution context results in undefined behaviour (how'd you even get a self object??)
 * */
AGT_agent_api agt_handle_t AGT_stdcall agt_export_self(agt_self_t self) AGT_noexcept;

AGT_agent_api agt_agent_t  AGT_stdcall agt_retain_sender(agt_self_t self) AGT_noexcept;

AGT_agent_api agt_status_t AGT_stdcall agt_retain(agt_agent_t* pNewAgent, agt_agent_t agent) AGT_noexcept;

AGT_agent_api agt_status_t AGT_stdcall agt_retain_self(agt_self_t self, agt_agent_t* pSelfAgent) AGT_noexcept;



AGT_agent_api agt_status_t AGT_stdcall agt_send(agt_self_t self, agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_agent_api agt_status_t AGT_stdcall agt_send_as(agt_self_t self, agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_agent_api agt_status_t AGT_stdcall agt_send_many(agt_self_t self, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_agent_api agt_status_t AGT_stdcall agt_send_many_as(agt_self_t self, agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_agent_api agt_status_t AGT_stdcall agt_reply(agt_self_t self, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_agent_api agt_status_t AGT_stdcall agt_reply_as(agt_self_t self, agt_agent_t spoofReplier, const agt_send_info_t* pSendInfo) AGT_noexcept;



/**
 * Raw send API
 * [ Unsafe ]
 *
 * Returns the raw message buffer to which a message can be directly written.
 * This can be used both to avoid extraneous memory copies, when message
 * data is memory position dependent, or to enable in-place construction of
 * non-trivially copyable C++ objects.
 *
 * Note that the buffer returned from agt_acquire_new must be treated like
 * memory acquired from malloc: all reads and writes of the returned buffer must
 * remain within the bounds [buffer,buffer+desiredMessageSize). If an out of
 * bound read/write is performed, behaviour is undefined, though it is highly
 * likely to cause a process to crash.
 *
 * the rawMsg parameter of each "agt_raw_send*" function must be a
 * valid message returned from a successful call to "agt_raw_acquire".
 * The buffer must no longer be read or written to once the message has been sent.
 * */


/**
 * @param [in]  recipient Intended receiver of the message. Used to select the message pool from which the message should be allocated. If null, message will be allocated from local message pool. The returned raw message must only be used for a call to "agt_raw_send_many*"
 * @param [in]  desiredMessageSize Desired size of the message. The buffer returned is guaranteed to be valid for at least this many bytes
 * @param [out] pRawMsg Handle for raw message. Used as a parameter in a subsequent send call.
 * */
AGT_agent_api agt_status_t AGT_stdcall agt_raw_acquire(agt_self_t self, agt_agent_t recipient, size_t desiredMessageSize, agt_raw_msg_t* pRawMsg, void** ppRawBuffer) AGT_noexcept;

/***/
AGT_agent_api agt_status_t AGT_stdcall agt_raw_send(agt_self_t self, agt_agent_t recipient, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

/***/
AGT_agent_api agt_status_t AGT_stdcall agt_raw_send_as(agt_self_t self, agt_agent_t spoofSender, agt_agent_t recipient, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

/***/
AGT_agent_api agt_status_t AGT_stdcall agt_raw_send_many(agt_self_t self, const agt_agent_t* recipients, agt_size_t agentCount, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

/***/
AGT_agent_api agt_status_t AGT_stdcall agt_raw_send_many_as(agt_self_t self, agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

/***/
AGT_agent_api agt_status_t AGT_stdcall agt_raw_reply(agt_self_t self, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

/***/
AGT_agent_api agt_status_t AGT_stdcall agt_raw_reply_as(agt_self_t self, agt_agent_t spoofSender, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;




AGT_agent_api void         AGT_stdcall agt_delegate(agt_self_t self, agt_agent_t recipient) AGT_noexcept;


/**
 * Signals early completion of a message process to any waiting agents.
 *
 * If a message does not call this function, by default, it will still "return" upon
 * the agent proc callback returning, but no value will be sent to any waiting parties.
 * */
AGT_agent_api void         AGT_stdcall agt_return(agt_self_t self, agt_u64_t value) AGT_noexcept;

AGT_agent_api void         AGT_stdcall agt_release(agt_self_t self, agt_agent_t agent) AGT_noexcept;

AGT_agent_api agt_pinned_msg_t AGT_stdcall agt_pin(agt_self_t self) AGT_noexcept;

AGT_agent_api void         AGT_stdcall agt_unpin(agt_self_t self, agt_pinned_msg_t pinnedMsg) AGT_noexcept;

/**
 * Signals normal termination of an agent.
 *
 * */
AGT_agent_api void         AGT_stdcall agt_exit(agt_self_t self, int exitCode) AGT_noexcept;

/**
 * Signals abnormal termination
 * */
AGT_agent_api void         AGT_stdcall agt_abort(agt_self_t self) AGT_noexcept;




/**
 * Interop with C++20 coroutine API.
 * Coroutine can be passed across C APIs as a simple address thanks to the standardized function calls
 *     - std::coroutine_handle<PromiseType>::address
 *     - std::coroutine_handle<PromiseType>::from_address
 * TODO: Figure out some way to set which C++ standard library implementation is used internally. Possibly with an environment variable?
 *       On windows, it should generally be safe to default to the MSVC coroutine implementation (what about MinGW/Cygwin?), but on
 *       unix-like systems, the choice between libstdc++ and libc++ is a meaningful one, and I'd have to double check but I'm virtually
 *       certain they're totally incompatible at the ABI level. At the very least there's no guarantee of compatibility.
 * */

AGT_agent_api void         AGT_stdcall agt_resume_coroutine(agt_agent_t receiver, void* coroutine, agt_async_t* asyncHandle) AGT_noexcept;



AGT_end_c_namespace


#endif//AGATE_AGENTS_H