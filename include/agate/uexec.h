//
// Created by maxwe on 2023-07-23.
//

#ifndef AGATE_UEXEC_H
#define AGATE_UEXEC_H

#include <agate/core.h>

AGT_begin_c_namespace


typedef struct agt_fiber_st*  agt_fiber_t;
typedef struct agt_thread_st* agt_thread_t;

typedef struct agt_uexec_st*  agt_uexec_t;
typedef void*                 agt_utask_t;



typedef struct agt_ctxexec_header_t {
  agt_utask_t activeTask; ///< Clients may inspect/modify the active task by setting the value of this field.
} agt_ctxexec_header_t; // Not used directly by the API
typedef void*  agt_ctxexec_t; // may be any type, so long as it is pointer-interconvertible with agt_ctxexec_header_t


typedef enum agt_uexec_flag_bits_t {
  AGT_UEXEC_IS_SHARED         = 0x1,
  AGT_UEXEC_IS_MULTI_THREADED = 0x2,
  AGT_UEXEC_USES_FIBERS       = 0x4,
} agt_uexec_flag_bits_t;
typedef agt_flags32_t agt_uexec_flags_t;


// Aligning to 32 bytes ensures that this struct will reside in a single cacheline.
typedef AGT_alignas(32) struct agt_uexec_vtable_t {
  // Allow some other task to execute without
  // 'blocking' the current task from executing.
  // Optional; If null, then yield operations will simply return immediately.
  // Returns AGT_TRUE if control was successfully yielded, otherwise returns AGT_FALSE.
  // If it is not necessarily known whether control was yielded (eg. if the implementation calls the POSIX sched_yield), return AGT_TRUE.
  agt_bool_t      (* yield)(agt_ctx_t ctx, agt_ctxexec_t execCtxData);

  // Suspends the current task indefinitely, such that this call does not return
  // until 'resume' is called on the given task.
  void            (* suspend)(agt_ctx_t ctx, agt_ctxexec_t execCtxData);

  // Suspends 'task' with a timeout, such that this call returns either when
  // 'resume' is called with the same task, OR after timeout has passed (timeout is in microseconds)
  // If the suspend times out, this should return AGT_TIMED_OUT, otherwise it should return AGT_SUCCESS.
  // Other values of agt_status_t may be returned, but (for now) the internal mechanisms of the library
  // will treat any other returned status values as a fatal error.
  agt_bool_t      (* suspend_for)(agt_ctx_t ctx, agt_ctxexec_t execCtxData, agt_timeout_t timeout);

  // Resumes the given task. prevCtxExec is the ctxexec that was passed to the prior call to suspend or
  // suspend_for, but should not be assumed to be the current ctxexec, as any thread may be responsible for
  // calling resume.
  void            (* resume)(agt_ctx_t ctx, agt_ctxexec_t prevCtxExec, agt_utask_t task);

  // Maybe include another function pointer to something like 'onCtxClose' or 'onUnbind(agt_ctx_t, agt_ctxexec_t, agt_bool_t isManualUnbind)' so that the uexec may react to being unbound (especially in cases where that might be unexpected, like early thread termination)
} agt_uexec_vtable_t;


typedef struct agt_uexec_desc_t {
  agt_uexec_flags_t         flags;
  const agt_uexec_vtable_t* vtable; // this pointer must remain valid for the entire use of uexec. Ideally, the vtables should be stored in constant memory.
} agt_uexec_desc_t;



/** ==================[ User Defined Executors ]================== **/




AGT_exec_api agt_status_t AGT_stdcall agt_new_uexec(agt_ctx_t ctx, agt_uexec_t* pExec, const agt_uexec_desc_t* pExecDesc) AGT_noexcept;

AGT_exec_api void         AGT_stdcall agt_destroy_uexec(agt_ctx_t ctx, agt_uexec_t exec) AGT_noexcept;


/**
 * @param ctxexec A pointer to a user-defined object that will be passed to the callback functions found in exec's vtable when invoked in the current context.
 *                The lifetime of this object must outlast
 *                This object must be pointer-interconvertible with agt_ctxexec_header_t.
 * */
AGT_exec_api agt_status_t AGT_stdcall agt_bind_uexec_to_ctx(agt_ctx_t ctx, agt_uexec_t exec, agt_ctxexec_t ctxexec) AGT_noexcept;


/**
 * Primarily intended to allow clients to determine whether or not a particular call to 'resume' is being made
 * by the same uexec as the one that suspended it (not all uexecs need to do this, but it may be useful in some cases).
 * */
AGT_exec_api agt_uexec_t  AGT_stdcall agt_get_current_uexec(agt_ctx_t ctx) AGT_noexcept;


AGT_end_c_namespace

#endif//AGATE_UEXEC_H
