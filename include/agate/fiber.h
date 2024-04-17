//
// Created by maxwe on 2023-08-09.
//

#ifndef AGATE_FIBER_H
#define AGATE_FIBER_H

#include <agate/core.h>


AGT_begin_c_namespace


#define AGT_CURRENT_FIBER ((agt_fiber_t)-1)
#define AGT_NO_PARENT_FIBER ((agt_fiber_t)-2)



/**
 *
 * Agate fibers operate on the following abstract model:
 *
 *      Each fiber maintains:
 *          - A local stack
 *          - An fctx stack
 *          - A user specified data
 *
 *      An fctx is roughly comparable to a linux ucontext.
 *      It's an opaque object that stores sufficient information to
 *      save execution state at a particular point in code. The fctx
 *      stack is a stack of active fctx objects; when a fiber is
 *      jumped to, execution resumes from the fctx object on top of
 *      that fiber's fctx stack.
 *
 * The elementary fiber operations upon which others are built are:
 *
 * fctx_active() -> (thisFiber):
 *     - Returns a handle to the active fiber.
 *
 * fctx_data(fiber) -> (userData):
 *     - Returns the user data bound to fiber.
 *
 * fctx_push(flags) -> (from, fromParam):
 *     - Creates a new fctx at the current position, and then pushes it to the active fiber's
 *       fctx stack. Then returns (null, null). If the fctx created by this operation is
 *       jumped to, execution returns from this operation again. In this case, the from value
 *       returned is the handle to the fiber that was jumped from, and the fromParam value is
 *       the param argument to the fctx_jump operation that was jumped from.
 *       The flags parameter is used to determine which state components are saved (and therefore
 *       which are subsequently restored).
 *
 * fctx_pop() -> ():
 *     - Pops the fctx from the top of the active fiber's fctx stack. Returns nothing.
 *
 * fctx_jump(to, param) -> ():
 *     - Restores the fctx at the top of to's fctx stack with param.
 *
 *
 * Pseudocode for the fiber API functions in terms of the above described elementary operations:
 *
 * ////
 * agt_fiber_switch(to, param, flags) -> (from, fromParam):
 *      (from, fromFiber) = fctx_push(flags)
 *      if from is null                       // if this is the initial call, jump contexts
 *          fctx_jump(to, param)
 *      fctx_pop()
 *      return (from, fromFiber)              // otherwise, pop the fctx and return
 *
 * ////
 * agt_fiber_jump(to, param) -> ():
 *      fctx_jump(to, param)
 *
 * ////
 * agt_fiber_loop(proc, param, flags) -> (fromParam):
 *      thisFiber = fctx_active()
 *      (from, fromParam) = fctx_push(flags)
 *      if from is null
 *          (from, fromParam) = (thisFiber, param)
 *      return proc(from, fromParam, fctx_data(thisFiber)) // Note that this will only actually return when proc returns. If proc jumps elsewhere, jumping back will instead call the user supplied function again, ie. reentering the loop.
 *
 * ////
 * agt_fiber_fork(proc, param, flags) -> (from, fromParam):
 *      thisFiber = fctx_active()
 *      (from, fromParam) = fctx_push(flags)
 *      if from is null
 *          fromParam = proc(thisFiber, param, fctx_data(thisFiber))
 *      return (from, fromParam)
 *
 * */


// typedef struct agt_fctx_st*  agt_fctx_t;
typedef struct agt_fiber_st* agt_fiber_t;

typedef agt_u64_t agt_fiber_param_t;



typedef struct agt_fiber_list_t {
  agt_fiber_t head;
  agt_fiber_t tail;
  size_t      size;
} agt_fiber_list_t;


typedef enum agt_fctx_flag_bits_t {
  AGT_FIBER_IS_THREAD_SAFE = 0x2
} agt_fctx_flag_bits_t;
typedef agt_flags32_t agt_fctx_flags_t;

typedef enum agt_fiber_flag_bits_t {
  // AGT_FIBER_SWITCH_DO_NOT_SAVE    = 0x1,
  AGT_FIBER_SAVE_EXTENDED_STATE = 0x1
} agt_fiber_flag_bits_t;
typedef agt_flags32_t agt_fiber_flags_t;


typedef struct agt_fiber_transfer_t {
  agt_fiber_t       source;
  agt_fiber_param_t param;
} agt_fiber_transfer_t;



/**
 * agt_fiber_proc_t is the function signature for user provided fiber routines.
 * \param fromFiber The handle of the fiber context that execution just jumped from,
 *                  or if the it is the initial callback from either agt_fiber_loop
 *                  or agt_fiber_fork, then it is the handle of the current fiber.
 * \param param     A parameter that is specified at the call site that resulted in
 *                  the current execution of this callback. For the initial callback
 *                  of agt_fiber_loop or agt_fiber_fork, this is the param argument
 *                  supplied to said function call. If execution was jumped to by
 *                  agt_fiber_jump or agt_fiber_switch, this is the param argument
 *                  supplied to said function call.
 * \param userData  The active UserData for the current fiber. Initially, this is
 *                  equal to agt_fctx_desc_t::userData for the initial fiber created
 *                  by agt_enter_fctx, or the userData argument of agt_new_fiber
 *                  otherwise. This is the same as the value returned by
 *                  agt_get_fiber_data(AGT_CURRENT_FIBER), and may be changed at any
 *                  point by calling agt_set_fiber_data.
 */
typedef agt_fiber_param_t (* AGT_stdcall agt_fiber_proc_t)(agt_fiber_t fromFiber, agt_fiber_param_t param, void* userData);



typedef struct agt_fctx_desc_t {
  agt_fctx_flags_t flags;          ///<
  agt_u32_t        stackSize;      ///< If zero, the value of AGT_ATTR_DEFAULT_FIBER_STACK_SIZE will be used instead
  agt_u32_t        maxFiberCount;  ///< If zero, an implementation defined value will be used instead.
  agt_ctx_t        parent;         ///< If not null, this should point to the context of the thread that spawned the current thread. Doing so will bind this new fctx to the fctx bound to the given context, such that fibers from a given fctx may execute on any thread bound to the shared fctx.
  agt_fiber_proc_t proc;
  agt_u64_t        initialParam;
  void*            userData;
} agt_fctx_desc_t;





/** ==================[         Fibers         ]================== **/


/**
 *
 * @param [optional, out] pExitCode If not null, the value of the exitCode argument used in the call to agt_exit_fctx that exits this fctx will be written to the specified address.
 * */
AGT_core_api agt_status_t AGT_stdcall agt_enter_fctx(agt_ctx_t ctx, const agt_fctx_desc_t* pFCtxDesc, int* pExitCode) AGT_noexcept;

AGT_core_api void         AGT_stdcall agt_exit_fctx(agt_ctx_t ctx, int exitCode) AGT_noexcept;


/**
 * Creates a new fiber within the current fiber context. The new fiber is created in a
 * suspended state, and will not execute until jumped to with either \ref agt_fiber_jump
 * or \ref agt_fiber_switch
 *
 *
 * This may only be called from within a fiber context; otherwise an error is returned.
 *
 * \note Unlike \ref agt_enter_fctx, there's no initial parameter set to be passed to proc.
 *       This is because the new fiber is created in a suspended state, and does not run until
 *       jumped to, and a jump operation always has a parameter. Therefore, the initial
 *       parameter to proc will be the parameter passed the first time the fiber is jumped to.
 */
AGT_core_api agt_status_t AGT_stdcall agt_new_fiber(agt_fiber_t* pFiber, agt_fiber_proc_t proc, void* userData) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_destroy_fiber(agt_fiber_t fiber) AGT_noexcept;



/**
 * Sets the userData pointer that is passed to any instance of agt_fiber_proc_t executed by fiber.
 *
 * @returns The previous userData bound to fiber
 */
AGT_core_api void*        AGT_stdcall agt_set_fiber_data(agt_fiber_t fiber, void* userData) AGT_noexcept;

AGT_core_api void*        AGT_stdcall agt_get_fiber_data(agt_fiber_t fiber) AGT_noexcept;

AGT_core_api agt_fiber_t  AGT_stdcall agt_get_current_fiber(agt_ctx_t ctx) AGT_noexcept;



// Save current context, jump to another.
// Returns the param value used to jump back to this context
// If @param fiber is the currently bound fiber, this returns @param param immediately
AGT_core_api agt_fiber_transfer_t AGT_stdcall agt_fiber_switch(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags) AGT_noexcept;

// Jumps to another fiber context with @param param
// Does not save current context
// If fiber is AGT_CURRENT_FIBER, the context will jump back to the previous context save in this fiber.
AGT_core_api AGT_noreturn void    AGT_stdcall agt_fiber_jump(agt_fiber_t fiber, agt_fiber_param_t param) AGT_noexcept;


// Probably can modify agt_fiber_fork to only need a function with the signature void(*)(void*, void*), taking both the fiber's userData, and a pointer supplied in the call.

// Saves current context,
// After saving, calls @param proc with @param param
// Expectation is that proc does not return, and later calls agt_fiber_jump
// Return value is the param argument passed to the call to agt_fiber_jump/agt_fiber_switch used to jump back to this save point
// If @param proc DOES return, the value that is returned by this call is equal to @param param.
/// The expected benefit of this over agt_fiber_switch is that with agt_fiber_fork, the switch may be
/// abandonned early if it is determined that it is not necessary.
AGT_core_api agt_fiber_transfer_t AGT_stdcall agt_fiber_fork(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags) AGT_noexcept;


/// Saves current context,
/// After saving, calls @param proc with @param param
/// Expectation is that proc DOES return
/// If this context is jumped to, it simply calls @param proc again with the passed parameter
/// The difference between agt_fiber_fork and agt_fiber_loop is essentially that,
///   when restored, the fctx saved by agt_fiber_fork returns from agt_fiber_fork, whereas
///   the fctx saved by agt_fiber_loop again calls @param proc
/// Returns the value returned by proc. Unlike agt_fiber_fork, this does not need to be an
///   agt_fiber_transfer_t, because control can only ever return from the current fiber.
AGT_core_api agt_fiber_param_t    AGT_stdcall agt_fiber_loop(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags) AGT_noexcept;




AGT_end_c_namespace

#endif//AGATE_FIBER_H
