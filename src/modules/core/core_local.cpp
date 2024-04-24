//
// Created by maxwe on 2024-03-20.
//

#include "core/object.hpp"

#include "core/agents.hpp"
#include "core/async.hpp"
#include "core/exec.hpp"
#include "core/fiber.hpp"

#include "core/msg/broadcast_queue.hpp"
#include "core/msg/message_pool.hpp"
#include "core/msg/message_queue.hpp"
#include "core/msg/ring_queue.hpp"




namespace agt {

  agt_status_t AGT_stdcall dup_local(agt_object_t srcObject, agt_object_t* pDstObject, size_t dstObjectCount) noexcept {
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }

  void AGT_stdcall close_local(agt_object_t obj_) noexcept {
    const auto obj = static_cast<agt::object*>(obj_);
    switch (obj->type) {
      // case object_type::agent_self: return agt::release(static_cast<agent_self*>(obj_));

    	// case object_type::local_agent: return agt::release(static_cast<local_agent*>(obj_));
    	// case object_type::proxy_agent: return agt::release(static_cast<proxy_agent*>(obj_));
    	// case object_type::shared_agent: return agt::release(static_cast<shared_agent*>(obj_));
    	// case object_type::imported_agent: return agt::release(static_cast<imported_agent*>(obj_));

    	// case object_type::local_busy_executor: return agt::release(static_cast<local_busy_executor*>(obj_));
    	// case object_type::local_event_executor: return agt::release(static_cast<local_event_executor*>(obj_));
    	// case object_type::local_user_executor: return agt::release(static_cast<local_user_executor*>(obj_));
    	// case object_type::local_parallel_executor: return agt::release(static_cast<local_parallel_executor*>(obj_));
    	// case object_type::local_proxy_executor: return agt::release(static_cast<local_proxy_executor*>(obj_));
    	// case object_type::shared_busy_executor: return agt::release(static_cast<shared_busy_executor*>(obj_));
    	// case object_type::shared_single_thread_executor: return agt::release(static_cast<shared_single_thread_executor*>(obj_));
    	// case object_type::shared_pool_executor: return agt::release(static_cast<shared_pool_executor*>(obj_));
    	// case object_type::shared_proxy_executor: return agt::release(static_cast<shared_proxy_executor*>(obj_));

    	case object_type::local_async_data: return agt::release(static_cast<local_async_data*>(obj_));
    	case object_type::shared_async_data: return agt::release(static_cast<shared_async_data*>(obj_));
    	case object_type::imported_async_data: return agt::release(static_cast<imported_async_data*>(obj_));

    	case object_type::private_message_pool: return agt::release(static_cast<private_message_pool*>(obj_));
    	case object_type::private_sized_message_pool: return agt::release(static_cast<private_sized_message_pool*>(obj_));
    	case object_type::local_spsc_message_pool: return agt::release(static_cast<local_spsc_message_pool*>(obj_));
    	case object_type::local_spsc_sized_message_pool: return agt::release(static_cast<local_spsc_sized_message_pool*>(obj_));
    	case object_type::local_mpsc_message_pool: return agt::release(static_cast<local_mpsc_message_pool*>(obj_));
    	case object_type::local_mpsc_sized_message_pool: return agt::release(static_cast<local_mpsc_sized_message_pool*>(obj_));
    	case object_type::local_spmc_message_pool: return agt::release(static_cast<local_spmc_message_pool*>(obj_));
    	case object_type::local_spmc_sized_message_pool: return agt::release(static_cast<local_spmc_sized_message_pool*>(obj_));
    	case object_type::local_mpmc_message_pool: return agt::release(static_cast<local_mpmc_message_pool*>(obj_));
    	case object_type::local_mpmc_sized_message_pool: return agt::release(static_cast<local_mpmc_sized_message_pool*>(obj_));
    	case object_type::shared_spsc_message_pool: return agt::release(static_cast<shared_spsc_message_pool*>(obj_));
    	case object_type::shared_spsc_sized_message_pool: return agt::release(static_cast<shared_spsc_sized_message_pool*>(obj_));
    	case object_type::shared_mpsc_message_pool: return agt::release(static_cast<shared_mpsc_message_pool*>(obj_));
    	case object_type::shared_mpsc_sized_message_pool: return agt::release(static_cast<shared_mpsc_sized_message_pool*>(obj_));
    	case object_type::shared_spmc_message_pool: return agt::release(static_cast<shared_spmc_message_pool*>(obj_));
    	case object_type::shared_spmc_sized_message_pool: return agt::release(static_cast<shared_spmc_sized_message_pool*>(obj_));
    	case object_type::shared_mpmc_message_pool: return agt::release(static_cast<shared_mpmc_message_pool*>(obj_));
    	case object_type::shared_mpmc_sized_message_pool: return agt::release(static_cast<shared_mpmc_sized_message_pool*>(obj_));


    	case object_type::local_spsc_sender: return agt::release(static_cast<local_spsc_sender*>(obj_));
    	case object_type::local_mpsc_sender: return agt::release(static_cast<local_mpsc_sender*>(obj_));
    	case object_type::local_spmc_sender: return agt::release(static_cast<local_spmc_sender*>(obj_));
    	case object_type::local_mpmc_sender: return agt::release(static_cast<local_mpmc_sender*>(obj_));
    	case object_type::shared_spsc_sender: return agt::release(static_cast<shared_spsc_sender*>(obj_));
    	case object_type::shared_mpsc_sender: return agt::release(static_cast<shared_mpsc_sender*>(obj_));
    	case object_type::shared_spmc_sender: return agt::release(static_cast<shared_spmc_sender*>(obj_));
    	case object_type::shared_mpmc_sender: return agt::release(static_cast<shared_mpmc_sender*>(obj_));
    	case object_type::private_sender: return agt::release(static_cast<private_sender*>(obj_));
    	case object_type::local_sp_ring_queue_sender: return agt::release(static_cast<local_sp_ring_queue_sender*>(obj_));
    	case object_type::local_mp_ring_queue_sender: return agt::release(static_cast<local_mp_ring_queue_sender*>(obj_));
    	// case object_type::shared_sp_ring_queue_sender: return agt::release(static_cast<shared_sp_ring_queue_sender*>(obj_));
    	// case object_type::shared_mp_ring_queue_sender: return agt::release(static_cast<shared_mp_ring_queue_sender*>(obj_));
    	// case object_type::local_sp_bqueue_sender: return agt::release(static_cast<local_sp_bqueue_sender*>(obj_));
    	// case object_type::local_mp_bqueue_sender: return agt::release(static_cast<local_mp_bqueue_sender*>(obj_));
    	// case object_type::shared_sp_bqueue_sender: return agt::release(static_cast<shared_sp_bqueue_sender*>(obj_));
    	// case object_type::shared_mp_bqueue_sender: return agt::release(static_cast<shared_mp_bqueue_sender*>(obj_));

    	case object_type::local_spsc_receiver: return agt::release(static_cast<local_spsc_receiver*>(obj_));
    	case object_type::local_mpsc_receiver: return agt::release(static_cast<local_mpsc_receiver*>(obj_));
    	case object_type::local_spmc_receiver: return agt::release(static_cast<local_spmc_receiver*>(obj_));
    	case object_type::local_mpmc_receiver: return agt::release(static_cast<local_mpmc_receiver*>(obj_));
    	case object_type::shared_spsc_receiver: return agt::release(static_cast<shared_spsc_receiver*>(obj_));
    	case object_type::shared_mpsc_receiver: return agt::release(static_cast<shared_mpsc_receiver*>(obj_));
    	case object_type::shared_spmc_receiver: return agt::release(static_cast<shared_spmc_receiver*>(obj_));
    	case object_type::shared_mpmc_receiver: return agt::release(static_cast<shared_mpmc_receiver*>(obj_));
    	case object_type::private_receiver: return agt::release(static_cast<private_receiver*>(obj_));
    	case object_type::local_sp_ring_queue_receiver: return agt::release(static_cast<local_sp_ring_queue_receiver*>(obj_));
    	case object_type::local_mp_ring_queue_receiver: return agt::release(static_cast<local_mp_ring_queue_receiver*>(obj_));
    	// case object_type::shared_sp_ring_queue_receiver: return agt::release(static_cast<shared_sp_ring_queue_receiver*>(obj_));
    	// case object_type::shared_mp_ring_queue_receiver: return agt::release(static_cast<shared_mp_ring_queue_receiver*>(obj_));
    	// case object_type::local_sp_bqueue_receiver: return agt::release(static_cast<local_sp_bqueue_receiver*>(obj_));
    	// case object_type::local_mp_bqueue_receiver: return agt::release(static_cast<local_mp_bqueue_receiver*>(obj_));
    	// case object_type::shared_sp_bqueue_receiver: return agt::release(static_cast<shared_sp_bqueue_receiver*>(obj_));
    	// case object_type::shared_mp_bqueue_receiver: return agt::release(static_cast<shared_mp_bqueue_receiver*>(obj_));


    	case object_type::local_mpmc_queue: return agt::release(static_cast<local_mpmc_queue*>(obj_));
    	// case object_type::shared_spsc_queue: return agt::release(static_cast<shared_spsc_queue*>(obj_));
    	// case object_type::shared_mpsc_queue: return agt::release(static_cast<shared_mpsc_queue*>(obj_));
    	// case object_type::shared_spmc_queue: return agt::release(static_cast<shared_spmc_queue*>(obj_));
    	// case object_type::shared_mpmc_queue: return agt::release(static_cast<shared_mpmc_queue*>(obj_));
    	// case object_type::shared_sp_bqueue: return agt::release(static_cast<shared_sp_bqueue*>(obj_));
    	// case object_type::shared_mp_bqueue: return agt::release(static_cast<shared_mp_bqueue*>(obj_));

    	// case object_type::private_object_pool: return agt::release(static_cast<private_object_pool*>(obj_));
    	// case object_type::user_object_pool: return agt::release(static_cast<user_object_pool*>(obj_));
    	// case object_type::locked_object_pool: return agt::release(static_cast<locked_object_pool*>(obj_));
    	// case object_type::shared_object_pool: return agt::release(static_cast<shared_object_pool*>(obj_));

    	case object_type::ctx_sized_pool: return agt::release(static_cast<ctx_sized_pool*>(obj_));
    	case object_type::user_sized_pool: return agt::release(static_cast<user_sized_pool*>(obj_));

    	// case object_type::user_allocation: return agt::release(static_cast<user_allocation*>(obj_));
    	// case object_type::user_rc_allocation: return agt::release(static_cast<user_rc_allocation*>(obj_));


    // 	case object_type::unsafe_fiber: return agt::release(static_cast<unsafe_fiber*>(obj_));
    	case object_type::fiber: return agt::release(static_cast<fiber*>(obj_));
    	case object_type::fctx: return agt::release(static_cast<fctx*>(obj_));
    	case object_type::pooled_fctx: return agt::release(static_cast<pooled_fctx*>(obj_));
    // 	case object_type::safe_fiber: return agt::release(static_cast<safe_fiber*>(obj_));

    	// case object_type::unsafe_fiber_pool: return agt::release(static_cast<unsafe_fiber_pool*>(obj_));
    	// case object_type::safe_fiber_pool: return agt::release(static_cast<safe_fiber_pool*>(obj_));

    	// case object_type::local_event_eagent: return agt::release(static_cast<local_event_eagent*>(obj_));

      [[unlikely]] default: {
        auto ctx = get_ctx();
        raise(get_instance(ctx), AGT_ERROR_INVALID_ARGUMENT, obj_);
      }
    }
  }
}