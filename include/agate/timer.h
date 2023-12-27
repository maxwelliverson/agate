//
// Created by maxwe on 2023-01-30.
//

#ifndef AGATE_TIMER_H
#define AGATE_TIMER_H

#include <agate/core.h>

AGT_begin_c_namespace


/**
 * I want timers to be able to be set up to be able to trigger arbitrary events
 * in a modular fashion.
 *
 * Ideally, I would like the following properties to be true:
 *
 *   - Can wait (suspend the current execution context) on a timer for until the next timer event (most important)
 *   - Can attach a message (static data), to be sent to attached receivers upon each timer event (important)
 *   - Can attach a callback to be run upon each timer event, possibly then sending out dynamically generated messages (important)
 *
 * Potential design for having a timer that emits periodic (or even simply a time delayed message) is having a derived, distinct object
 * that takes a timer/timer-desc at creation time.
 * */

typedef struct agt_timer_st* agt_timer_t;

typedef enum agt_timer_flag_bits_t {
  AGT_TIMER_EXACT_TIMING = 0x10, // Managed by the kernel, the timer going off triggers a signal handler that may interupt agent callbacks and message handling routines.
  AGT_TIMER_ONE_OFF      = 0x20, ///< Timer goes off once, and then is done
  // AGT_TIMER_
} agt_timer_flag_bits_t;
typedef agt_flags32_t agt_timer_flags_t;

typedef struct agt_timer_desc_t {
  agt_timer_flags_t flags;
  agt_timeout_t     firstTimeout;
  agt_duration_t    period;
} agt_timer_desc_t;



AGT_core_api agt_status_t AGT_stdcall agt_new_timer(agt_ctx_t ctx, agt_timer_t* pTimer, agt_name_t name, const agt_timer_desc_t* pTimerDesc) AGT_noexcept;





AGT_end_c_namespace

#endif//AGATE_TIMER_H
