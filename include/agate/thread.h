//
// Created by maxwe on 2023-08-09.
//

#ifndef AGATE_THREAD_H
#define AGATE_THREAD_H

#include <agate/core.h>


AGT_begin_c_namespace

typedef struct agt_thread_st*  agt_thread_t;


typedef enum agt_thread_flag_bits_t {
  AGT_THREAD_CREATE_SUSPENDED = 0x1,  ///< Thread starts suspended; the newly created thread does not begin execution until agt_start_thread is called on it.
  AGT_THREAD_CREATE_AS_CHILD  = 0x2,  ///< Note that unlike many popular thread APIs, agate threads are detached by default, and must be explicitly tied to the creating thread by specifying this flag in the call to agt_thread_desc_t::flags. When this flag is specified, when the thread handle is closed, the calling execution context will be blocked until the specified thread exits.
} agt_thread_flag_bits_t;
typedef agt_flags32_t agt_thread_flags_t;

typedef agt_u64_t (* AGT_stdcall agt_thread_proc_t)(agt_ctx_t ctx, void* userData);

typedef struct agt_thread_desc_t {
  agt_thread_flags_t flags;
  agt_u32_t          stackSize; ///< [optional] If 0, the platform's default stack size will be used.
  agt_priority_t     priority;
  agt_string_t       description;
  agt_thread_proc_t  proc;
  void*              userData;
} agt_thread_desc_t;






/** ==================[        Threads         ]================== **/


// Creates a new thread. If no flags are specified in agt_thread_desc_t::flags, pHandle may be null. If pHandle is not null and the call returns AGT_SUCCESS, a handle to the newly created thread is assigned to the address specified by pHandle. In this case, the returned handle must be closed by a call to agt_close()
AGT_exec_api agt_status_t AGT_stdcall agt_new_thread(agt_ctx_t ctx, agt_thread_t* pHandle, const agt_thread_desc_t* pDesc) AGT_noexcept;


// Start execution of a thread that specified the AGT_THREAD_CREATE_SUSPENDED flag upon creation.
AGT_exec_api agt_status_t AGT_stdcall agt_start_thread(agt_ctx_t ctx, agt_thread_t thread) AGT_noexcept;


AGT_exec_api void         AGT_stdcall agt_async_attach_thread(agt_async_t async, agt_thread_t thread) AGT_noexcept;




AGT_end_c_namespace

#endif//AGATE_THREAD_H
