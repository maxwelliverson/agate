//
// Created by maxwe on 2022-07-13.
//

#ifndef AGATE_ASYNC_H
#define AGATE_ASYNC_H

#include <agate/core.h>

AGT_begin_c_namespace


/* =================[ Types ]================= */

typedef struct agt_async_t {
  agt_u8_t reserved[AGT_ASYNC_STRUCT_SIZE];
} agt_async_t;

typedef struct agt_signal_t {
  agt_u8_t reserved[AGT_SIGNAL_STRUCT_SIZE];
} agt_signal_t;




/* ========================= [ Async Functions ] ========================= */

/**
 * Initializes a new async object that can store/track the state of asynchronous operations.
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
 *  \note Eases API mapping to modern C++, where move constructors are important and common.
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
AGT_api agt_status_t AGT_stdcall agt_async_status(agt_async_t* async) AGT_noexcept;

AGT_api agt_status_t AGT_stdcall agt_wait(agt_async_t* async, agt_timeout_t timeout) AGT_noexcept;
AGT_api agt_status_t AGT_stdcall agt_wait_all(agt_async_t* const * pAsyncs, agt_size_t asyncCount, agt_timeout_t timeout) AGT_noexcept;
AGT_api agt_status_t AGT_stdcall agt_wait_any(agt_async_t* const * pAsyncs, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) AGT_noexcept;




/* ========================= [ Signal Functions ] ========================= */

AGT_api agt_status_t AGT_stdcall agt_new_signal(agt_ctx_t ctx, agt_signal_t* pSignal) AGT_noexcept;
AGT_api void         AGT_stdcall agt_attach_signal(agt_signal_t* signal, agt_async_t* async) AGT_noexcept;
/**
 * if waitForN is 0,
 * */
AGT_api void         AGT_stdcall agt_attach_many_signals(agt_signal_t* const * pSignals, agt_size_t signalCount, agt_async_t* async, size_t waitForN) AGT_noexcept;
AGT_api void         AGT_stdcall agt_raise_signal(agt_signal_t* signal) AGT_noexcept;

/**
 * Equivalent to
 *
 * \code
 * for (agt_size_t i = 0; i < signalCount; ++i)
 *   agt_raise_signal(pSignals[i]);
 * \endcode
 * */
AGT_api void         AGT_stdcall agt_raise_many_signals(agt_signal_t* const * pSignals, agt_size_t signalCount) AGT_noexcept;
AGT_api void         AGT_stdcall agt_destroy_signal(agt_signal_t* signal) AGT_noexcept;




AGT_end_c_namespace

#endif//AGATE_ASYNC_H
