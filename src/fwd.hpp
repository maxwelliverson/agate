//
// Created by maxwe on 2021-11-25.
//

#ifndef JEMSYS_AGATE2_FWD_HPP
#define JEMSYS_AGATE2_FWD_HPP

#include <agate.h>

extern "C" {

typedef struct agt_async_data_st*   agt_async_data_t;
typedef struct agt_message_data_st* agt_message_data_t;

}

namespace agt {

  enum class shared_allocation_id : agt_u64_t;

  enum class object_type : agt_u32_t;
  enum class object_flags : agt_handle_flags_t;
  enum class connect_action : agt_u32_t;

  enum class error_state : agt_u32_t;


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
