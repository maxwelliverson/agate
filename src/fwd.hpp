//
// Created by maxwe on 2021-11-25.
//

#ifndef JEMSYS_AGATE2_FWD_HPP
#define JEMSYS_AGATE2_FWD_HPP

#include "agate.h"

extern "C" {

typedef struct agt_async_data_st*   agt_async_data_t;
typedef struct agt_message_data_st* agt_message_data_t;

}

namespace agt {

  /**
   * TODO: Implement idea for simple, opaque storage of both local and shared handles:
   *           - Opaque handle type is a pointer sized integer (uintptr_t)
   *           - Pointers to handles are always at least word aligned, meaning there are a couple low bits that will always be 0.
   *           - Store the shared allocation handles with such a scheme that the lowest bit is always 1
   *           - When using a handle, test the lowest bit: If the low bit is zero, cast the integer to a pointer and use as is.
   *           -                                           If the low bit is one, interpret the integer as a shared allocation handle, and convert to a local pointer using the local context
   * */

  enum class object_type : agt_u16_t {
    local_agent,
    proxy_agent,
    shared_agent,
    imported_agent,

    local_busy_executor,
    local_single_thread_executor,
    local_pool_executor,
    local_proxy_executor,
    shared_busy_executor,
    shared_single_thread_executor,
    shared_pool_executor,
    shared_proxy_executor,

    local_async_data,
    shared_async_data,

    private_message_pool,
    private_sized_message_pool,
    local_spsc_message_pool,
    local_spsc_sized_message_pool,
    local_mpsc_message_pool,
    local_mpsc_sized_message_pool,
    local_spmc_message_pool,
    local_spmc_sized_message_pool,
    local_mpmc_message_pool,
    local_mpmc_sized_message_pool,
    shared_spsc_message_pool,
    shared_spsc_sized_message_pool,
    shared_mpsc_message_pool,
    shared_mpsc_sized_message_pool,
    shared_spmc_message_pool,
    shared_spmc_sized_message_pool,
    shared_mpmc_message_pool,
    shared_mpmc_sized_message_pool,


    local_spsc_queue_sender,
    local_mpsc_queue_sender,
    local_spmc_queue_sender,
    local_mpmc_queue_sender,
    shared_spsc_queue_sender,
    shared_mpsc_queue_sender,
    shared_spmc_queue_sender,
    shared_mpmc_queue_sender,
    private_queue_sender,
    local_sp_bqueue_sender,
    local_mp_bqueue_sender,
    shared_sp_bqueue_sender,
    shared_mp_bqueue_sender,

    local_spsc_queue_receiver,
    local_mpsc_queue_receiver,
    local_spmc_queue_receiver,
    local_mpmc_queue_receiver,
    shared_spsc_queue_receiver,
    shared_mpsc_queue_receiver,
    shared_spmc_queue_receiver,
    shared_mpmc_queue_receiver,
    private_queue_receiver,
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


    agent_begin    = local_agent,
    agent_end      = imported_agent,
    executor_begin = local_busy_executor,
    executor_end   = shared_proxy_executor,
    sender_begin   = local_spsc_queue_sender,
    sender_end     = shared_mp_bqueue_sender,
    receiver_begin = local_spsc_queue_receiver,
    receiver_end   = shared_mp_bqueue_receiver,
  };

  enum class shared_allocation_id : agt_u64_t;


  enum class connect_action : agt_u32_t;

  enum class error_state : agt_u32_t;

  enum class async_data_t : agt_u64_t;
  enum class async_key_t  : agt_u32_t;

  enum class shared_handle : agt_u64_t;

  enum class context_id    : agt_u32_t;
  enum class name_token    : agt_u64_t;


  using message_pool_t  = void*;
  using message_queue_t = void*;

  using message_block_t = struct message_pool_block*;


  struct object;

  struct handle_header;
  struct shared_object_header;
  struct shared_handle_header;

  struct object_pool;
  struct thread_state;

  struct id;

  struct vtable;
  using  vpointer = const vtable*;


  struct local_channel_header;
  struct shared_channel_header;

  struct private_channel;
  struct local_spsc_channel;
  struct local_spmc_channel;
  struct local_mpsc_channel;
  struct local_mpmc_channel;

  struct sender;
  struct receiver;

  using sender_t   = sender*;
  using receiver_t = receiver*;



  struct agent_instance;



  struct private_channel_sender;
  struct private_channel_receiver;
  struct local_spsc_channel_sender;
  struct local_spsc_channel_receiver;
  struct local_spmc_channel_sender;
  struct local_spmc_channel_receiver;
  struct local_mpsc_channel_sender;
  struct local_mpsc_channel_receiver;
  struct local_mpmc_channel_sender;
  struct local_mpmc_channel_receiver;


  struct shared_spsc_channel_handle;
  struct shared_spsc_channel_sender;
  struct shared_spsc_channel_receiver;
  struct shared_spmc_channel_handle;
  struct shared_spmc_channel_sender;
  struct shared_spmc_channel_receiver;
  struct shared_mpsc_channel_handle;
  struct shared_mpsc_channel_sender;
  struct shared_mpsc_channel_receiver;
  struct shared_mpmc_channel_handle;
  struct shared_mpmc_channel_sender;
  struct shared_mpmc_channel_receiver;


  struct async_data;
  struct imported_async_data;


  template <typename T, bool IsThreadSafe = true>
  class strong_ref;

  template <typename T>
  using unsafe_strong_ref = strong_ref<T, false>;

  template <typename T, typename U = void, bool IsThreadSafe = true>
  class weak_ref;

  template <typename T, typename U = void>
  using unsafe_weak_ref = weak_ref<T, U, false>;



  enum class SharedAllocationId : agt_u64_t;

  enum class ObjectType : agt_u32_t;
  enum class ObjectFlags : agt_handle_flags_t;
  enum class ConnectAction : agt_u32_t;

  enum class ErrorState : agt_u32_t;


  struct HandleHeader;
  struct SharedObjectHeader;

  class Id;

  class Object;

  class Handle;
  class LocalHandle;
  class SharedHandle;
  class SharedObject;
  using LocalObject = LocalHandle;

  struct LocalChannel;
  struct SharedChannel;

  struct VTable;
  using  VPointer = const VTable*;

  struct SharedVTable;

  using  SharedVPtr = const SharedVTable*;


  struct LocalMpScChannel;
  struct SharedMpScChannelSender;
}

#endif//JEMSYS_AGATE2_FWD_HPP
