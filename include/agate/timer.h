//
// Created by maxwe on 2023-01-30.
//

#ifndef AGATE_TIMER_H
#define AGATE_TIMER_H

#include <agate/core.h>

AGT_begin_c_namespace

typedef struct agt_timer_st* agt_timer_t;

typedef enum agt_timer_flag_bits_t {
  AGT_TIMER_EXACT_TIMING = 0x10, // Managed by the kernel, the timer going off triggers a signal handler that may interupt agent callbacks and message handling routines.
  // AGT_TIMER_
} agt_timer_flag_bits_t;
typedef agt_flags32_t agt_timer_flags_t;

typedef struct agt_timer_desc_t {
  agt_timer_flags_t flags;
  agt_timeout_t     firstTimeout;
  agt_duration_t    period;
} agt_timer_desc_t;



AGT_api agt_status_t AGT_stdcall agt_new_timer(agt_ctx_t ctx, agt_timer_t* pTimer, agt_name_t name, const agt_timer_desc_t* pTimerDesc) AGT_noexcept;





AGT_end_c_namespace

#endif//AGATE_TIMER_H
