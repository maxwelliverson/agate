//
// Created by maxwe on 2023-01-25.
//

#ifndef AGATE_SIGNAL_H
#define AGATE_SIGNAL_H

#include <agate/core.h>



AGT_begin_c_namespace


typedef enum agt_signal_flag_bits_t {
  AGT_SIGNAL_AUTO_RESET = 0x1
} agt_signal_flag_bits_t;
typedef agt_flags32_t agt_signal_flags_t;




/* ========================= [ Signal Functions ] ========================= */

AGT_api agt_status_t AGT_stdcall agt_new_signal(agt_ctx_t ctx, agt_signal_t* pSignal, agt_name_t name, agt_signal_flags_t flags) AGT_noexcept;

AGT_api agt_status_t AGT_stdcall agt_open_signal(agt_ctx_t ctx, agt_signal_t* pSignal, const char* name, size_t nameLength, agt_scope_t scope, agt_signal_flags_t flags) AGT_noexcept;

AGT_api void         AGT_stdcall agt_close_signal(agt_signal_t signal) AGT_noexcept;


AGT_api void         AGT_stdcall agt_attach_signal(agt_signal_t signal, agt_async_t* async) AGT_noexcept;
/**
 * if waitForN is 0,
 * */
AGT_api void         AGT_stdcall agt_attach_many_signals(const agt_signal_t* pSignals, agt_size_t signalCount, agt_async_t* async, size_t waitForN) AGT_noexcept;


AGT_api void         AGT_stdcall agt_raise_signal(agt_signal_t signal) AGT_noexcept;
/**
 * Equivalent to
 *
 * \code
 * for (agt_size_t i = 0; i < signalCount; ++i)
 *   agt_raise_signal(pSignals[i]);
 * \endcode
 *
 * To raise N signals, generally prefer calling agt_raise_many_signals rather than calling
 * agt_raise_signal N times, as each individual API call incurs a small, but not completely
 * insignificant overhead.
 * */
AGT_api void         AGT_stdcall agt_raise_many_signals(const agt_signal_t* pSignals, agt_size_t signalCount) AGT_noexcept;


AGT_api void         AGT_stdcall agt_reset_signal(agt_signal_t signal, agt_u32_t resetValue) AGT_noexcept;

AGT_api void         AGT_stdcall agt_reset_many_signals() AGT_noexcept;


AGT_end_c_namespace

#endif//AGATE_SIGNAL_H