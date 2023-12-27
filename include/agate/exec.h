//
// Created by maxwe on 2023-07-23.
//

#ifndef AGATE_EXEC_H
#define AGATE_EXEC_H

#include <agate/core.h>

AGT_begin_c_namespace

typedef struct agt_equeue_st* agt_equeue_t;
typedef void*                 agt_eagent_t;
typedef struct agt_fiber_st*  agt_fiber_t;
typedef struct agt_thread_st* agt_thread_t;

typedef struct agt_receiver_st* agt_receiver_t;


typedef enum agt_ecmd_t {
  AGT_ECMD_NOOP             = 0,    ///< As the name would imply, this is a noop.
  AGT_ECMD_KILL             = 1,    ///< Command sent on abnormal termination. Minimal cleanup is performed, typically indicates some unhandled error
  AGT_ECMD_CLOSE_QUEUE      = 2,    ///< Normal termination command. Any messages sent before this will be processed as normal, but no new messages will be sent. As soon as the queue is empty, the agent is destroyed.
  AGT_ECMD_INVALIDATE_QUEUE = 3,    ///< Current message queue is discarded without having been processed, but the queue is not closed, nor is the agent destroyed.

  AGT_ECMD_AGENT_MESSAGE    = 4,    ///< Process message sent to agent
  AGT_ECMD_SEND_AS,                 ///< Process message sent with agt_send_as
  AGT_ECMD_SEND_MANY,               ///< Process message sent with agt_send_many
  AGT_ECMD_SEND_MANY_AS,            ///< Process message sent with agt_send_many_as

  AGT_ECMD_GET_TIMESTAMP,           ///<

  AGT_ECMD_ATTACH_AGENT,            ///< Attach agent; if agent is eager, also invokes the start routine
  AGT_ECMD_DETACH_AGENT,            ///< Detach agent from this executor
  AGT_ECMD_REBIND_AGENT,            ///< Rebind agent to another executor

  AGT_ECMD_BARRIER_ARRIVE,          ///< When this message is dequeued, the arrival count of the provided barrier is incremented. If the post-increment arrival count is equal to the expected arrival count, any agents waiting on the barrier are unblocked, and if the barrier was set with a continuation callback, it is called (in the context of the last agent to arrive).
  AGT_ECMD_BARRIER_WAIT,            ///< If the arrival count of the provided barrier is less than the expected arrival count, the agent is blocked until they are equal. Otherwise, this is a noop.
  AGT_ECMD_BARRIER_ARRIVE_AND_WAIT, ///< Equivalent to sending AGT_CMD_BARRIER_ARRIVE immediately followed by AGT_CMD_BARRIER_WAIT
  AGT_ECMD_BARRIER_ARRIVE_AND_DROP, ///<

  AGT_ECMD_ACQUIRE_SEMAPHORE,       ///<
  AGT_ECMD_RELEASE_SEMAPHORE,       ///<

  AGT_ECMD_QUERY_AGENT_PROPERTY,    ///<
  AGT_ECMD_SET_AGENT_PROPERTY,      ///<

  AGT_ECMD_QUERY_AGENT_METHOD,      ///<
  AGT_ECMD_INVOKE_AGENT_METHOD,     ///<
  AGT_ECMD_REGISTER_AGENT_METHOD,   ///<
  AGT_ECMD_UNREGISTER_AGENT_METHOD, ///<

  AGT_ECMD_WRITE,                   ///<
  AGT_ECMD_READ,                    ///<
  AGT_ECMD_FLUSH,                   ///<

  AGT_ECMD_INVOKE_COROUTINE,        ///<

  AGT_ECMD_INVOKE_CALLBACK,         ///<

  AGT_ECMD_REGISTER_HOOK,           ///<
  AGT_ECMD_UNREGISTER_HOOK,         ///<



  AGT_ECMD_PING_STATUS,             ///< Sent every so often by other executors to test connection/latency
} agt_ecmd_t;


typedef enum agt_executor_kind_t {
  AGT_DEFAULT_EXECUTOR_KIND,
  AGT_EVENT_EXECUTOR_KIND,
  AGT_BUSY_EXECUTOR_KIND,
  AGT_PARALLEL_EXECUTOR_KIND,

} agt_executor_kind_t;


typedef enum agt_executor_flag_bits_t {
  AGT_EXECUTOR_IS_THREAD_SAFE = 0x1,
} agt_executor_flag_bits_t;
typedef agt_flags32_t agt_executor_flags_t;


typedef void (* AGT_stdcall agt_executor_proc_t)(agt_ctx_t ctx, agt_equeue_t equeue, void* userData);



typedef struct agt_eagent_header_t {
  void*        reserved;       ///< Do not access once initialized.
  union {
    agt_fiber_t  boundFiber;  ///< May read, but do not modify once initialized. Active if the executor uses fibers
    agt_thread_t boundThread; ///< May read, but do not modify once initialized. Active if the executor uses threads
  };
} agt_eagent_header_t;

typedef struct agt_agent_info_t  {
  agt_agent_flags_t flags;
  agt_string_t      name;
  agt_scope_t       scope;
} agt_agent_info_t;



typedef struct agt_executor_vtable_t {
  void            (* init)(void* userData, agt_equeue_t equeue);
  agt_eagent_t    (* attachAgent)(void* userData, const agt_agent_info_t* agentInfo);
  void            (* detachAgent)(void* userData, agt_eagent_t agentTag);
  agt_eagent_t    (* getReadyAgent)(void* userData);
  agt_bool_t      (* dispatchMessage)(void* userData, agt_ecmd_t cmd, agt_address_t paramA, agt_address_t paramB);
  void            (* readyAgent)(void* userData, agt_eagent_t agent);
  void            (* blockAgent)(void* userData, agt_eagent_t agent);
  void            (* blockAgentUntil)(void* userData, agt_eagent_t agent, agt_timestamp_t timestamp);
} agt_executor_vtable_t;


typedef struct agt_executor_desc_t {
  agt_executor_flags_t flags;
  agt_executor_kind_t  kind;
  agt_name_t           name;
  agt_u32_t            maxBoundAgents;
  agt_u32_t            stackSize;
  agt_u32_t            maxConcurrency;
  const void*          params; // type depends on value of agt_executor_desc_t::kind
} agt_executor_desc_t;

typedef struct agt_custom_executor_params_t {
  const agt_executor_vtable_t* vtable;
  agt_executor_proc_t          proc;
  void*                        userData;
  agt_receiver_t               receiver;
} agt_custom_executor_params_t;



/** ==================[   Executors General    ]================== **/


AGT_exec_api agt_status_t AGT_stdcall agt_new_executor(agt_ctx_t ctx, agt_executor_t* pExecutor, const agt_executor_desc_t* pExecutorDesc) AGT_noexcept;


AGT_exec_api agt_status_t AGT_stdcall agt_executor_start(agt_executor_t executor) AGT_noexcept;

AGT_exec_api agt_status_t AGT_stdcall agt_executor_start_on_current_thread(agt_executor_t executor) AGT_noexcept;





/** ==================[   Library Executors    ]================== **/














/** ==================[ User Defined Executors ]================== **/


// AGT_exec_api agt_status_t AGT_stdcall agt_new_equeue() AGT_noexcept;

// AGT_exec_api void         AGT_stdcall agt_destroy_equeue(agt_equeue_t equeue) AGT_noexcept;

// returns previously bound user data.
AGT_exec_api void*        AGT_stdcall agt_equeue_bind_user_data(agt_equeue_t equeue, void* userData) AGT_noexcept;

AGT_exec_api agt_status_t AGT_stdcall agt_equeue_dispatch(agt_equeue_t equeue, agt_duration_t timeout) AGT_noexcept;






AGT_end_c_namespace

#endif //AGATE_EXEC_H
