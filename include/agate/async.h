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
  // AGT_ASYNC_UNINITIALIZED  = 0x2,
  AGT_ASYNC_MAY_REPLACE    = 0x4,
  // AGT_ASYNC_INIT_LAZY      = 0x80,   ///< Actual initialization does some minor allocation and such; If INIT_LAZY is specified, this work is postponed until first use; this is ideal if async may not even see use.
  AGT_ASYNC_IS_VALID       = 0x100,
  AGT_ASYNC_CACHE_STATUS   = 0x200
} agt_async_flag_bits_t;
typedef agt_flags32_t agt_async_flags_t;


typedef void (* AGT_stdcall agt_async_callback_t)(agt_ctx_t ctx, agt_status_t result, agt_u64_t returnValue, void* pUserData);

/*typedef struct agt_async_set_t {
  agt_async_t* handles[AGT_ASYNC_SET_MAX_SIZE];
  agt_u64_t    results[AGT_ASYNC_SET_MAX_SIZE];
  agt_status_t statuses[AGT_ASYNC_SET_MAX_SIZE];
  agt_u32_t    boundBitMask;
  agt_u32_t    readyBitMask;
} agt_async_set_t;*/


// Once initialized, these public fields should be left untouched.
typedef struct AGT_alignas(AGT_ASYNC_STRUCT_ALIGNMENT) agt_inline_async_t {
  agt_ctx_t         ctx;
  agt_u32_t         structSize;
  agt_u8_t          reserved[AGT_ASYNC_STRUCT_SIZE - sizeof(agt_ctx_t) - sizeof(agt_u32_t)];
} agt_inline_async_t;

#define AGT_INIT_INLINE_ASYNC(ctx) { ctx, (agt_u32_t)(sizeof(agt_inline_async_t)), { } }



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
AGT_async_api agt_async_t  AGT_stdcall agt_new_async(agt_ctx_t ctx, agt_async_flags_t flags) AGT_noexcept;

/**
 * */
AGT_async_api agt_async_t  AGT_stdcall agt_init_async(agt_inline_async_t* pInlineAsync, agt_async_flags_t flags) AGT_noexcept;

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
AGT_async_api void         AGT_stdcall agt_copy_async(agt_async_t from, agt_async_t to) AGT_noexcept;

/**
 * Optimized equivalent to
 *  \code
 *  agt_copy_async(from, to);
 *  agt_destroy_async(from);
 *  \endcode
 *
 *  \note Intended to eases API mapping to modern C++, where move constructors are important and common.
 * */
AGT_async_api void         AGT_stdcall agt_move_async(agt_async_t from, agt_async_t to) AGT_noexcept;

AGT_async_api void         AGT_stdcall agt_clear_async(agt_async_t async) AGT_noexcept;

AGT_async_api void         AGT_stdcall agt_destroy_async(agt_async_t async) AGT_noexcept;

/**
 * Optimized equivalent to
 *  \code
 *  agt_wait(async, AGT_DO_NOT_WAIT);
 *  \endcode
 * */
AGT_async_api agt_status_t AGT_stdcall agt_async_status(agt_async_t async, agt_u64_t* pResult) AGT_noexcept;
// AGT_async_api agt_status_t AGT_stdcall agt_async_status_all(const agt_async_t* pAsyncs, agt_size_t asyncCount, agt_status_t* pStatuses, agt_u64_t* pResults) AGT_noexcept;

AGT_async_api agt_status_t AGT_stdcall agt_wait(agt_async_t async, agt_u64_t* pResult, agt_timeout_t timeout) AGT_noexcept;
AGT_async_api agt_status_t AGT_stdcall agt_wait_all(const agt_async_t* pAsyncs, agt_size_t asyncCount, agt_timeout_t timeout) AGT_noexcept;
AGT_async_api agt_status_t AGT_stdcall agt_wait_any(const agt_async_t* pAsyncs, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) AGT_noexcept;




/* =================== [ Async Set ] ==================== */

typedef struct agt_aset_st* agt_aset_t;

/*agt_status_t AGT_stdcall agt_new_aset(agt_ctx_t ctx, agt_aset_t* pAsyncSet) AGT_noexcept;
agt_status_t AGT_stdcall agt_aset_add(agt_aset_t aset, agt_async_t* const * pAsyncs, agt_size_t asyncCount) AGT_noexcept;
void         AGT_stdcall agt_aset_remove(agt_aset_t aset, agt_async_t* const * pAsyncs, agt_size_t asyncCount) AGT_noexcept;

agt_status_t AGT_stdcall agt_aset_poll(agt_aset_t aset, ) AGT_noexcept;*/



AGT_end_c_namespace

#endif//AGATE_ASYNC_H
