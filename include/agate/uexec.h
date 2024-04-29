//
// Created by maxwe on 2023-07-23.
//

#ifndef AGATE_UEXEC_H
#define AGATE_UEXEC_H

#include <agate/core.h>

#define AGT_DEFAULT_SCHED_KEY ((agt_sched_key_t)0)

AGT_begin_c_namespace


typedef struct agt_fiber_st*  agt_fiber_t;
typedef struct agt_thread_st* agt_thread_t;

typedef struct agt_uexec_st*  agt_uexec_t;
typedef void*                 agt_utask_t;

typedef struct agt_sched_st*  agt_sched_t;
typedef agt_u64_t             agt_sched_key_t;


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



/**
 * IMPORTANT NOTE FOR IMPLEMENTATIONS
 *
 * Implementations of uexec should *NOT* assume that
 * a call to resume will be sequenced strictly after
 * suspend or suspend_for are invoked. In other words,
 * suspend, suspend_for, and resume should all be
 * implemented in such a fashion that execution is *not*
 * suspended in suspend(_for) if resume has already
 * been called for a given task.
 * */

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
  // 'resume' is called with the same task, OR after timeout has passed (timeout is in nanoseconds)
  // If the suspend times out, this should return AGT_FALSE, otherwise it should return AGT_TRUE.
  // Other values of agt_status_t may be returned, but (for now) the internal mechanisms of the library
  // will treat any other returned status values as a fatal error.
  agt_bool_t      (* suspend_for)(agt_ctx_t ctx, agt_ctxexec_t execCtxData, agt_timeout_t timeout);

  // Resumes the given task. prevCtxExec is the ctxexec that was passed to the prior call to suspend or
  // suspend_for, but should not be assumed to be the current ctxexec, as any thread may be responsible for
  // calling resume.
  void            (* resume)(agt_ctx_t ctx, agt_ctxexec_t prevCtxExec, agt_utask_t task);



  // IDEAS FOR OTHERS
  // replace suspend_for with suspend_until (timeout to deadline, so that deadline may be calculated from a given timeout at the start of a procedure)
  // precise_suspend_until (optional; if available, some APIs may use it over suspend_until, with the idea being that it allows for a more precise deadline, generally at the expense of system throughput).
  // on_close (called when the uexec is unbound)

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

AGT_exec_api agt_status_t AGT_stdcall agt_bind_sched(agt_ctx_t ctx, agt_sched_t sched, agt_utask_t** ppTask) AGT_noexcept;


// Returns true on success, and writes the popped sched to pPoppedSched if pPoppedSched is nonnull.
// NOTE: For safety reasons, this function will only succeed if called from within the same module
// that created the bound sched. This is to help protect against foreign code being able to maliciously
// wrest control of an application's scheduler. If a mismatch is detected, the pop fails, and
// AGT_ERROR_NO_OWNERSHIP is returned.
// If the bound sched is the default sched, returns AGT_ERROR_NOT_BOUND
AGT_exec_api agt_status_t AGT_stdcall agt_pop_sched(agt_ctx_t ctx, agt_sched_t* pPoppedSched) AGT_noexcept;


// Like with agt_pop_sched, this call only succeeds if called from the module from which
// the currently bound sched was created (NOTE: not newSched)
AGT_exec_api agt_status_t AGT_stdcall agt_push_sched(agt_ctx_t ctx, agt_sched_t newSched) AGT_noexcept;

// AGT_exec_api agt_bool_t   AGT_stdcall agt_pop_sched(agt_ctx_t ctx, agt_sched_key_t key, agt_sched_t* pPoppedSched) AGT_noexcept;

// AGT_exec_api void         AGT_stdcall agt_push_sched(agt_ctx_t ctx, agt_sched_t sched) AGT_noexcept;


/**
 * Primarily intended to allow clients to determine whether or not a particular call to 'resume' is being made
 * by the same uexec as the one that suspended it (not all uexecs need to do this, but it may be useful in some cases).
 * */
AGT_exec_api agt_uexec_t  AGT_stdcall agt_get_current_uexec(agt_ctx_t ctx) AGT_noexcept;


AGT_end_c_namespace

#endif//AGATE_UEXEC_H
