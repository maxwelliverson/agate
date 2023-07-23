//
// Created by maxwe on 2022-07-13.
//

#ifndef AGATE_ASYNC_H
#define AGATE_ASYNC_H

#include <agate/core.h>

AGT_begin_c_namespace


#define AGT_ASYNC_SET_MAX_SIZE 31


/* =================[ Types ]================= */


typedef enum agt_async_flag_bits_t {
  AGT_ASYNC_EDGE_TRIGGERED = 0x1,    ///< Default is level-triggered.
  // AGT_ASYNC_INIT_LAZY      = 0x80,   ///< Actual initialization does some minor allocation and such; If INIT_LAZY is specified, this work is postponed until first use; this is ideal if async may not even see use.
  AGT_ASYNC_IS_VALID       = 0x100,
} agt_async_flag_bits_t;
typedef agt_flags32_t agt_async_flags_t;


/*typedef struct agt_aset_st* agt_aset_t;*/
/*typedef struct agt_async_set_t {
  agt_async_t* handles[AGT_ASYNC_SET_MAX_SIZE];
  agt_u64_t    results[AGT_ASYNC_SET_MAX_SIZE];
  agt_status_t statuses[AGT_ASYNC_SET_MAX_SIZE];
  agt_u32_t    boundBitMask;
  agt_u32_t    readyBitMask;
} agt_async_set_t;*/


#ifndef AGT_UNDEFINED_ASYNC_STRUCT
typedef struct AGT_alignas(AGT_ASYNC_STRUCT_ALIGNMENT) agt_async_t {
  /// TODO: Decide whether or not to expose these fields for direct access (and very fast initialization).
  agt_ctx_t ctx;
  agt_status_t status;
  agt_async_flags_t flags;
  agt_u8_t reserved[AGT_ASYNC_STRUCT_SIZE - 16];
  // agt_u8_t reserved[AGT_ASYNC_STRUCT_SIZE];
} agt_async_t;
#endif


// typedef agt_status_t(* AGT_stdcall agt_deferred_op_t)(agt_ctx_t ctx, void* userData, );


#define AGT_INIT_ASYNC(ctx) AGT_INIT_ASYNC_FLAGS(ctx, 0)
#define AGT_INIT_ASYNC_FLAGS(ctx, flags) { ctx, AGT_ERROR_NOT_BOUND, flags }




/* ========================= [ Async Functions ] ========================= */

/**
 * Initializes a new async object that can store/track the state of asynchronous operations.
 *
 * \param [in] ctx (optional) If ctx is null, the context bound to the current thread is taken.
 * \param [out] pAsync
 * \param [in] flags May consist of some valid bitwise combination of the following flags:
 *                   AGT_ONE_AND_DONE: indicates that this async object should be automatically
 *                   destroyed after its first successful wait operation.
 * */
AGT_api agt_status_t AGT_stdcall agt_new_async(agt_ctx_t ctx, agt_async_t* pAsync, agt_async_flags_t flags) AGT_noexcept;

/**
 * Sets pTo to track the same asynchronous operation as pFrom.
 * \param pFrom The source object
 * \param pTo   The destination object
 * \pre - pFrom must be initialized. \n
 *      - pTo does not need to have been initialized, but may be. \n
 *      - If pTo is uninitialized, the memory pointed to by pTo must have been zeroed. \n
 *      \code memset(pTo, 0, sizeof(*pTo));\endcode
 * \post - pTo is initialized. \n
 *       - If before the call, pTo had been bound to a different operation, that operation is abandonned as though by calling agt_clear_async. \n
 *       - If pFrom is bound to an operation, pTo is now bound to the same operation. \n
 *       - pFrom is unmodified
 * \note While this function copies state from pFrom to pTo, any further changes made to pFrom after this call returns are not reflected in pTo.
 * */
AGT_api void         AGT_stdcall agt_copy_async(const agt_async_t* pFrom, agt_async_t* pTo) AGT_noexcept;

/**
 * Optimized equivalent to
 *  \code
 *  agt_copy_async(from, to);
 *  agt_destroy_async(from);
 *  \endcode
 *
 *  \note Intended to eases API mapping to modern C++, where move constructors are important and common.
 * */
AGT_api void         AGT_stdcall agt_move_async(agt_async_t* from, agt_async_t* to) AGT_noexcept;

AGT_api void         AGT_stdcall agt_clear_async(agt_async_t* async) AGT_noexcept;

AGT_api void         AGT_stdcall agt_destroy_async(agt_async_t* async) AGT_noexcept;

/**
 * Optimized equivalent to
 *  \code
 *  agt_wait(async, AGT_DO_NOT_WAIT);
 *  \endcode
 * */
AGT_api agt_status_t AGT_stdcall agt_async_status(agt_async_t* async, agt_u64_t* pResult) AGT_noexcept;
AGT_api agt_status_t AGT_stdcall agt_async_status_all(agt_async_t* const * pAsyncs, agt_size_t asyncCount, agt_status_t* pStatuses, agt_u64_t* pResults) AGT_noexcept;

AGT_api agt_status_t AGT_stdcall agt_wait(agt_async_t* async, agt_u64_t* pResult, agt_timeout_t timeout) AGT_noexcept;
AGT_api agt_status_t AGT_stdcall agt_wait_all(agt_async_t* const * pAsyncs, agt_size_t asyncCount, agt_timeout_t timeout) AGT_noexcept;
AGT_api agt_status_t AGT_stdcall agt_wait_any(agt_async_t* const * pAsyncs, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) AGT_noexcept;




/* =================== [ Async Set ] ==================== */

/*AGT_api agt_status_t AGT_stdcall agt_new_aset(agt_ctx_t ctx, agt_aset_t* pAsyncSet) AGT_noexcept;
AGT_api agt_status_t AGT_stdcall agt_aset_add(agt_aset_t aset, agt_async_t* const * pAsyncs, agt_size_t asyncCount) AGT_noexcept;
AGT_api void         AGT_stdcall agt_aset_remove(agt_aset_t aset, agt_async_t* const * pAsyncs, agt_size_t asyncCount) AGT_noexcept;

AGT_api agt_status_t AGT_stdcall agt_aset_poll(agt_aset_t aset, ) AGT_noexcept;*/



AGT_end_c_namespace

#endif//AGATE_ASYNC_H
