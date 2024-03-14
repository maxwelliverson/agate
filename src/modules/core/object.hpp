//
// Created by maxwe on 2021-11-22.
//

#ifndef JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
#define JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP

#include "config.hpp"

#include "agate/atomic.hpp"
#include "agate/flags.hpp"
#include "error.hpp"


namespace agt {

  enum object_flags_t : agt_u8_t {
    OBJECT_IS_PRIVATE    = 0x1,
    OBJECT_IS_TRANSIENT  = 0x2,
    OBJECT_SLOT_IS_LARGE = 0x4,
    OBJECT_IS_REF_COUNTED = 0x8
  };

  enum class object_type : agt_u16_t {

    agent_self,

    local_agent,
    proxy_agent,
    shared_agent,
    imported_agent,

    local_busy_executor,
    local_event_executor,
    local_user_executor,
    local_parallel_executor,
    local_proxy_executor,
    shared_busy_executor,
    shared_single_thread_executor,
    shared_pool_executor,
    shared_proxy_executor,

    local_async_data,
    shared_async_data,
    imported_async_data,

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

    user_allocation,
    user_rc_allocation,


    // unsafe_fiber,
    fiber,
    fctx,
    pooled_fctx,
    // safe_fiber,

    unsafe_fiber_pool,
    safe_fiber_pool,

    local_event_eagent,


    agent_begin    = local_agent,
    agent_end      = imported_agent,

    executor_begin = local_busy_executor,
    executor_end   = shared_proxy_executor,

    sender_begin   = local_spsc_sender,
    sender_end     = shared_mp_bqueue_sender,

    receiver_begin = local_spsc_receiver,
    receiver_end   = shared_mp_bqueue_receiver,

    object_pool_begin = private_object_pool,
    object_pool_end   = shared_object_pool,

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
  };

  struct object {
    object_type type;            // 2 bytes
    agt_u16_t   poolChunkOffset; // DO NOT TOUCH, internal data. Units of 32 bytes
  };


  static_assert(sizeof(object) == sizeof(agt_u32_t));


  // Returns nullptr if ctx has not been initialized
  [[nodiscard]] agt_ctx_t get_ctx() noexcept;

  [[nodiscard]] agt_ctx_t acquire_ctx(agt_instance_t instance, const agt_allocator_params_t* pAllocParams) noexcept;

  template <typename ObjectType>
  [[nodiscard]] AGT_forceinline ObjectType* alloc(agt_ctx_t ctx = get_ctx()) noexcept;
  template <typename ObjectType>
  [[nodiscard]] AGT_forceinline ObjectType* dyn_alloc(size_t size, agt_ctx_t ctx = get_ctx()) noexcept;

  AGT_forceinline void                      release(object* pObject) noexcept;

  template <std::derived_from<object> ObjectType>
  AGT_forceinline void                      destroy(ObjectType* pObject) noexcept {
    if constexpr (!std::is_trivially_destructible_v<ObjectType>) {
      if (pObject) {
        const auto offset = pObject->poolChunkOffset;
        pObject->~ObjectType();
        pObject->poolChunkOffset = offset; // some fucky shit, yes, but the ensures the compiler doesn't clear this field in the destructor
        release(static_cast<object*>(pObject));
      }
    }
    else {
      release(static_cast<object*>(pObject));
    }
  }



#define AGT_object_type(objType, ...) \
  struct objType; \
  PP_AGT_impl_SPECIALIZE_OBJECT_ENUM(objType); \
  struct objType : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_final_object_type(objType, ...) \
  struct objType; \
  PP_AGT_impl_SPECIALIZE_OBJECT_ENUM(objType); \
  struct objType final : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_virtual_object_type(objType, ...)           \
  struct objType;                                       \
  PP_AGT_impl_SPECIALIZE_OBJECT_ENUM_RANGE(objType);    \
  struct objType : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_assert_is_a(obj, selfType) AGT_assert( (obj)->type == object_type::selfType )
#define AGT_assert_is_type(obj, selfType) AGT_assert( object_type::selfType##_begin <= (obj)->type && (obj)->type <= object_type::selfType##_end )
#define AGT_get_type_index(obj, selfType) static_cast<agt_u32_t>(static_cast<agt_u16_t>((obj)->type) - static_cast<agt_u16_t>(object_type::selfType##_begin))







  /*namespace impl {
    template <bool IsThreadSafe>
    class ref_base {
    public:

      ref_base() = default;
      ref_base(const ref_base&) = delete;
      ref_base(ref_base&& other) noexcept
          : m_ptr(std::exchange(other.m_ptr, nullptr)) { }
      explicit ref_base(rc_object* ptr) noexcept
          : m_ptr(ptr) { }

      ref_base& operator=(const ref_base&) = delete;
      ref_base& operator=(ref_base&& other) noexcept {
        _free();
        m_ptr = std::exchange(other.m_ptr, nullptr);
        return *this;
      }

      ~ref_base() {
        _free();
      }


      [[nodiscard]] ref_base clone() const noexcept {
        if (m_ptr) {
          if constexpr (IsThreadSafe)
            atomicRelaxedIncrement(m_ptr->refCount);
          else
            ++m_ptr->refCount;
        }
        return ref_base{ m_ptr };
      }

      [[nodiscard]] rc_object* get() const noexcept {
        return m_ptr;
      }

      [[nodiscard]] rc_object* release() noexcept {
        return std::exchange(m_ptr, nullptr);
      }

      void reset(rc_object* obj) noexcept {
        if (obj != m_ptr) {
          auto o = _acquire_or_null(obj);
          _free();
          m_ptr = obj;
        }
      }


      static ref_base load(rc_object* obj) noexcept {
        ref_base p;
        p.m_ptr = obj;
        return std::move(p);
      }

    private:

      [[nodiscard]] static rc_object* _acquire(rc_object* obj) noexcept {
        if constexpr (IsThreadSafe)
          atomicRelaxedIncrement(obj->refCount);
        else
          ++obj->refCount;
        return obj;
      }

      [[nodiscard]] static rc_object* _acquire_or_null(rc_object* obj) noexcept {
        if (obj) {
          if constexpr (IsThreadSafe)
            atomicRelaxedIncrement(obj->refCount);
          else
            ++obj->refCount;
        }
        return obj;
      }

      void _free() noexcept {
        if (m_ptr) {
          if constexpr (IsThreadSafe) {
            if (atomicDecrement(m_ptr->refCount) == 1)
              free_obj(m_ptr);
          }
          else {
            if (--m_ptr->refCount == 0)
              free_obj(m_ptr);
          }
        }
      }


      rc_object* m_ptr;
    };

    extern template class ref_base<true>;
    extern template class ref_base<false>;
  }

  template <typename T, bool IsThreadSafe>
  class ref {
    ref(impl::ref_base<IsThreadSafe> val) noexcept
        : m_val(std::move(val)) { }

  public:

    using pointer   = T*;
    using reference = T&;

    ref() = default;
    explicit ref(pointer p) noexcept
        : m_val(p) {
      static_assert(std::derived_from<T, object>);
    }


    [[nodiscard]] inline pointer   operator->() const noexcept {
      return m_val.get();
    }
    [[nodiscard]] inline reference operator*()  const noexcept {
      return *m_val.get();
    }


    [[nodiscard]] inline pointer get() const noexcept {
      return m_val.get();
    }

    [[nodiscard]] inline ref clone() const noexcept {
      return strong_ref(m_val.clone());
    }

    inline pointer release() noexcept {
      return static_cast<pointer>(m_val.release());
    }

    inline void    reset(pointer p) noexcept {
      m_val.reset(p);
    }

    [[nodiscard]] inline explicit operator bool() const noexcept {
      return m_val.get() != nullptr;
    }

  private:
    impl::ref_base<IsThreadSafe> m_val;
  };*/
}

#endif//JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
