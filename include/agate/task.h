//
// Created by maxwe on 2023-07-23.
//

#ifndef AGATE_TASK_H
#define AGATE_TASK_H

#include <agate/core.h>

AGT_begin_c_namespace

typedef struct agt_task_st* agt_task_t;
typedef struct agt_stream_st* agt_stream_t;



AGT_core_api void AGT_stdcall agt_stream_task();



AGT_core_api void AGT_stdcall agt_bind_task(agt_task_t task);

AGT_core_api void AGT_stdcall agt_exec_task(agt_task_t task, agt_executor_t executor);


AGT_end_c_namespace

#endif //AGATE_TASK_H
