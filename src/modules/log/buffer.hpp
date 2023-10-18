//
// Created by maxwe on 2023-01-16.
//

#ifndef AGATE_MODULE_LOG_BUFFER_HPP
#define AGATE_MODULE_LOG_BUFFER_HPP

#include "config.hpp"

#include "agate/time.hpp"

namespace agt {


  // Actually dunno if it makes sense to implement logging here or not....

  enum log_buffer_flags {
    LOG_BUFFER_ALLOCATED_ON_REQUEST = 0x1,
  };

  struct log_buffer;
  using log_buffer_t = log_buffer*;

  struct AGT_cache_aligned log_buffer {
    log_buffer_t      nextBuffer;
    agt_flags32_t     flags;
    agt_u32_t         refCount;
    size_t            bufferSize;
    agt_agent_t       sourceAgent;
    agt_timestamp_t   allocationTimestamp;
    agt_timestamp_t   lastMessageTimestamp;
    agt_log_handler_t logHandler;
    void*             logHandlerUserData;
    std::byte         data[];
  };

  static_assert(sizeof(log_buffer) == AGT_CACHE_LINE);

  inline static log_buffer_t make_new_log_buffer(agt_agent_t agent, size_t bufferSize, agt_log_handler_t logHandler, void* handlerUserData) noexcept {
    auto mem = std::malloc(bufferSize);
    auto buf = new (mem) log_buffer;
    buf->nextBuffer  = nullptr;
    buf->flags       = LOG_BUFFER_ALLOCATED_ON_REQUEST;
    buf->refCount    = 1;
    buf->bufferSize  = bufferSize;
    buf->sourceAgent = agent;
    buf->allocationTimestamp = get_fast_timestamp();
    buf->lastMessageTimestamp = 0;
    buf->logHandler = logHandler;
    buf->logHandlerUserData = handlerUserData;
    return buf;
  }






}

#endif//AGATE_MODULE_LOG_BUFFER_HPP
