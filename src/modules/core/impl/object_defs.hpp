//
// Created by maxwe on 2024-03-14.
//

#ifndef AGATE_CORE_IMPL_OBJECT_DEFS_HPP
#define AGATE_CORE_IMPL_OBJECT_DEFS_HPP

#include "config.hpp"


namespace agt {

  enum class object_type : agt_u16_t {

    async,

    agent_instance,


    local_agent,
    proxy_agent,
    shared_agent,
    imported_agent,

    uexec,
    local_busy_executor,
    local_event_executor,
    local_user_executor,
    local_parallel_executor,
    local_proxy_executor,
    shared_busy_executor,
    shared_event_executor,
    shared_user_executor,
    shared_parallel_executor,
    shared_proxy_executor,


    local_busy_self,
    local_event_self,
    local_user_self,
    local_parallel_self,
    local_proxy_self,
    shared_busy_self,
    shared_event_self,
    shared_user_self,
    shared_parallel_self,
    shared_proxy_self,

    local_busy_eagent,
    local_event_eagent,
    local_user_eagent,
    local_parallel_eagent,
    local_proxy_eagent,
    shared_busy_eagent,
    shared_event_eagent,
    shared_user_eagent,
    shared_parallel_eagent,
    shared_proxy_eagent,


    local_async_data,
    shared_async_data,
    imported_async_data,



    private_sized_message_pool,
    local_spsc_sized_message_pool,
    local_mpsc_sized_message_pool,
    local_spmc_sized_message_pool,
    local_mpmc_sized_message_pool,
    shared_spsc_sized_message_pool,
    shared_mpsc_sized_message_pool,
    shared_spmc_sized_message_pool,
    shared_mpmc_sized_message_pool,

    private_message_pool,
    local_spsc_message_pool,
    local_mpsc_message_pool,
    local_spmc_message_pool,
    local_mpmc_message_pool,
    shared_spsc_message_pool,
    shared_mpsc_message_pool,
    shared_spmc_message_pool,
    shared_mpmc_message_pool,



    local_spsc_sender,
    local_mpsc_sender,
    local_spmc_sender,
    local_mpmc_sender,
    shared_spsc_sender,
    shared_mpsc_sender,
    shared_spmc_sender,
    shared_mpmc_sender,
    private_sender,

    local_sp_ring_queue_sender,
    local_mp_ring_queue_sender,
    shared_sp_ring_queue_sender,
    shared_mp_ring_queue_sender,
    local_sp_bqueue_sender,
    local_mp_bqueue_sender,
    shared_sp_bqueue_sender,
    shared_mp_bqueue_sender,

    local_spsc_receiver,
    local_mpsc_receiver,
    local_spmc_receiver,
    local_mpmc_receiver,
    shared_spsc_receiver,
    shared_mpsc_receiver,
    shared_spmc_receiver,
    shared_mpmc_receiver,
    private_receiver,

    local_sp_ring_queue_receiver,
    local_mp_ring_queue_receiver,
    shared_sp_ring_queue_receiver,
    shared_mp_ring_queue_receiver,
    local_sp_bqueue_receiver,
    local_mp_bqueue_receiver,
    shared_sp_bqueue_receiver,
    shared_mp_bqueue_receiver,


    local_mpmc_queue,
    shared_spsc_queue,
    shared_mpsc_queue,
    shared_spmc_queue,
    shared_mpmc_queue,
    shared_sp_bqueue,
    shared_mp_bqueue,

    private_object_pool,
    user_object_pool,
    locked_object_pool,
    shared_object_pool,

    ctx_sized_pool,
    user_sized_pool,

    user_allocation,
    user_rc_allocation,


    hazptr,
    hazptr_domain,


    // unsafe_fiber,
    fiber,
    fctx,
    pooled_fctx,
    // safe_fiber,

    unsafe_fiber_pool,
    safe_fiber_pool,


    blocked_queue_vec,
    task_queue_vec,
    local_channel_async_info,

    monitored_address,

    default_uexec_timeout_data,
    local_busy_executor_timeout_data,


    default_uexec_task,


    agent_begin    = local_agent,
    agent_end      = imported_agent,

    executor_begin = local_busy_executor,
    executor_end   = shared_proxy_executor,

    uexec_begin = uexec,
    uexec_end   = executor_end,

    agent_self_begin = local_busy_self,
    agent_self_end   = shared_proxy_self,

    eagent_begin   = local_busy_eagent,
    eagent_end     = shared_proxy_eagent,

    sender_begin   = local_spsc_sender,
    sender_end     = shared_mp_bqueue_sender,

    basic_sender_end = private_sender,

    receiver_begin = local_spsc_receiver,
    receiver_end   = shared_mp_bqueue_receiver,

    basic_receiver_end = private_receiver,

    object_pool_begin = private_object_pool,
    object_pool_end   = shared_object_pool,

    sized_msg_pool_begin = private_sized_message_pool,
    sized_msg_pool_end   = shared_mpmc_sized_message_pool,

    unsized_msg_pool_begin = private_message_pool,
    unsized_msg_pool_end   = shared_mpmc_message_pool,

    msg_pool_begin = sized_msg_pool_begin,
    msg_pool_end   = unsized_msg_pool_end,

    async_data_begin = local_async_data,
    async_data_end   = shared_async_data,

    ring_queue_sender_begin = local_sp_ring_queue_sender,
    ring_queue_sender_end   = shared_mp_ring_queue_sender,
    local_ring_queue_sender_begin = local_sp_ring_queue_sender,
    local_ring_queue_sender_end   = local_mp_ring_queue_sender,
    shared_ring_queue_sender_begin = shared_sp_ring_queue_sender,
    shared_ring_queue_sender_end = shared_mp_ring_queue_sender,

    ring_queue_receiver_begin = local_sp_ring_queue_receiver,
    ring_queue_receiver_end   = shared_mp_ring_queue_receiver,
    local_ring_queue_receiver_begin = local_sp_ring_queue_receiver,
    local_ring_queue_receiver_end   = local_mp_ring_queue_receiver,
    shared_ring_queue_receiver_begin = shared_sp_ring_queue_receiver,
    shared_ring_queue_receiver_end = shared_mp_ring_queue_receiver,

    // fiber_begin = unsafe_fiber,
    // fiber_end   = safe_fiber,

    fiber_pool_begin = unsafe_fiber_pool,
    fiber_pool_end   = safe_fiber_pool,

    sized_pool_begin = ctx_sized_pool,
    sized_pool_end   = user_sized_pool,
  };

  struct object {
    object_type type;            // 2 bytes
    agt_u16_t   poolChunkOffset; // DO NOT TOUCH, internal data. Units of 32 bytes
  };


  static_assert(sizeof(object) == sizeof(agt_u32_t));


  struct rc_object : object {
    agt_u32_t flags;
    agt_u32_t refCount;
    agt_u32_t epoch;
  };

  static_assert(sizeof(rc_object) == 16);
}

#define AGT_virtual_object(objType, ...) \
struct objType; \
PP_AGT_impl_SPECIALIZE_VIRTUAL_OBJECT_ENUM(objType, objType); \
struct PP_AGT_impl_OBJECT_ATTR(__VA_ARGS__) objType : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_object(objType, ...) \
struct objType; \
PP_AGT_impl_SPECIALIZE_OBJECT_ENUM(objType, objType); \
struct PP_AGT_impl_OBJECT_ATTR(__VA_ARGS__) objType final : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_abstract_object(objType, ...)           \
struct objType;                                       \
PP_AGT_impl_SPECIALIZE_ABSTRACT_OBJECT_ENUM(objType, objType);    \
struct PP_AGT_impl_OBJECT_ATTR(__VA_ARGS__) objType : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_virtual_api_object(apiType, objType, ...) \
PP_AGT_impl_SPECIALIZE_VIRTUAL_OBJECT_ENUM(apiType, objType); \
extern "C" struct PP_AGT_impl_OBJECT_ATTR(__VA_ARGS__) apiType : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_api_object(apiType, objType, ...) \
PP_AGT_impl_SPECIALIZE_OBJECT_ENUM(apiType, objType); \
extern "C" struct PP_AGT_impl_OBJECT_ATTR(__VA_ARGS__) apiType final : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_abstract_api_object(apiType, objType, ...)           \
PP_AGT_impl_SPECIALIZE_ABSTRACT_OBJECT_ENUM(apiType, objType);    \
extern "C" struct PP_AGT_impl_OBJECT_ATTR(__VA_ARGS__) apiType : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_virtual_alias_object(apiType, objType, ...) \
struct apiType; \
PP_AGT_impl_SPECIALIZE_VIRTUAL_OBJECT_ENUM(apiType, objType); \
struct PP_AGT_impl_OBJECT_ATTR(__VA_ARGS__) apiType : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_alias_object(apiType, objType, ...) \
struct apiType; \
PP_AGT_impl_SPECIALIZE_OBJECT_ENUM(apiType, objType); \
struct PP_AGT_impl_OBJECT_ATTR(__VA_ARGS__) apiType final : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_abstract_alias_object(apiType, objType, ...)           \
struct apiType; \
PP_AGT_impl_SPECIALIZE_ABSTRACT_OBJECT_ENUM(apiType, objType);    \
struct PP_AGT_impl_OBJECT_ATTR(__VA_ARGS__) apiType : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_assert_is_a(obj, selfType) AGT_invariant( (obj)->type == object_type::selfType )
#define AGT_assert_is_type(obj, selfType) AGT_invariant( object_type::selfType##_begin <= (obj)->type && (obj)->type <= object_type::selfType##_end )
#define AGT_get_type_index(obj, selfType) static_cast<agt_u32_t>(static_cast<agt_u16_t>((obj)->type) - static_cast<agt_u16_t>(object_type::selfType##_begin))


#endif //AGATE_CORE_IMPL_OBJECT_DEFS_HPP
