//
// Created by maxwe on 2024-04-23.
//

#ifndef AGATE_FUTEX_H
#define AGATE_FUTEX_H

#include <agate/core.h>

AGT_begin_c_namespace

typedef agt_bool_t (* AGT_stdcall agt_futex_predicate_t)(agt_ctx_t ctx, const void* addr, void* userData);


AGT_core_api agt_bool_t AGT_stdcall agt_sysfutex_wait(agt_ctx_t ctx, const void* addr, agt_timeout_t timeout, void* compare, size_t valueSize) AGT_noexcept;

AGT_core_api void       AGT_stdcall agt_sysfutex_wake_one(agt_ctx_t ctx, const void* addr) AGT_noexcept;

AGT_core_api void       AGT_stdcall agt_sysfutex_wake_all(agt_ctx_t ctx, const void* addr) AGT_noexcept;


AGT_core_api agt_bool_t AGT_stdcall agt_futex_wait(agt_ctx_t ctx, const void* addr, agt_timeout_t timeout, void* compare, size_t valueSize) AGT_noexcept;

AGT_core_api agt_bool_t AGT_stdcall agt_futex_wait_callback(agt_ctx_t ctx, const void* addr, agt_timeout_t timeout, agt_futex_predicate_t predicate, void* userData) AGT_noexcept;

AGT_core_api void       AGT_stdcall agt_futex_wake_one(agt_ctx_t ctx, const void* addr) AGT_noexcept;

AGT_core_api void       AGT_stdcall agt_futex_wake_all(agt_ctx_t ctx, const void* addr) AGT_noexcept;


AGT_end_c_namespace

#endif//AGATE_FUTEX_H
