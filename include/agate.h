//
// Created by maxwe on 2022-06-03.
//

#ifndef AGATE_AGATE_H
#define AGATE_AGATE_H

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4229)
#endif

#include <agate/core.h>

#include <agate/init.h>

#include <agate/agents.h>
#include <agate/async.h>
#include <agate/channels.h>
#include <agate/executor.h>
#include <agate/fiber.h>
#include <agate/hazptr.h>
#include <agate/log.h>
#include <agate/naming.h>
#include <agate/network.h>
#include <agate/pool.h>
#include <agate/shmem.h>
#include <agate/signal.h>
#include <agate/task.h>
#include <agate/thread.h>
#include <agate/timer.h>
#include <agate/uexec.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif//AGATE_AGATE_H
