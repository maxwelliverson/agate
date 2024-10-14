//
// Created by maxwe on 2023-07-23.
//

#ifndef AGATE_SCHED_H
#define AGATE_SCHED_H

#include <agate/core.h>

AGT_begin_c_namespace

// TODO: Rename executors to something better indicating that they are
//       essentially full sched implementations for the builtin agent
//       library. Possibly agent_sched? asched?

/**
 * ==================[ User Defined Task Management ]===================
 *
 * This library consists of a couple key types:
 *
 *   - agt_sched_t: Schedulers; defines mechanisms for suspending and resuming tasks,
 *                  and is responsible for determining which tasks are run next.
 *   - agt_task_t:  Tasks; definition and use is entirely left up to a given scheduler.
 *
 * The key abstractions of the library are as follows:
 *
 *   - Scheduler:          An abstract interface whose instances are responsible
 *                         for the scheduling of tasks.
 *   - Task:               An abstract unit of work; any given task is owned by
 *                         a specific scheduler instance, and may only run on
 *                         contexts that are a part of that instance.
 *   - Context Scheduler:  The binding of a scheduler entity to a given context.
 *                         Every context has exactly one.
 *   - Scheduler Instance: A set of one or more context schedulers that share
 *                         ownership of a set of tasks. Every context bound to
 *                         the same scheduler entity shares a scheduler instance
 *                         if that entity is cross-context enabled. Otherwise,
 *                         context schedulers bound to the same entity are all
 *                         seperate instances.
 *   - Scheduler Entity:   The result of an invocation of agt_new_sched.
 *
 *
 *
 *
 *
 *
 *
 *
 * */




typedef struct agt_sched_st*  agt_sched_t;
typedef void*                 agt_task_t;

typedef struct agt_bound_task_st* agt_bound_task_t;


/// Structure that is passed to agt_sched_vtable_t::resume.
/// Contains information about the task to be resumed.
typedef struct agt_suspended_task_t {
  agt_ctx_t   lastCtx;  ///< The last context this task ran on. This is only meaningful for sched objects created with AGT_SCHED_IS_CROSS_CONTEXT
  agt_task_t  task;     ///< The task to be resumed.
  void*       userData; ///< The user specified data bound to lastCtx's context-scheduler.
} agt_suspended_task_t;


// typedef struct agt_


typedef enum agt_sched_flag_bits_t {
  AGT_SCHED_IS_SHARED        = 0x1, ///< sched is inter-process.
  AGT_SCHED_IS_CROSS_CONTEXT = 0x2, ///< tasks may be run on any context to which this sched is bound.
  AGT_SCHED_USES_FIBERS      = 0x4, ///< a sched uses fibers for multi-tasking
  AGT_SCHED_AUTO_INHERIT     = 0x8, ///< contexts created by any context to which this sched is bound will have this sched bound to them automatically.
} agt_sched_flag_bits_t;
typedef agt_flags32_t agt_sched_flags_t;


/**
 * IMPORTANT NOTE FOR IMPLEMENTATIONS
 *
 * Implementations of sched should *NOT* assume that
 * a call to resume will be sequenced strictly after
 * suspend or suspend_for are invoked. In other words,
 * suspend, suspend_for, and resume should all be
 * implemented in such a fashion that execution is *not*
 * suspended in suspend(_until) if resume has already
 * been called for a given task.
 * */



typedef AGT_alignas(64) struct agt_sched_vtable_t {
  /**
   * Allow some other task to execute without 'blocking' the current task from executing.
   * Returns AGT_TRUE is control was successfully yielded (ie. some other task was
   * allowed to run), otherwise returns AGT_FALSE.
   *
   * This method is optional. If null, yield operations will behave as though this were a
   * noop that returned AGT_FALSE.
   *
   * If it is not necessarily known whether control was yielded (eg. if the implementation calls the POSIX sched_yield), return AGT_TRUE.
   * */
  agt_bool_t      (* yield)(agt_ctx_t ctx, void* userData);

  // Suspends the current task indefinitely, such that this call does not return
  // until 'resume' is called on the given task.
  void            (* suspend)(agt_ctx_t ctx, agt_bound_task_t boundTask, void* userData);

  // Suspends 'task' with a timeout, such that this call returns either when
  // 'resume' is called with the same task, OR after timeout has passed (timeout is in nanoseconds)
  // If the suspend times out, this should return AGT_FALSE, otherwise it should return AGT_TRUE.
  // Other values of agt_status_t may be returned, but (for now) the internal mechanisms of the library
  // will treat any other returned status values as a fatal error.
  agt_bool_t      (* suspend_until)(agt_ctx_t ctx, agt_bound_task_t boundTask, agt_timestamp_t deadline, void* userData);

  // Resumes the given task. prevCtxExec is the ctxexec that was passed to the prior call to suspend or
  // suspend_for, but should not be assumed to be the current ctxexec, as any thread may be responsible for
  // calling resume.
  // Returns true if this call was responsible for successfully resuming the task; returns false if the task
  // had already been resumed (eg. it had been suspended with a call to suspend_until that had since timed out).
  // IMPORTANT: Unlike all other scheduler methods, resume may be called by *ANY* context. Implementations
  //            should not simply assume that the task may be restarted on the current context, and signaling
  //            methods used to indicate that a task should be resumed should be thread-safe. Note that this
  //            thread-safety is only relevant to user code; there aren't any internal mechanisms that require
  //            it. As such, if it can be determined that thread-safety is not necessary (for instance, if it
  //            is known that the current thread is also the only one that may resume the task), then it may
  //            be eschewed.
  agt_bool_t      (* resume)(agt_ctx_t ctx, const agt_suspended_task_t* pSuspendedTask);


  // OPTIONAL: If null, precise suspends will default to using the non-precise suspend_until.
  agt_bool_t      (* precise_suspend_until)(agt_ctx_t ctx, agt_bound_task_t boundTask, agt_timestamp_t deadline, void* userData);


  /**
   * This will be called during binding for each context this scheduler is bound to.
   *
   * @param ctx The context to which this sched is being bound
   * @param pTask A pointer to the active task. This should be stored
   *              somewhere so that subsequent operations can change
   *              the active task.
   * @param bindData A user specified pointer passed to agt_new_sched
   *
   * @return A pointer that will be passed to each of the other callbacks. Intended to act as 'context local storage' for this sched.
   * */
  void*           (* on_bind)(agt_ctx_t ctx, agt_task_t* pTask, void* bindData);

  // OPTIONAL: If specified, this function will be called when the context scheduler is unbound.
  //           Use this to perform any necessary cleanup of the context scheduler state.
  void            (* on_unbind)(agt_ctx_t ctx, void* userData);

  // OPTIONAL: If specified, this function will be called when the scheduler entity is destroyed.
  //           Use this to perform any necessary cleanup of the entity bind data.
  void            (* on_destroy)(agt_ctx_t ctx, void* bindData);
} agt_sched_vtable_t;

typedef struct agt_sched_desc_t {
  agt_sched_flags_t         flags;    ///< [optional]
  void*                     bindData; ///< [optional] If vtable->on_bind is non-null, this value will be passed to the bindData parameter whenever called. Intended to allow implementations of sched to use cross-context scheduling.
  const agt_sched_vtable_t* vtable;   ///< this pointer must remain valid for the entire use of uexec. Ideally, the vtables should be stored in constant memory.
} agt_sched_desc_t;



// Note: There is *intentionally* no API that returns the current sched

/** ==================[ User Defined Executors ]================== **/




AGT_exec_api agt_status_t AGT_stdcall agt_new_sched(agt_ctx_t ctx, agt_sched_t* pSched, const agt_sched_desc_t* pSchedDesc) AGT_noexcept;

/**
 * Closes an open handle to a sched object.
 *
 * Note: Calling agt_bind_sched does take ownership from the passed handle. As such, the
 *       it is only necessary to call agt_close_sched on handles that have been duplicated.
 * */
AGT_exec_api void         AGT_stdcall agt_close_sched(agt_ctx_t ctx, agt_sched_t sched) AGT_noexcept;

/**
 *
 * Note: Once explicitly bound
 *
 * @returns One of \n
 *              - AGT_ERROR_ALREADY_BOUND: A sched object has already been bound to this context.
 * */
AGT_exec_api agt_status_t AGT_stdcall agt_bind_sched(agt_ctx_t ctx, agt_sched_t sched) AGT_noexcept;



/**
 *
 * */
AGT_exec_api agt_status_t AGT_stdcall agt_alloc_task(agt_ctx_t ctx, size_t size, agt_task_t* pTask) AGT_noexcept;

AGT_exec_api agt_status_t AGT_stdcall agt_recycle_task(agt_ctx_t ctx) AGT_noexcept;

AGT_exec_api void         AGT_stdcall agt_free_task(agt_ctx_t ctx, agt_task_t task) AGT_noexcept;


// If returns true, it was successfully suspended. If returns false, resume immediately.
AGT_exec_api agt_bool_t   AGT_stdcall agt_sched_task_suspend(agt_ctx_t ctx, agt_bound_task_t queue) AGT_noexcept;

// If returns true, this call successfully resumed task. If returns false, then task was already resumed.
// Intended to be called
AGT_exec_api agt_bool_t   AGT_stdcall agt_sched_task_resume(agt_ctx_t ctx, agt_bound_task_t queue) AGT_noexcept;

AGT_exec_api agt_bool_t   AGT_stdcall agt_sched_task_wake() AGT_noexcept;

/**
 * Utility function that indicates whether or not the two contexts share the same bound sched object.
 *
 * Note that this checks whether or not the two contexts are bound to the exact same cross-context
 * enabled sched object. This is intended allow users to determine whether or not a task created on
 * one context may be ran on the other.
 *
 * */
AGT_exec_api agt_bool_t  AGT_stdcall agt_ctx_has_same_sched(agt_ctx_t ctx, agt_ctx_t otherCtx) AGT_noexcept;




// For now, we avoid these problems by having sched be locked once bound.

/*// Returns true on success, and writes the popped sched to pPoppedSched if pPoppedSched is nonnull.
// NOTE: For safety reasons, this function will only succeed if called from within the same module
// that created the bound sched. This is to help protect against foreign code being able to maliciously
// wrest control of an application's scheduler. If a mismatch is detected, the pop fails, and
// AGT_ERROR_NO_OWNERSHIP is returned.
// If the bound sched is the default sched, returns AGT_ERROR_NOT_BOUND
AGT_exec_api agt_status_t AGT_stdcall agt_pop_sched(agt_ctx_t ctx, agt_sched_t* pPoppedSched) AGT_noexcept;


// Like with agt_pop_sched, this call only succeeds if called from the module from which
// the currently bound sched was created (NOTE: not newSched)
AGT_exec_api agt_status_t AGT_stdcall agt_push_sched(agt_ctx_t ctx, agt_sched_t newSched) AGT_noexcept;*/



AGT_end_c_namespace

#endif//AGATE_SCHED_H
