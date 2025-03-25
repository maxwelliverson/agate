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

// typedef struct agt_bound_task_st* agt_bound_task_t;
typedef struct agt_task_waker_st* agt_task_waker_t;

// typedef agt_u32_t             agt_suspend_id_t;


// ORDERED_WAKING is more like 'conditional_waking'. If specified, this means that agt_wake_task will be guaranteed to not be called on a task that has already woken up. If not specified, agt_wake_task MUST be called on every task at some point. Use this, for example, in a timed callback that runs unconditionally.
typedef enum agt_sched_flag_bits_t {
  AGT_SCHED_IS_SHARED              = 0x01,  ///< sched is inter-process.
  AGT_SCHED_IS_CROSS_CONTEXT       = 0x02,  ///< tasks may be run on any context to which this sched is bound.
  AGT_SCHED_USES_FIBERS            = 0x04,  ///< a sched uses fibers for multi-tasking
  AGT_SCHED_AUTO_INHERIT           = 0x08,  ///< contexts created by any context to which this sched is bound will have this sched bound to them automatically.
  AGT_SCHED_ORDERED_WAKING         = 0x10,  ///< Indicates that the waker does not need to perform any additional synchronization, allowing for improved performance. This flag may only be specified if the scheduler can guarantee that agt_wake_task will never be called on a task that has already been woken.
  AGT_SCHED_SKIP_RESUME_ON_TIMEOUT = 0x20,  ///< Indicates that this scheduler's resume method would do nothing if invoked during agt_wake_task, and that as such, it is not invoked (this will typically be the case if we know that whatever action is taken by resume must have already occured by the time that agt_wake_task is called)
} agt_sched_flag_bits_t;
typedef agt_flags32_t agt_sched_flags_t;

typedef enum agt_suspend_flag_bits_t {
  AGT_SUSPEND_IS_PRECISE      = 0x1,
  AGT_SUSPEND_STRICT_DEADLINE = 0x8, ///< If this flag is specified, any 'wake' that happens after the deadline has passed is guaranteed to fail. If not specified, wakes that are sequenced after the deadline but before the task has resumed will mean that the task will resume with the wait having "succeeded" rather than timed out.
  // as a general rule, 'wait until' operations with an explicit deadline likely want this flag, whereas 'wait for' operations with only a timeout given likely don't.
} agt_suspend_flag_bits_t;
typedef agt_flags32_t agt_suspend_flags_t;


typedef enum agt_resume_reason_t {
  AGT_RESUME_REASON_TIMED_OUT, ///< What should be passed to agt_wake_task when waking after the deadline has passed.
  AGT_RESUME_REASON_WOKEN,     ///< Non-specific reason;
  // TODO: Add more resume reasons maybe corresponding to certain synchronization objects
  //         eg. periodic timer that indicates every time it goes off as such
} agt_resume_reason_t;


typedef struct agt_suspend_info_t {
  agt_u32_t           structSize;
  agt_suspend_flags_t flags;
  agt_task_t          task;
  agt_task_t          recommendedNext; /** < If present, indicates a task that the implementation suggests be run next. This may be set, for example, to a task waiting on a mutex that the suspended task owned. */
} agt_suspend_info_t;

typedef struct agt_suspend_until_info_t {
  agt_u32_t           structSize;
  agt_suspend_flags_t flags;
  agt_task_t          task;
  agt_timestamp_t     deadline;
  agt_task_waker_t    waker;
  agt_task_t          recommendedNext; /** < If present, indicates a task that the implementation suggests be run next. This may be set, for example, to a task waiting on a mutex that the suspended task owned. */
} agt_suspend_until_info_t;



/// Structure that is passed to agt_sched_vtable_t::resume.
/// Contains information about the task to be resumed.
typedef struct agt_suspended_task_t {
  agt_ctx_t   lastCtx;  ///< The context this task last ran on.
  agt_task_t  task;     ///< The task to be resumed.
  void*       userData; ///< The user specified data bound to lastCtx's context-scheduler.
} agt_suspended_task_t;





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

  /**
   * Suspends the current task indefinitely, such that this call does not return
   * until 'resume' is called on the given task.
   * This call returning will cause execution of the task that was current when suspend was called
   * to resume.
   * */
  void            (* suspend)(agt_ctx_t ctx, const agt_suspend_info_t* pSuspendInfo, void* userData);

  /**
   * Suspends 'task' with a timeout, such that this call returns either when
   * 'resume' is called with the same task, OR after timeout has passed (timeout is in nanoseconds)
   * If the suspend times out, this should return AGT_FALSE, otherwise it should return AGT_TRUE.
   * Other values of agt_status_t may be returned, but (for now) the internal mechanisms of the library
   * will treat any other returned status values as a fatal error.
   */
  agt_bool_t      (* suspend_until)(agt_ctx_t ctx, const agt_suspend_until_info_t* pSuspendInfo, void* userData);

  /**
   * Resumes the given task. prevCtxExec is the ctxexec that was passed to the prior call to suspend or
   * suspend_for, but should not be assumed to be the current ctxexec, as any thread may be responsible for
   * calling resume.
   * This call is guaranteed to only be called ONCE per suspension, and only by whoever is successfully
   * responsible for waking. The 'reason' parameter can be understood as communication the
   * cause of the wake up.
   * IMPORTANT: Unlike all other scheduler methods, resume may be called by *ANY* context. Implementations
   *          should not simply assume that the task may be restarted on the current context, and signaling
   *          methods used to indicate that a task should be resumed should be thread-safe. Note that this
   *          thread-safety is only relevant to user code; there aren't any internal mechanisms that require
   *          it. As such, if it can be determined that thread-safety is not necessary (for instance, if it
   *          is known that the current thread is also the only one that may resume the task), then it may
   *          be eschewed.
   */
  void           (* resume)(agt_ctx_t ctx, const agt_suspended_task_t* pSuspendedTask, agt_resume_reason_t reason);

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

/**
 * Once a sched is bound to a ctx, there is no way to unbind it or overwrite it in any capacity.
 *
 * Moreover, there is no way to obtain a reference to the bound sched entity of a given ctx.
 *
 * These are both intentional choices taken as security measures.
 * */

/** ==================[ User Defined Executors ]================== **/




AGT_exec_api agt_status_t AGT_stdcall agt_new_sched(agt_ctx_t ctx, agt_sched_t* pSched, const agt_sched_desc_t* pSchedDesc) AGT_noexcept;

/**
 * Closes an open handle to a sched object.
 *
 * Note: Calling agt_bind_sched does take ownership from the passed handle. As such, it
 * is only necessary to call agt_close_sched on handles that have been duplicated.
 * */
AGT_exec_api void         AGT_stdcall agt_close_sched(agt_ctx_t ctx, agt_sched_t sched) AGT_noexcept;

/**
 *
 * Note: Once explicitly bound, a sched may not be unbound, and no other sched may be bound to
 *       the same ctx.
 *
 * @returns One of \n
 *              - AGT_ERROR_ALREADY_BOUND: A sched object has already been bound to this context.
 * */
AGT_exec_api agt_status_t AGT_stdcall agt_bind_sched(agt_ctx_t ctx, agt_sched_t sched) AGT_noexcept;


/**
 * Wakes a task after a timeout has finished.
 *
 * @param [in] waker The waker for the task in question. Must have been obtained from
 *                   the value of pSuspendInfo->waker passed to the scheduler's
 *                   suspend_until method. Ownership is released upon calling this
 *                   function.
 *
 * AGT_SCHED_ORDERED_WAKING is an optional optimization flag.
 * A scheduler that specifies AGT_SCHED_ORDERED_WAKING *must* guarantee that
 * agt_wake_task is never called on a task that has already been resumed.
 * Specifying this flag does not otherwise change library behaviour in any
 * way, but does provide some internal optimizations over schedulers that do
 * not specify AGT_SCHED_ORDERED_WAKING.
 *
 * If AGT_SCHED_ORDERED_WAKING is specified, the "waker" for that particular suspension
 * is rendered completely invalid once the task has been resumed. It does not need to
 * be freed or closed in any manner (and moreover, it must not be).
 *
 * An example that may specify AGT_SCHED_ORDERED_WAKING:
 *      A single threaded, fiber based scheduler that implements timed suspensions
 *      with a lock-free timeout queue that is checked periodically. Each element
 *      in the queue has a state field that indicates whether or not it is ready to
 *      run. When checked, each element in the queue whose deadline has passed is removed,
 *      and each removed element is put in a ready queue, and has agt_wake_task called on
 *      them with a reason of AGT_RESUME_REASON_TIMED_OUT.
 *      The scheduler's resume method - so long as the resume reason ISN'T
 *      AGT_RESUME_REASON_TIMED_OUT - removes the task from the timeout queue and places
 *      it in the ready queue. Because of this, we know that anything removed from the
 *      timeout queue for its deadline having passed is guaranteed to have not yet resumed.
 *      We thus know that when we call agt_wake_task on them, they have not yet otherwise
 *      been woken up.
 *      NOTE: This example would also allow the AGT_SCHED_SKIP_RESUME_ON_TIMEOUT flag,
 *            as when agt_wake_task is called, resume would do nothing. As such, the
 *            call can be avoided altogether.
 *
 * An example that may NOT specify AGT_SCHED_ORDERED_WAKING:
 *      A scheduler that implements timed suspensions by creating callbacks that are
 *      invoked asynchronously after a system timer elapses. When the callback is invoked,
 *      there's no way to know offhand whether or not the task in question was already
 *      resumed. Calling agt_wake_task in this case is not an error, and will simply return
 *      AGT_FALSE.
 *
 * */
AGT_exec_api agt_bool_t   AGT_stdcall agt_wake_task(agt_ctx_t ctx, agt_task_waker_t waker, agt_resume_reason_t reason) AGT_noexcept;



/*

*/
/**
 *
 * @param size Size of memory to allocate
 *
 * @param align If 0, the default alignment will be used.
 *
 *
 * *//*

AGT_exec_api agt_task_t AGT_stdcall agt_alloc_task(agt_ctx_t ctx, size_t size, size_t align) AGT_noexcept;

*/
/**
 * @param task Task to be recycled. The returned task will have the same size and alignment as the original.
 * *//*

AGT_exec_api agt_task_t AGT_stdcall agt_recycle_task(agt_task_t task) AGT_noexcept;

*/
/**
 * @param task Task to be recycled. The returned task will have the same size and alignment as the original.
 * *//*

AGT_exec_api void       AGT_stdcall agt_free_task(agt_task_t task) AGT_noexcept;

*/


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
