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

  enum class shared_allocation_id : agt_u64_t;

  enum class object_type : agt_u32_t;
  enum class object_flags : agt_handle_flags_t;
  enum class connect_action : agt_u32_t;

  enum class error_state : agt_u32_t;

  enum class async_data_t : agt_u64_t;
  enum class async_key_t  : agt_u32_t;


  using message_pool_t  = void*;
  using message_queue_t = void*;

  using message_block_t = struct message_pool_block*;


  struct handle_header;
  struct shared_object_header;
  struct shared_handle_header;

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
