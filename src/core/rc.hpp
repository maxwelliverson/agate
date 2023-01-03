//
// Created by maxwe on 2022-12-20.
//

#ifndef AGATE_CORE_TRANSIENT_HPP
#define AGATE_CORE_TRANSIENT_HPP

#include "config.hpp"
#include "agate/atomic.hpp"

#include <utility>
#include <memory>

namespace agt {

  struct rcpool;

  struct rc_object {
    agt_u16_t nextFreeSlot;
    agt_u16_t thisSlot;
    agt_u32_t flags;
    agt_u32_t refCount;
    agt_u32_t epoch;
  };

  namespace impl {

    struct rcpool_chunk;
    using rcpool_chunk_t = rcpool_chunk*;

    inline constexpr static size_t    UserRCAllocationHeaderSize = sizeof(rc_object);
    inline constexpr static agt_u16_t UserRCAllocationMarkerConstant = 0xDEE2;

    AGT_forceinline bool           _is_user_rc_alloc(void* ptr) noexcept {
      agt_u16_t field;
      auto cPtr = static_cast<char*>(ptr);
      std::memcpy(&field, cPtr - UserRCAllocationHeaderSize, sizeof field);
      return field == UserRCAllocationMarkerConstant;
    }
    AGT_forceinline bool           _is_user_rc_alloc(rc_object& obj) noexcept {
      return obj.nextFreeSlot == UserRCAllocationMarkerConstant;
    }

    AGT_forceinline rc_object*     _cast_rc_from_user_address(void* ptr) noexcept {
      return reinterpret_cast<rc_object*>(reinterpret_cast<char*>(ptr) - UserRCAllocationHeaderSize);
    }
    AGT_forceinline void*          _cast_user_address_from_rc(rc_object* obj) noexcept {
      return reinterpret_cast<char*>(obj) + UserRCAllocationHeaderSize;
    }

    AGT_forceinline rc_object*     _cast_rc_from_weak_ref(agt_weak_ref_t ref) noexcept {
      return reinterpret_cast<rc_object*>(ref);
    }
    AGT_forceinline agt_weak_ref_t _cast_weak_ref_from_rc(rc_object* obj) noexcept {
      return reinterpret_cast<agt_weak_ref_t>(obj);
    }


    AGT_forceinline agt_u64_t& _get_cas_value(rc_object& obj) noexcept {
      return *std::assume_aligned<8>(reinterpret_cast<agt_u64_t*>(&obj.refCount));
    }

    union rc_object_cas_wrapper {
      struct {
        agt_u32_t refCount;
        agt_u32_t epoch;
      };
      agt_u64_t u64;
    };

    AGT_forceinline void atomicCASInit(rc_object& obj, rc_object_cas_wrapper& old) noexcept {
      old.u64 = atomicRelaxedLoad(_get_cas_value(obj));
    }

    AGT_forceinline bool atomicCAS(rc_object& obj, rc_object_cas_wrapper& old, rc_object_cas_wrapper next) noexcept {
      return atomicCompareExchangeWeak(_get_cas_value(obj), old.u64, next.u64);
    }

    AGT_forceinline rcpool_chunk_t _get_chunk(rc_object& obj) noexcept;
    AGT_forceinline rcpool*        _get_pool(rcpool_chunk_t chunk) noexcept;


    template <thread_safety SafetyModel>
    AGT_forceinline rc_object*     _alloc_from_rcpool(rcpool& pool) noexcept;

    template <thread_safety SafetyModel>
    AGT_forceinline void           _return_to_chunk(rcpool_chunk_t chunk, rc_object& obj) noexcept;

    template <bool AtomicOp>
    AGT_forceinline void           _retain_chunk(rcpool_chunk_t chunk, agt_u32_t count) noexcept;

    template <bool AtomicOp>
    AGT_forceinline void           _release_chunk(rcpool_chunk_t chunk, agt_u32_t count) noexcept;
  }


  [[nodiscard]] inline rcpool*    get_owner_pool(rc_object& obj) noexcept {
    return impl::_get_pool(impl::_get_chunk(obj));
  }


  template <thread_safety SafetyModel>
  [[nodiscard]] inline rc_object* alloc_rc_object(rcpool& pool, agt_u32_t initialRefCount) noexcept {

    auto obj        = impl::_alloc_from_rcpool<SafetyModel>(pool);

    obj->refCount = initialRefCount;

    if constexpr (test(SafetyModel, thread_safe))
      std::atomic_thread_fence(std::memory_order_release);

    return obj;
  }
  template <thread_safety SafetyModel>
  inline void                     release_rc_object(rc_object& obj, agt_u32_t releaseCount) noexcept {
    constexpr static bool IsThreadSafe = test(SafetyModel, thread_safe);
    constexpr static bool IsUserSafe   = test(SafetyModel, thread_user_safe);

    bool objectIsReleased;

    if constexpr (IsThreadSafe) {

      // Use a CAS loop to ensure that when the reference count is decreased to zero,
      // the the epoch is incremented in the same atomic operation as the reference count decrease.

      impl::rc_object_cas_wrapper old, next;

      impl::atomicCASInit(obj, old);

      do {
        next.refCount = old.refCount - releaseCount;
        next.epoch    = old.epoch;
        objectIsReleased = next.refCount == 0;
        if (objectIsReleased)
          next.epoch += 1;
      } while(!impl::atomicCAS(obj, old, next));
    }
    else {
      objectIsReleased = (obj.refCount -= releaseCount) == 0;
      if (objectIsReleased)
        obj.epoch += 1;
    }

    if (objectIsReleased)
      impl::_return_to_chunk<SafetyModel>(impl::_get_chunk(obj), obj);
  }
  template <thread_safety SafetyModel, std::derived_from<rc_object> T, std::invocable<T*> F>
  inline void                     release_rc_object(T& dObj, agt_u32_t releaseCount, F&& destroyFunc) noexcept {
    constexpr static bool IsThreadSafe = test(SafetyModel, thread_safe);
    constexpr static bool IsUserSafe   = test(SafetyModel, thread_user_safe);

    bool objectIsReleased;

    rc_object& obj = dObj;

    if constexpr (IsThreadSafe) {

      // Use a CAS loop to ensure that when the reference count is decreased to zero,
      // the the epoch is incremented in the same atomic operation as the reference count decrease.

      impl::rc_object_cas_wrapper old, next;

      impl::atomicCASInit(obj, old);

      do {
        next.refCount = old.refCount - releaseCount;
        next.epoch    = old.epoch;
        objectIsReleased = next.refCount == 0;
        if (objectIsReleased)
          next.epoch += 1;
      } while(!impl::atomicCAS(obj, old, next));
    }
    else {
      objectIsReleased = (obj.refCount -= releaseCount) == 0;
      if (objectIsReleased)
        obj.epoch += 1;
    }

    if (objectIsReleased) {
      std::invoke(destroyFunc, std::addressof(dObj));
      impl::_return_to_chunk<SafetyModel>(impl::_get_chunk(obj), obj);
    }
  }
  template <thread_safety SafetyModel>
  [[nodiscard]] inline rc_object* recycle_rc_object(rc_object& obj, agt_u32_t releaseCount, agt_u32_t initialRefCount, rcpool* pool = nullptr) noexcept {

    if constexpr (test(SafetyModel, thread_safe)) {
      impl::rc_object_cas_wrapper old, next;

      impl::atomicCASInit(obj, old);

      bool mayRecycleObject;

      if (!pool)
        pool = get_owner_pool(obj);

      do {
        next.epoch      = old.epoch;
        mayRecycleObject = next.refCount == releaseCount;
        if (mayRecycleObject) {
          next.refCount = initialRefCount;
          next.epoch   += 1;
        }
        else {
          next.refCount = old.refCount - releaseCount;
        }
      } while(!impl::atomicCAS(obj, old, next));

      if (mayRecycleObject)
        return &obj;
    }
    else {
      AGT_invariant(obj.refCount > 0);

      if (obj.refCount == releaseCount) {
        ++obj.epoch;
        return &obj;
      }

      obj.refCount -= releaseCount;

      if (!pool)
        pool = get_owner_pool(obj);
    }

    AGT_invariant(pool != nullptr);

    return alloc_rc_object<SafetyModel>(*pool, initialRefCount);
  }
  template <thread_safety SafetyModel, std::derived_from<rc_object> T, std::invocable<T*> Destroy>
  [[nodiscard]] inline T*         recycle_rc_object(T& dObj, agt_u32_t releaseCount, agt_u32_t initialRefCount, rcpool* pool, Destroy&& destroyFunc) noexcept {

    rc_object& obj = dObj;

    if constexpr (test(SafetyModel, thread_safe)) {
      impl::rc_object_cas_wrapper old, next;

      impl::atomicCASInit(obj, old);

      bool mayRecycleObject;

      if (!pool)
        pool = get_owner_pool(obj);

      do {
        next.epoch      = old.epoch;
        mayRecycleObject = next.refCount == releaseCount;
        if (mayRecycleObject) {
          next.refCount = initialRefCount;
          next.epoch   += 1;
        }
        else {
          next.refCount = old.refCount - releaseCount;
        }
      } while(!impl::atomicCAS(obj, old, next));

      if (mayRecycleObject) {
        std::invoke(destroyFunc, std::addressof(dObj));
        return std::addressof(dObj);
      }
    }
    else {
      AGT_invariant(obj.refCount > 0);

      if (obj.refCount == releaseCount) {
        ++obj.epoch;
        std::invoke(destroyFunc, std::addressof(dObj));
        return std::addressof(dObj);
      }

      obj.refCount -= releaseCount;

      if (!pool)
        pool = get_owner_pool(obj);
    }

    AGT_invariant(pool != nullptr);

    return static_cast<T*>(alloc_rc_object<SafetyModel>(*pool, initialRefCount));
  }


  template <thread_safety SafetyModel>
  inline void                     retain_rc_object(rc_object& obj, agt_u32_t retainedCount) noexcept {
    AGT_invariant(retainedCount > 0);
    if constexpr (test(SafetyModel, thread_owner_safe)) {
      atomicExchangeAdd(obj.refCount, retainedCount);
    }
    else {
      obj.refCount += retainedCount;
    }
  }


  template <thread_safety SafetyModel>
  inline void                     take_rc_weak_ref(rc_object& obj, agt_u32_t& epoch, agt_u32_t count) noexcept {
    if constexpr (test(SafetyModel, thread_user_safe)) {
      epoch = atomicRelaxedLoad(obj.epoch);
    }
    else {
      epoch = obj.epoch;
    }

    impl::_retain_chunk<test_any(SafetyModel, thread_safe)>(impl::_get_chunk(obj), count);
  }
  template <thread_safety SafetyModel>
  inline void                     drop_rc_weak_ref(rc_object& obj, agt_u32_t count) noexcept {
    impl::_release_chunk<test(SafetyModel, thread_safe)>(impl::_get_chunk(obj), count);
  }
  template <thread_safety SafetyModel>
  [[nodiscard]] inline bool       retain_rc_weak_ref(rc_object& obj, agt_u32_t epoch, agt_u32_t count) noexcept {
    constexpr static bool IsThreadSafe = test(SafetyModel, thread_safe);

    if constexpr (test(SafetyModel, thread_user_safe))
      std::atomic_thread_fence(std::memory_order_acquire);

    const bool isValidRef = obj.epoch == epoch;

    if (isValidRef)
      impl::_retain_chunk<IsThreadSafe>(impl::_get_chunk(obj), count);

    return isValidRef;
  }
  template <thread_safety SafetyModel>
  [[nodiscard]] inline rc_object* acquire_ownership_of_weak_ref(rc_object& obj, agt_u32_t epoch) noexcept {
    if constexpr (test(SafetyModel, thread_owner_safe)) {
      using namespace impl;

      rc_object_cas_wrapper old, next;

      next.epoch = epoch;

      atomicCASInit(obj, old);

      do {
        if (old.epoch != epoch)
          return nullptr;
        if (old.refCount == UINT32_MAX) [[unlikely]] // Should pretty much never be taken, so this is very reliably predicted and the check causes effectively no slowdown
          return nullptr;
        next.refCount = old.refCount + 1;
      } while(!atomicCAS(obj, old, next));

      return &obj;
    }
    else {
      if (obj.epoch == epoch) {
        if (obj.refCount == UINT32_MAX) [[unlikely]]
          return nullptr;
        ++obj.refCount;
        return &obj;
      }
      return nullptr;
    }

  }



  namespace impl {

    /*inline rc_object*      _alloc_rc_object(rcpool* pool) noexcept;
    inline void            _free_rc_object(rc_object* obj) noexcept;
    inline rcpool* _get_owner_pool(rc_object& obj) noexcept;

    inline void            _retain_rc_object_ref(rc_object& obj) noexcept;
    inline void            _release_rc_object_ref(rc_object& obj) noexcept;*/

    template <thread_safety SafetyModel>
    class owner_ref_base;
    template <thread_safety SafetyModel>
    class maybe_ref_base;

    template <thread_safety SafetyModel>
    class owner_ref_base {
    public:

      owner_ref_base() = default;
      owner_ref_base(const owner_ref_base&) = delete;
      owner_ref_base(owner_ref_base&& other) noexcept
          : m_ptr(std::exchange(other.m_ptr, nullptr)) { }
      explicit owner_ref_base(rc_object* ptr) noexcept
          : m_ptr(ptr) { }

      owner_ref_base& operator=(const owner_ref_base&) = delete;
      owner_ref_base& operator=(owner_ref_base&&) noexcept = delete;


      [[nodiscard]] owner_ref_base _clone() const noexcept {
        return owner_ref_base{ _acquire_or_null(m_ptr) };
      }

      [[nodiscard]] rc_object* _get() const noexcept {
        return m_ptr;
      }

      [[nodiscard]] rc_object* _take() noexcept {
        return std::exchange(m_ptr, nullptr);
      }


      template <typename T, typename Dtor>
      void _reset(T* obj, Dtor& dtor) noexcept {
        if (obj != m_ptr) {
          auto o = _acquire_or_null(obj);
          this->_release<T>(dtor);
          m_ptr = o;
        }
      }
      void _trivial_reset(rc_object* obj) noexcept {
        if (obj != m_ptr) {
          auto o = _acquire_or_null(obj);
          _trivial_release();
          m_ptr = o;
        }
      }


      template <typename T, typename Dtor>
      void _recycle(rcpool* pool, Dtor& dtor) noexcept {
        AGT_invariant(m_ptr != nullptr);
        m_ptr = recycle_rc_object<SafetyModel>(*static_cast<T*>(m_ptr), 1, 1, pool, dtor);
      }
      void _trivial_recycle(rcpool* pool) noexcept {
        AGT_invariant(m_ptr != nullptr);
        m_ptr = recycle_rc_object<SafetyModel>(*m_ptr, 1, 1, pool);
      }

      template <typename T, typename Dtor>
      void _recycle_or_alloc(rcpool& pool, Dtor& dtor) noexcept {
        m_ptr = m_ptr ? recycle_rc_object<SafetyModel>(*static_cast<T*>(m_ptr), 1, 1, &pool, dtor) : alloc_rc_object<SafetyModel>(pool, 1);
      }
      void _trivial_recycle_or_alloc(rcpool& pool) noexcept {
        m_ptr = m_ptr ? recycle_rc_object<SafetyModel>(*m_ptr, 1, 1, &pool) : alloc_rc_object<SafetyModel>(pool, 1);
      }


      template <typename T, typename Dtor>
      void _release(Dtor& dtor) noexcept {
        if (m_ptr)
          release_rc_object<SafetyModel>(*static_cast<T*>(m_ptr), 1, dtor);
      }
      void _trivial_release() noexcept {
        if (m_ptr)
          release_rc_object<SafetyModel>(*m_ptr, 1);
      }

      /*static owner_ref_base load(rc_object* obj) noexcept {
        owner_ref_base p;
        p.m_ptr = obj;
        return std::move(p);
      }*/

    private:

      [[nodiscard]] static rc_object* _acquire(rc_object* obj) noexcept {
        AGT_invariant(obj != nullptr);
        return retain_rc_object<SafetyModel>(*obj, 1);
      }

      [[nodiscard]] static rc_object* _acquire_or_null(rc_object* obj) noexcept {
        return obj ? _acquire(obj) : nullptr;
      }




      rc_object* m_ptr;
    };

    template <thread_safety SafetyModel>
    class maybe_ref_base {
    protected:
      AGT_forceinline static agt_u32_t _take_nonnull(rc_object& obj) noexcept {
        agt_u32_t epoch;
        take_rc_weak_ref<SafetyModel>(obj, epoch, 1);
        return epoch;
      }

      AGT_forceinline static agt_u32_t _take(rc_object* obj) noexcept {
        return obj ? _take_nonnull(*obj) : 0;
      }

      [[nodiscard]] AGT_forceinline rc_object* _retain_nonnull() noexcept {
        AGT_invariant(m_ptr != nullptr);

        if (!retain_rc_weak_ref<SafetyModel>(*m_ptr, m_epoch, 1)) {
          _drop_nonnull();
          m_ptr = nullptr;
        }

        return m_ptr;
      }

      [[nodiscard]] AGT_forceinline rc_object* _retain() noexcept {
        return m_ptr ? _retain_nonnull() : nullptr;
      }

      AGT_forceinline void             _drop_nonnull() const noexcept {
        drop_rc_weak_ref<SafetyModel>(*m_ptr, 1);
      }

      AGT_forceinline void             _drop() const noexcept {
        if (m_ptr)
          _drop_nonnull();
      }



      [[nodiscard]] AGT_forceinline static agt_u32_t _get_epoch(rc_object& obj) noexcept {
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
        return obj.epoch;
      }

    public:

      maybe_ref_base() = default;
      maybe_ref_base(const maybe_ref_base&) = delete;

      explicit maybe_ref_base(rc_object* obj) noexcept
          : m_ptr(obj),
            m_epoch(_take(obj))
      { }

      maybe_ref_base(std::in_place_t, rc_object* obj, agt_u32_t epoch) noexcept
          : m_ptr(obj),
            m_epoch(epoch)
      { }

      template <typename T>
      maybe_ref_base(rc_object* obj, T&& value) noexcept
          : m_ptr(obj),
            m_epoch(_take(obj)) {
        emplace<std::remove_cvref_t<T>>(std::forward<T>(value));
      }

      template <typename T>
      maybe_ref_base(rc_object* obj, agt_u32_t epoch, T&& value) noexcept
          : m_ptr(obj),
            m_epoch(epoch) {
        emplace<std::remove_cvref_t<T>>(std::forward<T>(value));
      }

      maybe_ref_base& operator=(maybe_ref_base&& other) noexcept {
        _drop();
        m_ptr = std::exchange(other.m_ptr, nullptr);
        m_epoch = other.m_epoch;
        m_value = std::exchange(other.m_value, 0);
        return *this;
      }

      ~maybe_ref_base() {
        _drop();
      }

      template <typename T>
      [[nodiscard]] maybe_ref_base clone() noexcept {
        if constexpr (std::same_as<T, void>)
          return maybe_ref_base(std::in_place, _retain(), m_epoch);
        else
          return maybe_ref_base(_retain(), m_epoch, (const T&)m_value);
      }

      [[nodiscard]] rc_object*     get() const noexcept {
        return m_ptr;
      }

      [[nodiscard]] agt_u32_t      epoch() const noexcept {
        return m_epoch;
      }

      template <typename T>
      void move_assign(maybe_ref_base&& other, T&& val) noexcept {
        using value_type = std::remove_cvref_t<T>;
        _drop();
        m_ptr = std::exchange(other.m_ptr, nullptr);
        m_epoch = other.m_epoch;
        *reinterpret_cast<value_type*>(static_cast<void*>(&m_value)) = std::forward<T>(val);
      }

      [[nodiscard]] rc_object*&         ptr()       &  noexcept {
        return m_ptr;
      }
      [[nodiscard]] rc_object* const &  ptr() const &  noexcept {
        return m_ptr;
      }
      [[nodiscard]] rc_object*&&        ptr()       && noexcept {
        return std::move(m_ptr);
      }
      [[nodiscard]] rc_object* const && ptr() const && noexcept {
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

      [[nodiscard]] AGT_forceinline rc_object*       try_acquire_ownership_nonnull() const noexcept {
        AGT_invariant(m_ptr != nullptr);

        return acquire_ownership_of_weak_ref<SafetyModel>(*m_ptr, m_epoch);
      }

      [[nodiscard]] AGT_forceinline rc_object*       try_acquire_ownership() const noexcept {
        return m_ptr ? try_acquire_ownership_nonnull() : nullptr;
      }


    private:

      rc_object* m_ptr;
      agt_u32_t  m_epoch;
      agt_u32_t  m_value;
    };

    template <auto>
    struct func_object;
    template <typename Ret, typename ...Args, Ret(* pfn)(Args...)>
    struct func_object<pfn> {
      using return_type = Ret;
      template <size_t I>
      using arg_type    = std::tuple_element_t<I, std::tuple<Args...>>;
      template <typename ...OArgs> requires std::invocable<decltype(pfn), OArgs...>
      AGT_forceinline constexpr Ret operator()(OArgs&&... args) const noexcept {
        return (*pfn)(std::forward<OArgs>(args)...);
      }
    };
  }

  template <auto PFN>
  using func = impl::func_object<PFN>;

  template <typename T, typename Dtor, thread_safety SafetyModel>
  class owner_ref : Dtor {
    template <typename, typename, typename, thread_safety>
    friend class maybe_ref;

    inline constexpr static bool IsTriviallyDestructible  = std::is_trivially_destructible_v<T>;
    inline constexpr static bool IsTriviallyConstructible = std::is_trivially_constructible_v<T>;
    inline constexpr static bool IsTrivial                = IsTriviallyDestructible && IsTriviallyConstructible;

    using base_type = impl::owner_ref_base<SafetyModel>;

    owner_ref(base_type val) noexcept
        : m_val(std::move(val)) { }

  public:

    using pointer   = T*;
    using reference = T&;
    using destructor = Dtor;

    owner_ref() = default;
    explicit owner_ref(pointer p) noexcept
        : m_val(p) {
      static_assert(std::derived_from<T, rc_object>, "Type parameter T of agt::owner_ref<T, ...> must derive from agt::rc_object");
      static_assert(std::invocable<destructor, T*>, "Type parameter Dtor of agt::owner_ref<T, Dtor, ...> must be an object type callable with one argument of type T*");
    }
    explicit owner_ref(pointer p, const destructor& dtor) noexcept requires std::copy_constructible<destructor>
        : destructor(dtor), m_val(p) {
      static_assert(std::derived_from<T, rc_object>, "Type parameter T of agt::owner_ref<T, ...> must derive from agt::rc_object");
      static_assert(std::invocable<destructor, T*>, "Type parameter Dtor of agt::owner_ref<T, Dtor, ...> must be an object type callable with one argument of type T*");
    }
    explicit owner_ref(pointer p, destructor&& dtor) noexcept requires std::move_constructible<destructor>
        : destructor(std::move(dtor)), m_val(p) {
      static_assert(std::derived_from<T, rc_object>, "Type parameter T of agt::owner_ref<T, ...> must derive from agt::rc_object");
      static_assert(std::invocable<destructor, T*>, "Type parameter Dtor of agt::owner_ref<T, Dtor, ...> must be an object type callable with one argument of type T*");
    }

    ~owner_ref() {
      _free();
    }


    [[nodiscard]] inline pointer   operator->() const noexcept {
      return m_val.get();
    }
    [[nodiscard]] inline reference operator*()  const noexcept {
      return *m_val.get();
    }


    [[nodiscard]] inline pointer   get() const noexcept {
      return m_val._get();
    }

    [[nodiscard]] inline owner_ref clone() const noexcept {
      return owner_ref(m_val._clone());
    }

    inline pointer release() noexcept {
      return static_cast<pointer>(m_val._take());
    }

    inline void    reset(pointer p) noexcept {
      if constexpr (IsTriviallyDestructible)
        m_val._trivial_reset(p);
      else
        m_val.template _reset(p, _dtor());
      // _init();
    }

    inline bool    recycle(rcpool* pool = nullptr) noexcept {
      if (**this) {
        m_val.recycle(pool);
        _init_nonnull();
        return true;
      }
      return false;
    }

    inline void    recycle_or_alloc(rcpool& pool) noexcept {
      if constexpr (IsTriviallyDestructible)
        m_val._trivial_recycle_or_alloc(pool);
      else
        m_val.template _recycle_or_alloc<T>(pool, _dtor());
      _init_nonnull();
    }

    [[nodiscard]] inline explicit operator bool() const noexcept {
      return m_val.get() != nullptr;
    }

  private:

    AGT_forceinline void        _init_nonnull() const noexcept {
      AGT_invariant(**this); // ie. is not null
      if constexpr (!IsTriviallyConstructible) {
        void* mem = m_val._get();
        new (mem) T{ };
      }
    }

    AGT_forceinline void        _init() const noexcept {
      if constexpr (!IsTriviallyConstructible) {
        if (**this)
          _init_nonnull();
      }
    }

    AGT_forceinline destructor& _dtor() noexcept {
      return *this;
    }

    inline void _free() noexcept {
      if constexpr (IsTriviallyDestructible)
        m_val._trivial_release();
      else
        m_val.template _release<T>(_dtor());
    }


    base_type m_val;
  };

  template <typename T, typename Dtor, thread_safety SafetyModel>
  class maybe_ref<T, void, Dtor, SafetyModel> {
    friend owner_ref<T, Dtor, SafetyModel>;
    using base_type = impl::maybe_ref_base<SafetyModel>;
  public:

    using pointer        = T*;
    using reference      = T&;
    using destructor     = Dtor;
    using owner_ref_type = owner_ref<T, Dtor, SafetyModel>;

    maybe_ref() noexcept : m_val(nullptr) { }
    maybe_ref(const owner_ref_type& owner) noexcept
        : m_val(owner.get()) { }

    /*maybe_ref(pointer p) noexcept : m_val(p) { }
    maybe_ref(pointer p, agt_u32_t epoch) noexcept : m_val(std::in_place, p, epoch) { }*/

    maybe_ref(maybe_ref&& other) noexcept
        : m_val(std::in_place,
                std::exchange(other.m_val.ptr(), nullptr),
                other.m_val.epoch())
    { }

    maybe_ref& operator=(maybe_ref&& other) noexcept {
      m_val = std::move(other.m_val);
      // m_val.move_assign(std::move(other.m_val), std::move(other.second()));
      return *this;
    }

    ~maybe_ref() = default;

    [[nodiscard]] inline maybe_ref      clone() const noexcept {
      return m_val.template clone<void>();
    }

    [[nodiscard]] inline owner_ref_type try_acquire() const noexcept {
      return { static_cast<pointer>(m_val.try_acquire_ownership()) };
      // return { static_cast<pointer>(m_val.try_load_ptr()) };
    }

    [[nodiscard]] inline agt_u32_t      epoch() const noexcept {
      return m_val.epoch();
    }

    [[nodiscard]] inline explicit operator bool() const noexcept {
      return m_val.ptr() != nullptr;
    }

  private:

    maybe_ref(base_type&& base) noexcept
        : m_val(std::move(base)) { }

    base_type m_val;
  };

  template <typename T, typename U, typename Dtor, thread_safety SafetyModel>
  class maybe_ref {
    friend owner_ref<T, Dtor, SafetyModel>;
    using base_type = impl::maybe_ref_base<SafetyModel>;
  public:

    using pointer   = T*;
    using reference = T&;
    using destructor = Dtor;
    using owner_ref_type = owner_ref<T, Dtor, SafetyModel>;
    using second_type = U;

    maybe_ref() noexcept requires std::default_initializable<U> : m_val(nullptr, U{}) { }

    maybe_ref(const owner_ref_type& owner) noexcept requires std::default_initializable<U>
        : m_val(owner.get(), U{}) { }
    maybe_ref(const owner_ref_type& owner, const U& val) noexcept requires std::copy_constructible<U>
        : m_val(owner.get(), val) { }
    maybe_ref(const owner_ref_type& owner, U&& val) noexcept requires std::move_constructible<U>
        : m_val(owner.get(), val) { }

    maybe_ref(maybe_ref&& other) noexcept requires std::move_constructible<U>
        : m_val(std::exchange(other.m_val.ptr(), nullptr),
                other.m_val.epoch(),
                std::move(other.second()))
    { }

    maybe_ref& operator=(maybe_ref&& other) noexcept requires std::is_move_assignable_v<U> {
      m_val.move_assign(std::move(other.m_val), std::move(other.second()));
      return *this;
    }

    ~maybe_ref() requires std::is_trivially_destructible_v<U> = default;

    ~maybe_ref() {
      if constexpr (!std::is_trivially_destructible_v<U>)
        m_val.second().~U();
      // destruction of the pointer should be taken care of automatically by the compiler
      // m_val.~impl::maybe_ref_base<IsThreadSafe>();
    }



    [[nodiscard]] inline owner_ref_type     try_acquire() noexcept {
      return { static_cast<pointer>(m_val.try_acquire_ownership()) };
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
    base_type m_val;
  };
}

#endif//AGATE_CORE_TRANSIENT_HPP
