//
// Created by maxwe on 2022-05-27.
//

#ifndef AGATE_MESSAGE_QUEUE_HPP
#define AGATE_MESSAGE_QUEUE_HPP

#include "fwd.hpp"

namespace agt {


  void          enqueueMessage(message_queue_t queue, agt_message_t message) noexcept;
  agt_message_t dequeueMessage(message_queue_t queue, agt_timeout_t timeout) noexcept;


  enum class message_queue_kind {
    priv,
    local_spsc,
    local_mpsc,
    local_spmc,
    local_mpmc
  };

  struct message_queue_header {
    message_queue_kind kind;
  };

  struct private_queue : message_queue_header {
    size_t        queuedMessageCount;
    agt_message_t queueHead;
    agt_message_t queueTail;
  };

  struct local_mpsc_queue : message_queue_header {
    size_t        queuedMessageCount;
    agt_message_t queueHead;
    agt_message_t queueTail;
  };

  struct local_mpmc_queue : message_queue_header {

  };

  struct local_spmc_queue : message_queue_header {

  };

  struct local_spsc_queue : message_queue_header {

  };


  void          enqueueMessage(private_queue* queue, agt_message_t message) noexcept;
  agt_message_t dequeueMessage(private_queue* queue, agt_timeout_t timeout) noexcept;

  void          enqueueMessage(local_mpsc_queue* queue, agt_message_t message) noexcept;
  agt_message_t dequeueMessage(local_mpsc_queue* queue, agt_timeout_t timeout) noexcept;

  void          enqueueMessage(local_spsc_queue* queue, agt_message_t message) noexcept;
  agt_message_t dequeueMessage(local_spsc_queue* queue, agt_timeout_t timeout) noexcept;

  void          enqueueMessage(local_mpmc_queue* queue, agt_message_t message) noexcept;
  agt_message_t dequeueMessage(local_mpmc_queue* queue, agt_timeout_t timeout) noexcept;

  void          enqueueMessage(local_spmc_queue* queue, agt_message_t message) noexcept;
  agt_message_t dequeueMessage(local_spmc_queue* queue, agt_timeout_t timeout) noexcept;

}

#endif//AGATE_MESSAGE_QUEUE_HPP
