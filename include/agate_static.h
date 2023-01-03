//
// Created by maxwe on 2022-12-20.
//

#ifndef AGATE_AGATE_STATIC_H
#define AGATE_AGATE_STATIC_H

#define AGT_USE_STATIC_DISPATCH

#if AGT_STATIC_FLAG_SHARED_CONTEXT
#define agt_send _agate_api_send__s
#else
#define agt_send _agate_api_send__p
#endif

#include <agate/core.h>
#include <agate/agents.h>
#include <agate/async.h>
#include <agate/channels.h>

#endif//AGATE_AGATE_STATIC_H
