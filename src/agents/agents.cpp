//
// Created by maxwe on 2022-05-27.
//

#include "agents.hpp"
// #include "agents.h"

#include "async/async.hpp"

agt_status_t agt::wait(blocked_queue& queue, agt_async_t* async, agt_timeout_t timeout) noexcept {
  agt_status_t status = asyncStatus(*async);
  if (timeout == AGT_DO_NOT_WAIT || status != AGT_NOT_READY)
    return status;

  queue.blockKind = timeout == AGT_WAIT ? block_kind::eSingleBlock : block_kind::eSingleBlockWithTimeout;

  // if (setjmp(queue.contextStorage))
}

agt_status_t agt::waitAny(blocked_queue& queue, agt_async_t* const * ppAsyncs, size_t asyncCount, agt_timeout_t timeout) noexcept {}

agt_status_t agt::waitMany(blocked_queue& queue, agt_async_t* const * ppAsyncs, size_t asyncCount, size_t waitForCount, size_t& index, agt_timeout_t timeout) noexcept {}