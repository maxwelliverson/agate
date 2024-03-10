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
typedef agt_u64_t                 agt_agent_handle_t; // For sending shared handles across shared channels
typedef agt_u64_t                 agt_typeid_t;
typedef agt_u64_t                 agt_raw_msg_t;

typedef struct agt_msg_st*        agt_msg_t;




typedef void* (* agt_agent_ctor_t)(const void* ctorUserData, void* params);
typedef void  (* agt_agent_init_t)(agt_self_t self, void* agentState);
typedef void  (* agt_agent_dtor_t)(agt_self_t self, void* agentState);
typedef void  (* agt_agent_proc_t)(agt_self_t self, void* agentState, const void* message, agt_size_t messageSize);


typedef enum agt_exec_mode_t {
  AGT_EXEC_EXCLUSIVE,
  AGT_EXEC_CONCURRENT
} agt_exec_mode_t;


typedef enum agt_enumeration_action_t {
  AGT_CONTINUE_ENUMERATION,
  AGT_FINISH_ENUMERATION,
} agt_enumeration_action_t;



typedef agt_enumeration_action_t (* agt_type_enumerator_t)(void* userData, const char* name, size_t nameLength, agt_typeid_t id);


typedef enum agt_agent_create_flag_bits_t {
  AGT_AGENT_CREATE_SHARED             = 0x01,
  AGT_AGENT_CREATE_DETACHED           = 0x02, ///< A detached agent is not reference counted and is responsible for its own lifetime. This can be useful for breaking reference cycles for instance.
  AGT_AGENT_CREATE_BUSY               = 0x04,
  AGT_AGENT_CREATE_WITH_NEW_EXECUTOR  = 0x08,
  AGT_AGENT_CREATE_ONLY_EXCLUSIVE     = 0x10, ///< The execution mode of the created agent cannot be set to AGT_EXEC_CONCURRENT.
  AGT_AGENT_CREATE_DEFAULT_CONCURRENT = 0x20,
  AGT_AGENT_CREATE_TRIVIAL            = 0x80, ///< The created agent must NOT have any possibility of blocking
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
  AGT_SEND_READ_ONLY_MESSAGE = 0x40
} agt_send_flag_bits_t;
typedef agt_flags32_t agt_send_flags_t;


typedef enum agt_msg_flag_bits_t {

} agt_msg_flag_bits_t;
typedef agt_flags32_t agt_msg_flags_t;


typedef struct agt_send_info_t {
  agt_size_t       size;
  const void*      buffer;
  agt_async_t      async;
  agt_send_flags_t flags;
} agt_send_info_t;

typedef struct agt_raw_send_info_t {
  agt_raw_msg_t    msg;
  size_t           msgSize;
  agt_async_t      async;
  agt_send_flags_t flags;
} agt_raw_send_info_t;


typedef struct agt_msg_info_t {
  const void*     data;
  size_t          size;
  agt_msg_flags_t flags;
  agt_agent_t     sender;
  agt_timestamp_t sendTime;
  agt_timestamp_t receiveTime;
} agt_msg_info_t;




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
  agt_executor_t           executor;         ///< [optional] The executor responsible for executing agent actions. If null and within an agent execution context, will try to use the current executor. If that fails for some reason, or if this field is null and not within an agent execution context, a default executor will be created as per context settings.
  agt_agent_init_t         initFn;           ///< [optional] Initial callback executed in the agent's context before it starts receiving messages.
  agt_agent_proc_t         procFn;           ///< Main callback executed in the agent's context
  agt_agent_dtor_t         dtorFn;           ///< [optional] Destructor, called on state when agent is destroyed
  void*                    state;            ///< [optional] The argument passed to the agent's constructor. If the agent does not have a constructor, then this points to the agent state (NOTE: in this case, the memory pointed to by userData must remain valid for the entire lifetime of the agent. May be null if the agent type has no state).
} agt_agent_create_info_t;



/* =================[ Agent Types ]================= */


// agt_status_t AGT_stdcall agt_register_type(agt_ctx_t ctx, const agt_agent_type_create_info_t* cpCreateInfo, agt_typeid_t* pId) AGT_noexcept;

// size_t       AGT_stdcall agt_enumerate_types(agt_ctx_t ctx, const char* pattern, size_t patternLength, agt_type_enumerator_t enumerator, void* userData) AGT_noexcept;



/* =================[ Creation Functions ]================= */


/***/

AGT_core_api agt_status_t AGT_stdcall agt_create_agent(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo, agt_agent_t* pAgent) AGT_noexcept;

// agt_status_t AGT_stdcall agt_create_agent_from_type(agt_ctx_t ctx, ) AGT_noexcept;


/**
 * Surrenders the current thread to a newly created BusyAgent.
 *
 * A typical pattern would involve creating the initial ecosystem of agents in main,
 * followed by a call to this function to start the execution of the system.
 * This function may *not* be called within an agent context
 * */
AGT_noreturn AGT_core_api void AGT_stdcall agt_create_busy_agent_on_current_thread(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_create_executor(agt_ctx_t ctx, const agt_executor_create_info_t* cpCreateInfo, agt_executor_t* pExecutor) AGT_noexcept;


/* =============[ Queries ]============= */

/**
 * If agent is null, then the executor for self is returned instead.
 * */
AGT_core_api agt_status_t AGT_stdcall agt_get_executor(agt_self_t self, agt_agent_t agent, agt_executor_t* pResult) AGT_noexcept;


/* ========================= [ Agents ] ========================= */

AGT_core_api agt_self_t   AGT_stdcall agt_self(agt_ctx_t ctx) AGT_noexcept;

AGT_core_api agt_ctx_t    AGT_stdcall agt_self_ctx(agt_self_t self) AGT_noexcept;

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
AGT_core_api agt_handle_t AGT_stdcall agt_export_self(agt_self_t self) AGT_noexcept;

AGT_core_api agt_agent_t  AGT_stdcall agt_retain_sender(agt_self_t self) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_retain(agt_agent_t* pNewAgent, agt_agent_t agent) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_retain_self(agt_self_t self, agt_agent_t* pSelfAgent) AGT_noexcept;



AGT_core_api agt_status_t AGT_stdcall agt_send(agt_self_t self, agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_send_as(agt_self_t self, agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_send_many(agt_self_t self, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_send_many_as(agt_self_t self, agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_reply(agt_self_t self, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_reply_as(agt_self_t self, agt_agent_t spoofReplier, const agt_send_info_t* pSendInfo) AGT_noexcept;



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
AGT_core_api agt_status_t AGT_stdcall agt_raw_acquire(agt_self_t self, agt_agent_t recipient, size_t desiredMessageSize, agt_raw_send_info_t* pRawSendInfo, void** ppRawBuffer) AGT_noexcept;

/***/
AGT_core_api agt_status_t AGT_stdcall agt_raw_send(agt_self_t self, agt_agent_t recipient, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

/***/
AGT_core_api agt_status_t AGT_stdcall agt_raw_send_as(agt_self_t self, agt_agent_t spoofSender, agt_agent_t recipient, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

/***/
AGT_core_api agt_status_t AGT_stdcall agt_raw_send_many(agt_self_t self, const agt_agent_t* recipients, agt_size_t agentCount, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

/***/
AGT_core_api agt_status_t AGT_stdcall agt_raw_send_many_as(agt_self_t self, agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

/***/
AGT_core_api agt_status_t AGT_stdcall agt_raw_reply(agt_self_t self, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

/***/
AGT_core_api agt_status_t AGT_stdcall agt_raw_reply_as(agt_self_t self, agt_agent_t spoofSender, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;



/**
 * Forwards the current message to recipient, as though it was sent directly to recipient in the first place.
 *
 * Primarily intended for use by delegation agents, whose primary responsibility is to sort through received
 * messages and forward them to the agents that will actually process them.
 *
 */
AGT_core_api void         AGT_stdcall agt_delegate(agt_self_t self, agt_agent_t recipient) AGT_noexcept;


/**
 * Signals early completion of a message process to any waiting agents.
 *
 * If a message does not call this function, by default, it will still "return" upon
 * the agent proc callback returning, but no value will be sent to any waiting parties.
 * */
AGT_core_api void         AGT_stdcall agt_return(agt_self_t self, agt_u64_t value) AGT_noexcept;


AGT_core_api void         AGT_stdcall agt_release(agt_self_t self, agt_agent_t agent) AGT_noexcept;



/**
 * Sets the current execution mode. Lasts the duration of the message currently being processed.
 *
 *   AGT_EXEC_EXCLUSIVE -> AGT_EXEC_CONCURRENT: Drops exclusive access, any concurrent messages that had previously been blocked may now run.
 *   AGT_EXEC_CONCURRENT -> AGT_EXEC_EXCLUSIVE: Tries to acquire exclusive access; If self has no other concurrent messages running, this returns immediately and execution may continue. Otherwise, execution of this message blocks until other concurrent processes finish execution. New concurrent processes will be blocked from starting execution so as not to risk indefinite blockage.
 *
 * If the set mode is equal to the current mode, this is a noop.
 * */
AGT_core_api void            AGT_stdcall agt_set_exec_mode(agt_self_t self, agt_exec_mode_t mode) AGT_noexcept;

AGT_core_api agt_exec_mode_t AGT_stdcall agt_get_exec_mode(agt_self_t self) AGT_noexcept;






AGT_core_api agt_msg_t    AGT_stdcall agt_msg_retain(agt_self_t self) AGT_noexcept;

AGT_core_api void         AGT_stdcall agt_msg_get_info(agt_self_t self, agt_msg_t msg, agt_msg_info_t* pMsgInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_msg_reply(agt_self_t self, agt_msg_t msg, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_msg_reply_as(agt_self_t self, agt_msg_t msg, agt_agent_t spoofReplier, const agt_send_info_t* pSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_msg_raw_reply(agt_self_t self, agt_msg_t msg, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_msg_raw_reply_as(agt_self_t self, agt_msg_t msg, agt_agent_t spoofSender, const agt_raw_send_info_t* pRawSendInfo) AGT_noexcept;

AGT_core_api void         AGT_stdcall agt_msg_delegate(agt_self_t self, agt_msg_t msg, agt_agent_t recipient) AGT_noexcept;

AGT_core_api void         AGT_stdcall agt_msg_return(agt_self_t self, agt_msg_t msg, agt_u64_t value) AGT_noexcept;

AGT_core_api void         AGT_stdcall agt_msg_release(agt_self_t self, agt_msg_t msg) AGT_noexcept;





AGT_core_api void         AGT_stdcall agt_yield(agt_self_t self) AGT_noexcept;


/**
 * Signals normal termination of an agent.
 *
 * */
AGT_core_api void         AGT_stdcall agt_exit(agt_self_t self, int exitCode) AGT_noexcept;

/**
 * Signals abnormal termination
 * */
AGT_core_api void         AGT_stdcall agt_abort(agt_self_t self) AGT_noexcept;




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

AGT_core_api void         AGT_stdcall agt_resume_coroutine(agt_agent_t receiver, void* coroutine, agt_async_t async) AGT_noexcept;



AGT_end_c_namespace


#endif//AGATE_AGENTS_H
