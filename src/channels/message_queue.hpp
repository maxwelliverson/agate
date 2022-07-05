//
// Created by maxwe on 2022-05-27.
//

#ifndef AGATE_MESSAGE_QUEUE_HPP
#define AGATE_MESSAGE_QUEUE_HPP

#include "fwd.hpp"

namespace agt {

  struct private_queue {
    size_t        queuedMessageCount;
    agt_message_t queueHead;
    agt_message_t queueTail;
  };

  struct local_mpsc_queue {
    size_t        queuedMessageCount;
    agt_message_t queueHead;
    agt_message_t queueTail;
  };

  struct local_mpmc_queue {

  };

  struct local_spmc_queue {

  };

  struct local_spsc_queue {

  };

}

#endif//AGATE_MESSAGE_QUEUE_HPP
