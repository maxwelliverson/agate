//
// Created by maxwe on 2021-11-22.
//

#ifndef JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
#define JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP

#include "fwd.hpp"

#include "core/thread_state.hpp"
#include "context/context.hpp"
#include "context/error.hpp"
#include "support/atomic.hpp"
#include "support/flags.hpp"


namespace agt {

  enum object_flags_t {
    OBJECT_IS_PRIVATE    = 0x1,
    OBJECT_SLOT_IS_LARGE = 0x4
  };


  struct object {
    object_type type;            // 2 bytes
    agt_u16_t   poolChunkOffset; // DO NOT TOUCH, internal data. Units of 16 bytes
    agt_u32_t   flags;
    agt_u32_t   refCount;
    agt_u32_t   epoch;           // DO NOT TOUCH, internal data. Allows for valid "dangling pointers"
  };



  object*        alloc_obj(object_pool* pool) noexcept;

  object*        retain_obj(object* obj) noexcept;

  void           free_obj(object* obj, thread_state* state = get_thread_state()) noexcept;

  void           release_obj(object* obj) noexcept;



  [[nodiscard]] inline object*  alloc_object32(thread_state*  state = get_thread_state()) noexcept {
    return alloc_obj(&state->poolObj32);
  }

  [[nodiscard]] inline object*  alloc_object64(thread_state*  state = get_thread_state()) noexcept {
    return alloc_obj(&state->poolObj64);
  }

  [[nodiscard]] inline object*  alloc_object128(thread_state* state = get_thread_state()) noexcept {
    return alloc_obj(&state->poolObj128);
  }

  [[nodiscard]] inline object*  alloc_object256(thread_state* state = get_thread_state()) noexcept {
    return alloc_obj(&state->poolObj256);
  }

  [[nodiscard]] inline object*  alloc_object(size_t size, thread_state* state = get_thread_state()) noexcept {
    auto num = std::numeric_limits<size_t>::digits - std::countl_zero(size - 1);
    switch (num) {
      // case 0:
      // case 1:
      // case 2:
      // case 3:
      // case 4:
        // return alloc_object16(state);
      case 5:
        return alloc_object32(state);
      case 6:
        return alloc_object64(state);
      case 7:
        return alloc_object128(state);
      case 8:
        return alloc_object256(state);
      default:
        AGT_assert(size <= 256 && "Invalid object size");
        return nullptr;
    }
  }

  template <std::derived_from<object> Obj> requires(!std::same_as<std::remove_cv_t<Obj>, object>)
  [[nodiscard]] inline Obj*     alloc_obj(thread_state* state = get_thread_state()) noexcept {
    constexpr static size_t ObjSize = std::bit_ceil(sizeof(Obj));
    // if constexpr (ObjSize == 16)
      //return static_cast<Obj*>(alloc_object16(state));
    /*else*/
    if constexpr (ObjSize == 32)
      return static_cast<Obj*>(alloc_object32(state));
    else if constexpr (ObjSize == 64)
      return static_cast<Obj*>(alloc_object64(state));
    else if constexpr (ObjSize == 128)
      return static_cast<Obj*>(alloc_object128(state));
    else if constexpr (ObjSize == 256)
      return static_cast<Obj*>(alloc_object256(state));
    else {
      AGT_assert(ObjSize <= 256 && "Invalid object size");
      return nullptr;
    }
  }



#define AGT_assert_is_a(obj, selfType) AGT_assert( obj->type == object_type::selfType )
#define AGT_assert_is_type(obj, selfType) AGT_assert( object_type::selfType##_begin <= obj->type && obj->type <= object_type::selfType##_end )
#define AGT_get_type_index(obj, selfType) static_cast<agt_u32_t>(static_cast<agt_u16_t>(obj->type) - static_cast<agt_u16_t>(object_type::selfType##_begin))



  namespace impl {
    template <bool IsThreadSafe>
    class strong_ref_base {
    public:

      strong_ref_base() = default;
      strong_ref_base(const strong_ref_base&) = delete;
      strong_ref_base(strong_ref_base&& other) noexcept
          : m_ptr(std::exchange(other.m_ptr, nullptr)) { }
      explicit strong_ref_base(object* ptr) noexcept
          : m_ptr(ptr) { }

      strong_ref_base& operator=(const strong_ref_base&) = delete;
      strong_ref_base& operator=(strong_ref_base&& other) noexcept {
        _free();
        m_ptr = std::exchange(other.m_ptr, nullptr);
        return *this;
      }

      ~strong_ref_base() {
        _free();
      }


      [[nodiscard]] strong_ref_base clone() const noexcept {
        if (m_ptr) {
          if constexpr (IsThreadSafe)
            atomicRelaxedIncrement(m_ptr->refCount);
          else
            ++m_ptr->refCount;
        }
        return strong_ref_base{ m_ptr };
      }

      [[nodiscard]] object* get() const noexcept {
        return m_ptr;
      }

      [[nodiscard]] object* release() noexcept {
        return std::exchange(m_ptr, nullptr);
      }

      void reset(object* obj) noexcept {
        if (obj != m_ptr) {
          auto o = _acquire_or_null(obj);
          _free();
          m_ptr = obj;
        }
      }


      static strong_ref_base load(object* obj) noexcept {
        strong_ref_base p;
        p.m_ptr = obj;
        return std::move(p);
      }

    private:

      [[nodiscard]] static object* _acquire(object* obj) noexcept {
        if constexpr (IsThreadSafe)
          atomicRelaxedIncrement(obj->refCount);
        else
          ++obj->refCount;
        return obj;
      }

      [[nodiscard]] static object* _acquire_or_null(object* obj) noexcept {
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


      object* m_ptr;
    };

    template <bool IsThreadSafe>
    class weak_ref_base {
      static void _release(object* obj) noexcept {
        if (obj)
          release_obj(obj);
      }
      [[nodiscard]] AGT_forceinline static object*   _retain(object* obj) noexcept {
        return obj ? retain_obj(obj) : nullptr;
      }
      [[nodiscard]] AGT_forceinline static agt_u32_t _get_epoch(object* obj) noexcept {
        // Might be able to get away with a memory barrier rather than a full blown atomic load?
        // At least one strong reference to obj is guaranteed to persist throughout the duration
        // of the retain operation, and epoch is only updated when there are no strong references
        // active. Thus, epoch is guaranteed to not be written to for the duration of the retain
        // operation, and therefore the load does not need to be atomic with respect to other
        // threads, but rather it needs only to ensure that any possible prior writes to epoch
        // are detected.
        // Because epoch is only ever written to with atomic operations, placing an acquire fence
        // prior to the memory read ensures proper synchronization.

        std::atomic_thread_fence(std::memory_order_acquire);
        return obj->epoch;
      }
    public:

      weak_ref_base() = default;
      weak_ref_base(const weak_ref_base&) = delete;
      explicit weak_ref_base(object* obj) noexcept
          : m_ptr(_retain(obj)),
            m_epoch(obj ? _get_epoch(obj) : 0)
      { }
      weak_ref_base(std::in_place_t, object* obj, agt_u32_t epoch) noexcept
          : m_ptr(retain_obj(obj)),
            m_epoch(epoch)
      { }
      template <typename T>
      weak_ref_base(object* obj, T&& value) noexcept
          : m_ptr(_retain(obj)),
            m_epoch(obj ? _get_epoch(obj) : 0) {
        emplace<std::remove_cvref_t<T>>(std::forward<T>(value));
      }
      template <typename T>
      weak_ref_base(object* obj, agt_u32_t epoch, T&& value) noexcept
          : m_ptr(retain_obj(obj)),
            m_epoch(epoch) {
        emplace<std::remove_cvref_t<T>>(std::forward<T>(value));
      }

      weak_ref_base& operator=(weak_ref_base&& other) noexcept {
        _release(m_ptr);
        m_ptr = std::exchange(other.m_ptr, nullptr);
        m_epoch = other.m_epoch;
        m_value = std::exchange(other.m_value, 0);
        return *this;
      }

      ~weak_ref_base() {
        _release(m_ptr);
      }


      [[nodiscard]] object* get() const noexcept {
        return m_ptr;
      }

      [[nodiscard]] agt_u32_t epoch() const noexcept {
        return m_epoch;
      }

      template <typename T>
      void move_assign(weak_ref_base&& other, T&& val) noexcept {
        using value_type = std::remove_cvref_t<T>;
        _release(m_ptr);
        m_ptr = std::exchange(other.m_ptr, nullptr);
        m_epoch = other.m_epoch;
        *reinterpret_cast<value_type*>(static_cast<void*>(&m_value)) = std::forward<T>(val);
      }

      [[nodiscard]] object*&         ptr()       &  noexcept {
        return m_ptr;
      }
      [[nodiscard]] object* const &  ptr() const &  noexcept {
        return m_ptr;
      }
      [[nodiscard]] object*&&        ptr()       && noexcept {
        return std::move(m_ptr);
      }
      [[nodiscard]] object* const && ptr() const && noexcept {
        return std::move(m_ptr);
      }



      [[nodiscard]]       agt_u32_t&  second()       &  noexcept {
        return m_value;
      }
      [[nodiscard]] const agt_u32_t&  second() const &  noexcept {
        return m_value;
      }
      [[nodiscard]]       agt_u32_t&& second()       && noexcept {
        return std::move(m_value);
      }
      [[nodiscard]] const agt_u32_t&& second() const && noexcept {
        return std::move(m_value);
      }

      template <typename T, typename ...Args>
      T& emplace(Args&& ...args) noexcept {
        static_assert(sizeof(T) <= sizeof(agt_u32_t));
        return *new(&m_value) T(std::forward<Args>(args)...);
      }

      [[nodiscard]] object* try_load_ptr() noexcept {
        union {
          struct {
            agt_u32_t refCount;
            agt_u32_t epoch;
          };
          agt_u64_t field;
        };

        if constexpr (IsThreadSafe) {
          field = atomicRelaxedIncrement(_ctl_field());
          if (refCount == static_cast<agt_u32_t>(-1)) [[unlikely]] {
            _reset_epoch(epoch + 1);
            return nullptr;
          }
          if (epoch != m_epoch) {
            free_obj(m_ptr);
            return nullptr;
          }
        }
        else {
          if (m_ptr->epoch != m_epoch)
            return nullptr;
          ++m_ptr->refCount;
        }

        return m_ptr;
      }

    protected:

      AGT_noinline void _reset_epoch(agt_u32_t badEpoch) noexcept {
        union field {
          struct {
            agt_u32_t refCount;
            agt_u32_t epoch;
          };
          agt_u64_t value;
        } prev, next;

        prev.refCount = 0;
        prev.epoch    = badEpoch;

        do {
          next.refCount = prev.refCount - 1;
          next.epoch    = prev.epoch    - 1;
        } while (!atomicCompareExchange(_ctl_field(), prev.value, next.value));

        err::internal_overflow info{
            .obj = m_ptr,
            .msg = "Reference count overflow",
            .fieldBits = 32
        };

        raiseError(AGT_ERROR_INTERNAL_OVERFLOW, &info);
      }






    private:

      [[nodiscard]] AGT_forceinline agt_u64_t& _ctl_field() noexcept {
        return *reinterpret_cast<agt_u64_t*>(static_cast<void*>(&m_ptr->refCount));
      }

      object*   m_ptr;
      agt_u32_t m_epoch;
      agt_u32_t m_value;
    };


    extern template class strong_ref_base<true>;
    extern template class strong_ref_base<false>;
    extern template class weak_ref_base<true>;
    extern template class weak_ref_base<false>;
  }




  template <typename T, bool IsThreadSafe>
  class strong_ref {
    template <typename, typename, bool>
    friend class weak_ref;

    strong_ref(impl::strong_ref_base<IsThreadSafe> val) noexcept
        : m_val(std::move(val)) { }

  public:

    using pointer   = T*;
    using reference = T&;

    strong_ref() = default;
    explicit strong_ref(pointer p) noexcept
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

    [[nodiscard]] inline strong_ref clone() const noexcept {
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
    impl::strong_ref_base<IsThreadSafe> m_val;
  };

  template <typename T, bool IsThreadSafe>
  class weak_ref<T, void, IsThreadSafe> {
    friend strong_ref<T, IsThreadSafe>;
  public:

    using pointer   = T*;
    using reference = T&;
    using strong_ref_type = strong_ref<T, IsThreadSafe>;

    weak_ref() noexcept : m_val(nullptr) { }
    weak_ref(pointer p) noexcept : m_val(p) { }
    weak_ref(pointer p, agt_u32_t epoch) noexcept : m_val(std::in_place, p, epoch) { }

    weak_ref(weak_ref&& other) noexcept
        : m_val(std::in_place,
                std::exchange(other.m_val.ptr(), nullptr),
                other.m_val.epoch())
    { }

    weak_ref& operator=(weak_ref&& other) noexcept {
      m_val = std::move(other.m_val);
      // m_val.move_assign(std::move(other.m_val), std::move(other.second()));
      return *this;
    }

    ~weak_ref() = default;

    [[nodiscard]] inline strong_ref_type     try_acquire() noexcept {
      return { static_cast<pointer>(m_val.try_load_ptr()) };
    }

    [[nodiscard]] inline agt_u32_t           epoch() const noexcept {
      return m_val.epoch();
    }

    [[nodiscard]] inline explicit operator bool() const noexcept {
      return m_val.ptr() != nullptr;
    }

  private:
    impl::weak_ref_base<IsThreadSafe> m_val;
  };

  template <typename T, typename U, bool IsThreadSafe>
  class weak_ref {
    friend strong_ref<T, IsThreadSafe>;
  public:

    using pointer   = T*;
    using reference = T&;
    using strong_ref_type = strong_ref<T, IsThreadSafe>;
    using second_type = U;

    weak_ref() noexcept requires std::default_initializable<U> : m_val(nullptr, U{}) { }
    weak_ref(pointer p) noexcept requires std::default_initializable<U> : m_val(p, U{}) { }
    weak_ref(pointer p, const U& val) noexcept requires std::copy_constructible<U> : m_val(p, val) { }
    weak_ref(pointer p, U&& val) noexcept requires std::move_constructible<U> : m_val(p, std::move(val)) { }
    weak_ref(pointer p, agt_u32_t epoch, const U& val) noexcept requires std::copy_constructible<U> : m_val(p, epoch, val) { }
    weak_ref(pointer p, agt_u32_t epoch, U&& val) noexcept requires std::move_constructible<U> : m_val(p, epoch, std::move(val)) { }

    weak_ref(weak_ref&& other) noexcept requires std::move_constructible<U>
        : m_val(std::exchange(other.m_val.ptr(), nullptr),
                other.m_val.epoch(),
                std::move(other.second()))
    { }

    weak_ref& operator=(weak_ref&& other) noexcept requires std::is_move_assignable_v<U> {
      m_val.move_assign(std::move(other.m_val), std::move(other.second()));
      return *this;
    }

    ~weak_ref() requires std::is_trivially_destructible_v<U> = default;

    ~weak_ref() {
      m_val.second().~U();
      // destruction of the pointer should be taken care of automatically by the compiler
      // m_val.~impl::weak_ref_base<IsThreadSafe>();
    }



    [[nodiscard]] inline strong_ref_type     try_acquire() noexcept {
      return { static_cast<pointer>(m_val.try_load_ptr()) };
    }

    [[nodiscard]] inline agt_u32_t           epoch() const noexcept {
      return m_val.epoch();
    }

    [[nodiscard]] inline second_type&        second()       &  noexcept {
      return *reinterpret_cast<second_type*>(static_cast<void*>(&m_val.second()));
    }
    [[nodiscard]] inline const second_type&  second() const &  noexcept {
      return *reinterpret_cast<const second_type*>(static_cast<const void*>(&m_val.second()));
    }
    [[nodiscard]] inline second_type&&       second()       && noexcept {
      return std::move(*reinterpret_cast<second_type*>(static_cast<void*>(&m_val.second())));
    }
    [[nodiscard]] inline const second_type&& second() const && noexcept {
      return std::move(*reinterpret_cast<const second_type*>(static_cast<const void*>(&m_val.second())));
    }

    [[nodiscard]] inline explicit operator bool() const noexcept {
      return m_val.ptr() != nullptr;
    }

  private:
    impl::weak_ref_base<IsThreadSafe> m_val;
  };





  /*enum class connect_action : agt_u32_t {
    connectSender,
    disconnectSender,
    connectReceiver,
    disconnectReceiver
  };

  AGT_forceinline static agt_handle_type_t toHandleType(object_type type) noexcept;

  AGT_BITFLAG_ENUM(object_flags, agt_handle_flags_t) {
    isShared                  = 0x01,
    isBuiltinType             = 0x02,
    isSender                  = 0x04,
    isReceiver                = 0x08,
    isUnderlyingType          = 0x10,
    supportsOutOfLineMsg      = 0x10000,
    supportsMultiFrameMsg     = 0x20000,
    supportsExternalMsgMemory = 0x40000
  };

  constexpr static auto getHandleFlagMask() noexcept {
    if constexpr (AGT_HANDLE_FLAGS_MAX > 0x1000'0001) {
      // 64 bit path
      if (AGT_HANDLE_FLAGS_MAX == 0x1000'0000'0000'0001ULL)
        return static_cast<agt_u64_t>(-1);
      return agt_u64_t ((AGT_HANDLE_FLAGS_MAX - 1) << 1) - 1ULL;
    }
    else {
      // 32 bit path
      if (AGT_HANDLE_FLAGS_MAX == 0x1000'0001)
        return static_cast<agt_u32_t>(-1);
      return agt_u32_t ((AGT_HANDLE_FLAGS_MAX - 1) << 1) - 1;
    }
  }

  AGT_forceinline static agt_handle_flags_t toHandleFlags(object_flags flags) noexcept {
    constexpr static auto Mask = getHandleFlagMask();
    return static_cast<agt_handle_flags_t>(flags) & Mask;
  }


  class proxy_object {
    // The reserved member field is here to enable the spoofing of Handles as objects (Handles are virtual, while general objects are not).
    // Note that while this relies on technically undefined behaviour, the placement of the virtual pointer at the start of objects has been
    // the the behaviour of the three major C++ compilers for long enough that I feel comfortable taking advantage of that particular implementation detail.
    void*           reserved;
    agt_object_id_t id;
    object_type     type;
    object_flags    flags;

  public:

    AGT_nodiscard AGT_forceinline agt_object_id_t get_id() const noexcept {
      return id;
    }
    AGT_nodiscard AGT_forceinline object_type  get_type() const noexcept {
      return type;
    }
    AGT_nodiscard AGT_forceinline object_flags get_flags() const noexcept {
      return flags;
    }
  };


  struct shared_object_header {
    size_t     objectSize;
    agt_object_id_t id;
    object_type  type;
    object_flags flags;
  };

  struct handle_header {
    vpointer        vptr;
    agt_object_id_t id;
    object_type  type;
    object_flags flags;
    agt_ctx_t  context;
  };

  struct shared_handle_header : handle_header {
    shared_object_header* sharedInstance;
  };*/


  /*class Handle {

  protected:

    agt_object_id_t id;
    ObjectType  type;
    ObjectFlags flags;
    agt_ctx_t  context;



    virtual agt_status_t acquire() noexcept = 0;
    // Releases a single handle reference, and if the reference count has dropped to zero, destroys the object.
    // By delegating responsibility of destruction to this function, a redundant virtual call is saved on the fast path
    // The object reference on which this function is called will be invalid after, and must not be used for anything else.
    virtual void      release() noexcept = 0;



  public:

    Handle(const Handle&) = delete;



    AGT_nodiscard AGT_forceinline ObjectType  getType() const noexcept {
      return type;
    }
    AGT_nodiscard AGT_forceinline ObjectFlags getFlags() const noexcept {
      return flags;
    }
    AGT_nodiscard AGT_forceinline agt_ctx_t  getContext() const noexcept {
      return context;
    }
    AGT_nodiscard AGT_forceinline agt_object_id_t getId() const noexcept {
      return id;
    }

    AGT_nodiscard AGT_forceinline bool        isShared() const noexcept {
      return static_cast<bool>(getFlags() & ObjectFlags::isShared);
    }


    virtual agt_status_t stage(agt_staged_message_t& pStagedMessage, agt_timeout_t timeout) noexcept = 0;
    virtual void      send(agt_message_t message, agt_send_flags_t flags) noexcept = 0;
    virtual agt_status_t receive(agt_message_info_t& pMessageInfo, agt_timeout_t timeout) noexcept = 0;

    virtual void      releaseMessage(agt_message_t message) noexcept = 0;

    virtual agt_status_t connect(Handle* otherHandle, ConnectAction action) noexcept = 0;


    agt_status_t               duplicate(Handle*& newHandle) noexcept;

    void                    close() noexcept;

  };

  class LocalHandle : public Handle {
  protected:

    ~LocalHandle() = default;
  };

  class SharedObject {

    void*       reserved;
    agt_object_id_t id;
    ObjectType  type;
    ObjectFlags flags;


  protected:

    SharedObject(ObjectType type, ObjectFlags flags, agt_object_id_t id) noexcept
        : type(type),
          flags(flags),
          id(id)
    {}

  public:

    AGT_nodiscard ObjectType  getType() const noexcept {
      return type;
    }
    AGT_nodiscard ObjectFlags getFlags() const noexcept {
      return flags;
    }
    AGT_nodiscard agt_object_id_t getId() const noexcept {
      return id;
    }
  };

  class SharedHandle : public Handle {
  protected:
    SharedVPtr    const vptr;
    SharedObject* const instance;

    SharedHandle(SharedObject* pInstance, agt_ctx_t context, Id localId) noexcept;

  public:

    AGT_forceinline agt_status_t sharedAcquire() const noexcept {
      return vptr->acquireRef(instance, getContext());
    }
    AGT_forceinline size_t   sharedRelease() const noexcept {
      return vptr->releaseRef(instance, getContext());
    }

    AGT_nodiscard AGT_forceinline SharedObject* getInstance() const noexcept {
      return instance;
    }


    AGT_nodiscard AGT_forceinline agt_status_t sharedStage(agt_staged_message_t& pStagedMessage, agt_timeout_t timeout) noexcept {
      return vptr->stage(instance, getContext(), pStagedMessage, timeout);
    }
    AGT_forceinline void sharedSend(agt_message_t message, agt_send_flags_t flags) noexcept {
      vptr->send(instance, getContext(), message, flags);
    }
    AGT_nodiscard AGT_forceinline agt_status_t sharedReceive(agt_message_info_t& pMessageInfo, agt_timeout_t timeout) noexcept {
      return vptr->receive(instance, getContext(), pMessageInfo, timeout);
    }

    AGT_nodiscard AGT_forceinline agt_status_t sharedConnect(Handle* otherHandle, ConnectAction action) noexcept {
      return vptr->connect(instance, getContext(), otherHandle, action);
    }


  };*/

  /*template <typename Hdl>
  inline Hdl*      allocHandle(agt_ctx_t ctx) noexcept {
    using HandleType = typename object_info<Hdl>::handle_type;
    auto handle      = (handle_header*)ctxAllocHandle(ctx, sizeof(HandleType), alignof(HandleType));
    handle->vptr     = &vtable_instance<Hdl>;
    handle->type     = object_info<Hdl>::TypeValue;
    handle->context  = ctx;
    return static_cast<Hdl*>(handle);
  }*/


}

#endif//JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
