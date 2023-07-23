//
// Created by maxwe on 2021-11-25.
//

#ifndef JEMSYS_AGATE2_FWD_HPP
#define JEMSYS_AGATE2_FWD_HPP



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

  // handle layout:
  //       epoch      segmentId    segmentOffset     segmentSizeClass    flags   isShared
  //   [   63:48    |        |   2:1   |      0      ]

  /**
   * isShared      = 0:0
   * isTransient   = 1:1
   * reservedFlags = 3:2
   * segmentOffset = 23:4
   *
   * */


  enum class object_type : agt_u16_t;

  enum class shared_allocation_id : agt_u64_t;

  enum class object_handle : uintptr_t;


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
  struct rc_object;

  struct object_pool;
  struct thread_state;

  struct export_table;

  struct id;

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


  AGT_BITFLAG_ENUM(thread_safety, agt_u32_t) {
    unsafe        = 0x0,
    producer_safe = 0x1,
    consumer_safe = 0x2,
    safe          = consumer_safe | producer_safe,
    dynamic       = 0x4
  };

  inline constexpr static thread_safety thread_unsafe        = thread_safety::unsafe;
  inline constexpr static thread_safety thread_consumer_safe = thread_safety::consumer_safe;
  inline constexpr static thread_safety thread_owner_safe    = thread_consumer_safe;
  inline constexpr static thread_safety thread_producer_safe = thread_safety::producer_safe;
  inline constexpr static thread_safety thread_user_safe     = thread_producer_safe;
  inline constexpr static thread_safety thread_safe          = thread_safety::safe;

  inline constexpr static thread_safety default_safety_model(bool threadsAreEnabled) noexcept {
    return threadsAreEnabled ? thread_safe : thread_unsafe;
  }






  namespace impl {
    template <typename T>
    struct default_destroy {
      AGT_forceinline constexpr void operator()(T* value) const noexcept {
        AGT_invariant(value != nullptr);
        if constexpr (!std::is_trivially_destructible_v<T>)
          std::destroy_at(value);
      }
    };
  }


  namespace impl::obj_types {
    template <typename T>
    struct object_type_id;
    template <typename T>
    struct object_type_range;
  }



  template <typename T,
            typename Dtor = impl::default_destroy<T>,
            thread_safety SafetyModel = thread_user_safe>
  class owner_ref;

  template <typename T, typename Dtor = impl::default_destroy<T>>
  using safe_owner_ref = owner_ref<T, Dtor, thread_safe>;

  template <typename T, typename Dtor = impl::default_destroy<T>>
  using unsafe_owner_ref = owner_ref<T, Dtor, thread_unsafe>;

  template <typename T,
            typename U = void,
            typename Dtor = impl::default_destroy<T>,
            thread_safety SafetyModel = thread_user_safe>
  class maybe_ref;

  template <typename T, typename U = void,
            typename Dtor = impl::default_destroy<T>>
  using unsafe_maybe_ref = maybe_ref<T, U, Dtor, thread_unsafe>;

  template <typename T,
            typename U = void,
            typename Dtor = impl::default_destroy<T>>
  using safe_maybe_ref = maybe_ref<T, U, Dtor, thread_safe>;
}



#endif//JEMSYS_AGATE2_FWD_HPP
