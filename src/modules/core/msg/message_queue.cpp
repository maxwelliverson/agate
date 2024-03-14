//
// Created by maxwe on 2022-05-28.
//

#include "agate/atomic.hpp"

#include "message_queue.hpp"


#include <atomic>


struct agt::basic_message {
  union {
    struct {
      basic_message*   next;
      agt_u32_t        i;
    };
    agt::guarded_atomic<basic_message*> guardedNext;
    agt::atomic_epoch_ptr<basic_message> epochNext;
  };

};







agt_status_t agt::sendLocalSPSCQueue(sender_t sender,  agt_message_t message) noexcept {
  AGT_assert_is_a(sender, local_spsc_queue_sender);
  // AGT_assert( AGT_queue_kind(queue) == local_spsc_queue_kind );
  auto s = static_cast<agt::local_spsc_sender*>(sender);

  std::atomic_thread_fence(std::memory_order_acquire);


  message->next = nullptr;

  basic_message** tail = s->tail;
  s->tail = &message->next;
  std::atomic_thread_fence(std::memory_order_acq_rel);
  atomicStore(*tail, message);

  return AGT_SUCCESS;
}
agt_status_t agt::sendLocalMPSCQueue(sender_t sender,  agt_message_t message) noexcept;
agt_status_t agt::sendLocalSPMCQueue(sender_t sender,  agt_message_t message) noexcept;
agt_status_t agt::sendLocalMPMCQueue(sender_t sender,  agt_message_t message) noexcept;
agt_status_t agt::sendSharedSPSCQueue(sender_t sender, agt_message_t message) noexcept;
agt_status_t agt::sendSharedMPSCQueue(sender_t sender, agt_message_t message) noexcept;
agt_status_t agt::sendSharedSPMCQueue(sender_t sender, agt_message_t message) noexcept;
agt_status_t agt::sendSharedMPMCQueue(sender_t sender, agt_message_t message) noexcept;


agt_status_t agt::sendPrivateQueue(sender_t sender,    agt_message_t message) noexcept {
  AGT_assert_is_a( sender, private_queue_sender );
  auto s = static_cast<agt::private_sender*>(sender);
  // message->next = nullptr;

  if (s->)

  message->next.ptr() = nullptr;

  *q->tail = message;
  q->tail  = &message->next;
  return AGT_SUCCESS;
}

message agt::try_receive(private_receiver* receiver) noexcept {

}
message agt::try_receive(local_mpsc_receiver* receiver) noexcept {

}



agt_status_t        agt::enqueueLocalSPSC(message_queue_t queue, basic_message* message) noexcept {

}
agt::basic_message* agt::dequeueLocalSPSC(message_queue_t queue, agt_timeout_t  timeout) noexcept {
  AGT_assert( AGT_queue_kind(queue) == local_spsc_queue_kind );
  auto q = static_cast<agt::local_spsc_queue*>(queue);

  basic_message* msg = nullptr;

  if (atomicWaitFor(q->head, msg, timeout, nullptr)) {
    atomicStore(q->head, msg->next);
    auto newTail = &msg->next;
    atomicCompareExchange(q->tail, newTail, &q->head);
  }

  return msg;
}

agt_status_t        agt::enqueueLocalMPSC(message_queue_t queue, basic_message* message) noexcept {
  AGT_assert( AGT_queue_kind(queue) == local_mpsc_queue_kind );
  auto q = static_cast<agt::local_mpsc_queue*>(queue);
  message->next = nullptr;

  // basic_message** expectedTail = &q->head;
  // while (!atomicCompareExchange(q->tail, expectedTail, &message->next));
  // atomicStore(*expectedTail, message);

  basic_message** tail = atomicExchange(q->tail, &message->next);
  atomicStore(*tail, message);

  return AGT_SUCCESS;
}
agt::basic_message* agt::dequeueLocalMPSC(message_queue_t queue, agt_timeout_t timeout) noexcept {
  AGT_assert( AGT_queue_kind(queue) == local_mpsc_queue_kind );
  auto q = static_cast<agt::local_mpsc_queue*>(queue);

  basic_message* msg = nullptr;

  if (atomicWaitFor(q->head, msg, timeout, nullptr)) {
    atomicStore(q->head, msg->next);
    auto newTail = &msg->next;
    atomicCompareExchange(q->tail, newTail, &q->head);
  }

  return msg;
}

agt_status_t        agt::enqueueLocalSPMC(message_queue_t queue, basic_message* message) noexcept {
  AGT_assert( AGT_queue_kind(queue) == local_spmc_queue_kind );
  auto q = static_cast<agt::local_spmc_queue*>(queue);
  message->next = nullptr;

  basic_message** tail = atomicExchange(q->tail, &message->next);
  // std::atomic_thread_fence(std::memory_order_acq_rel);
  atomicStore(*tail, message);

  return AGT_SUCCESS;
}
agt::basic_message* agt::dequeueLocalSPMC(message_queue_t queue, agt_timeout_t timeout) noexcept {
  AGT_assert( AGT_queue_kind(queue) == local_spmc_queue_kind );
  auto q = static_cast<agt::local_spmc_queue*>(queue);

  basic_message* msg = nullptr;


  switch (timeout) {
    default: {
      auto dl   = deadline::fromTimeout(timeout);
      agt_u32_t backoff = 0;
      do {

        DUFFS_MACHINE_EX(backoff,
                         );
      } while(dl.hasNotPassed());
    }
    case AGT_WAIT:
      atomicWait(q->head, msg, nullptr);
      break;
    case AGT_DO_NOT_WAIT:
      if (!(msg = atomicLoad(q->head)))
        return nullptr;
  }

  do {
    if (!atomicWaitUntil(q->head, msg, timeout, ))
  } while(true);

  if (atomicWaitFor(q->head, msg, timeout, nullptr)) {
    atomicStore(q->head, msg->next);
    auto newTail = &msg->next;
    atomicCompareExchange(q->tail, newTail, &q->head);
  }

  return msg;
}

agt_status_t        agt::enqueueLocalMPMC(message_queue_t queue, basic_message* message) noexcept {
  AGT_assert( AGT_queue_kind(queue) == local_mpmc_queue_kind );
  auto q = static_cast<agt::local_mpmc_queue*>(queue);
  // message->epochNext.store({ nullptr, 1 });
  epoch_ptr<atomic_epoch_ptr<basic_message>> oldTail, newTail;

  // message does not need to be thread safe until it is successfully enqueued, however we DO need to ensure
  // that these writes are visible to other threads as soon as the
  message->next = nullptr;

  // std::atomic_thread_fence(std::memory_order_release);

  oldTail = q->tail.relaxedLoad();
  newTail.set(&message->epochNext);

  // Message at end of queue has a

  do {
    agt_u32_t newEpoch = oldTail.epoch() + 1;
    newTail.setEpoch(newEpoch);
    message->i = newEpoch;                                // Need not be atomic, given that at this point, there's no possible thread contention on message until the subsequent CAS operation succeeds
  } while(!q->tail.compareAndSwapWeak(oldTail, newTail)); // which by virtue of its rel/acq semantics, ensures the non-atomic write to message->i will be visible to other threads beyond this point

  oldTail->store({ message, oldTail.epoch() });
  // oldTail->wakeOne();

  return AGT_SUCCESS;
}
agt::basic_message* agt::dequeueLocalMPMC(message_queue_t queue, agt_timeout_t  timeout) noexcept {
  AGT_assert( AGT_queue_kind(queue) == local_mpmc_queue_kind );
  auto q = static_cast<agt::local_mpmc_queue*>(queue);

  deadline dl = deadline::fromTimeout(timeout);
  epoch_ptr<basic_message> headMsg;
  epoch_ptr<basic_message> nextMsg;

  headMsg = q->head.relaxedLoad();

  do {

    if (headMsg.get() == nullptr) {
      // If, at the moment of reading, the list is empty, either wait for an element to be
      // enqueued or return null depending on the value of timeout.
      switch(timeout) {
        default:
          do {
            if (!q->head.waitAndLoadUntil(headMsg, dl))
              return nullptr;
          } while(headMsg.get() == nullptr);
          break;
        case AGT_WAIT:
          do {
            q->head.waitAndLoad(headMsg);
          } while(headMsg.get() == nullptr);
          break;
        case AGT_DO_NOT_WAIT:
          return nullptr;
      }

      // At this point, headMsg is guaranteed to be non-null.
    }

    nextMsg = headMsg->epochNext.load();

    if (nextMsg.get() == nullptr) {

      // This code path is taken if; at the moment the queue head was
      // read, the queue contained a single element. We must try to
      // update the queue tail to point to the queue head pointer,
      // while also accounting for the possibility that elements may
      // have been appended to queue's tail, but not yet linked to
      // the read element, and also that multiple consumers might have
      // read the same element.

      epoch_ptr<atomic_epoch_ptr<basic_message>> oldTail, newTail;

      oldTail = { &headMsg->epochNext, headMsg.epoch() };
      newTail = { &q->head, headMsg.epoch() + 1 };

      if (!q->tail.compareAndSwap(oldTail, newTail)) {

        // This code path can be reached in one of two ways:
        // 1) More elements are in the process of being enqueued,
        //    and the list head isn't actually the tail. While
        //    this scenario would be better resolved by busy waiting
        //    for the next pointer to be updated, it is indistinguishable
        //    from the following scenario, which isn't.
        // 2) Multiple consumers read the head element at the same time,
        //    and concurrently tried to reset the tail pointer. Some other
        //    consumer succeeded, while this one didn't. In this case, let
        //    the other consumer taken ownership of this element, and
        //    try again from the start. This scenario cannot be resolved by
        //    busy waiting, because there's no guarantee any new messages
        //    will be enqueued at all, let alone any time soon. As such,
        //    we should try reading from head again and if the queue truly
        //    is empty, we may then act accordingly based on timeout.


        // PAUSE_x1(); Maybe it's worth pausing for a bit?
        headMsg = q->head.load();
        continue;
      }

      // By this point, we know we are the only consumer holding
      // this element, and that no producer may append new queue
      // elements to this message. As such, the only case where
      // the following operation fails is if producers have
      // enqueued elements subsequent to the tail reset, in which
      // case it's not needed anyways, so it doesn't matter whether
      // or not this operation succeeds. Either way, we have
      // successfully taken ownership of this message.
      q->head.compareAndSwap(headMsg, nextMsg);
      break;
    }

    if (q->head.compareAndSwap(headMsg, nextMsg))
      break;
  } while(true);

  return headMsg.get();
}

agt_status_t        agt::enqueuePrivate(message_queue_t queue, basic_message* message) noexcept {

}
agt::basic_message* agt::dequeuePrivate(message_queue_t queue, agt_timeout_t timeout) noexcept {
  (void)timeout; // The queue is thread local, and as such, timeout is redundant; the only thread that
                 // could enqueue a new message would be the thread currently waiting for one. In such
                 // cases, return immediately if queue is empty, no matter the timeout.

  // Because of this however, the public dequeue API CANNOT guarantee a message will be returned
  // when timeout is infinite and the queue kind is unknown.

  AGT_assert( AGT_queue_kind(queue) == private_queue_kind );
  auto q = static_cast<agt::private_queue*>(queue);
  basic_message* msg;
  if ((msg = q->head)) {
    q->head = msg->next;
    if (q->tail == &msg->next)
      q->tail = &q->head;
  }
  return msg;
}