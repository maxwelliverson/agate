//
// Created by maxwe on 2021-11-22.
//

#ifndef JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
#define JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP

#include "config.hpp"

#include "agate/atomic.hpp"
#include "agate/flags.hpp"
#include "error.hpp"

#include "ctx.hpp"


// Note, the various bits of object infrastructure are not actually defined in this file,
// but are rather in impl/object_defs.hpp, impl/sized_pool.hpp, and impl/allocator.hpp
// Including this file however, does ensure all of the required files are included.


namespace agt {

  /*enum object_flags_t : agt_u8_t {
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
  }*/



/*
#define AGT_virtual_object(objType, ...) \
  struct objType; \
  PP_AGT_impl_SPECIALIZE_VIRTUAL_OBJECT_ENUM(objType); \
  struct objType : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_object(objType, ...) \
  struct objType; \
  PP_AGT_impl_SPECIALIZE_OBJECT_ENUM(objType); \
  struct objType final : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_abstract_object(objType, ...)           \
  struct objType;                                       \
  PP_AGT_impl_SPECIALIZE_ABSTRACT_OBJECT_ENUM(objType);    \
  struct objType : PP_AGT_impl_OBJECT_BASE(__VA_ARGS__)

#define AGT_assert_is_a(obj, selfType) AGT_assert( (obj)->type == object_type::selfType )
#define AGT_assert_is_type(obj, selfType) AGT_assert( object_type::selfType##_begin <= (obj)->type && (obj)->type <= object_type::selfType##_end )
#define AGT_get_type_index(obj, selfType) static_cast<agt_u32_t>(static_cast<agt_u16_t>((obj)->type) - static_cast<agt_u16_t>(object_type::selfType##_begin))
*/


  /*
  template <typename T>
  class weak_ref;
  template <typename T>
  class ref;


  template <typename T>
  class ref {
    friend weak_ref<T>;
  public:

    using pointer   = T*;
    using reference = T&;

    ref() = default;
    ref(const ref&) = delete;
    ref(ref&& other) noexcept : m_ptr(std::exchange(other.m_ptr, nullptr)) { }
    ref(pointer p) noexcept
        : m_ptr(p) {
      static_assert(std::derived_from<T, rc_object>);
    }

    ref(const weak_ref<T>& weak, agt_epoch_t epoch) noexcept;

    ref& operator=(const ref&) = delete;
    ref& operator=(ref&& other) noexcept {
      if (m_ptr)
        agt::release(m_ptr);
      m_ptr = std::exchange(other.m_ptr, nullptr);
      return *this;
    }

    ~ref() {
      if (m_ptr)
        agt::release(m_ptr);
    }


    [[nodiscard]] inline pointer   operator->() const noexcept {
      return m_ptr;
    }
    [[nodiscard]] inline reference operator*()  const noexcept {
      return *m_ptr;
    }


    [[nodiscard]] inline pointer get() const noexcept {
      return m_ptr;
    }

    [[nodiscard]] inline ref clone() const noexcept {
      if (m_ptr)
        agt::retain(m_ptr);
      return ref(m_ptr);
    }

    template <typename ...Args>
    inline void    recycle(Args&& ...args) noexcept {
      AGT_assert( m_ptr != nullptr );
      m_ptr = agt::recycle(m_ptr, std::forward<Args>(args)...);
    }

    inline pointer take() noexcept {
      return std::exchange(m_ptr, nullptr);
    }

    template <typename ...Args>
    inline void    reset(pointer p, Args&& ...args) noexcept {
      if (m_ptr)
        agt::release(m_ptr, std::forward<Args>(args)...);
      m_ptr = p;
    }

    // Disable overload if Args is a single argument that is convertible to pointer. In that case, we wish to select the other overload.
    template <typename ...Args> requires(!(sizeof...(Args) == 1 && (std::convertible_to<Args, pointer> && ...)))
    inline void    reset(Args&& ...args) noexcept {
      if (m_ptr)
        agt::release(m_ptr, std::forward<Args>(args)...);
      m_ptr = nullptr;
    }

    [[nodiscard]] inline explicit operator bool() const noexcept {
      return m_ptr != nullptr;
    }

  private:
    pointer m_ptr = nullptr;
  };

  template <typename T>
  class weak_ref {
    friend ref<T>;

    weak_ref(T* ptr) noexcept : m_ptr(ptr) { }
  public:

    using pointer   = T*;
    using reference = T&;

    weak_ref() = default;
    weak_ref(const weak_ref&) = delete;
    weak_ref(weak_ref&& other) noexcept
      : m_ptr(std::exchange(other.m_ptr, nullptr)) { }

    weak_ref(const ref<T>& obj, agt_epoch_t& key) noexcept
      : weak_ref(obj.get(), key) { }
    weak_ref(pointer ptr, agt_epoch_t& key) noexcept
      : m_ptr(ptr) {
      static_assert(std::derived_from<T, rc_object>);
      key = take_weak_ref(ptr);
    }

    weak_ref& operator=(const weak_ref&) = delete;
    weak_ref& operator=(weak_ref&& other) noexcept {
      if (m_ptr)
        drop_weak_ref(m_ptr);
      m_ptr = std::exchange(other.m_ptr, nullptr);
      return *this;
    }

    ~weak_ref() {
      if (m_ptr)
        drop_weak_ref(m_ptr);
    }


    weak_ref<T> clone(agt_epoch_t epoch) noexcept {
      if (m_ptr && !retain_weak_ref(m_ptr, epoch))
        return nullptr;
      return m_ptr;
    }

    ref<T>      actualize(agt_epoch_t epoch) noexcept {
      return { *this, epoch };
    }


    void        drop() noexcept {
      if (m_ptr) {
        drop_weak_ref(m_ptr);
        m_ptr = nullptr;
      }
    }


    [[nodiscard]] inline explicit operator bool() const noexcept {
      return m_ptr != nullptr;
    }

  private:
    pointer m_ptr = nullptr;
  };


  template <typename T>
  ref<T>::ref(const weak_ref<T>& weak, agt_epoch_t epoch) noexcept
    : m_ptr(static_cast<pointer>(actualize_weak_ref(weak.m_ptr, epoch))) { }

  template <typename To, typename From>
  ref<To> ref_cast(ref<From>&& r) noexcept {
    return static_cast<To*>(r.take());
  }
  */


}

#endif//JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
