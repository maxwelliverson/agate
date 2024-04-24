//
// Created by maxwe on 2024-04-15.
//

#ifndef AGATE_INTERNAL_CORE_HAZPTR_HPP
#define AGATE_INTERNAL_CORE_HAZPTR_HPP

#include "config.hpp"
#include "impl/hazptr_def.hpp"
#include "agate/cast.hpp"

namespace agt {

  namespace impl {
    void do_retire_hazard(agt_ctx_t ctx, hazard_obj* hazard) noexcept;

    template <typename ObjectType>
    inline void reclaim_callback(agt_ctx_t ctx, hazard_obj* obj, hazard_obj_sized_list& children) noexcept {
      if constexpr (impl::has_adl_destroy<ObjectType, agt_ctx_t, hazard_obj_sized_list&>)
        release(unsafe_nonnull_object_cast<ObjectType>(obj), ctx, children);
      else if constexpr (impl::has_adl_destroy<ObjectType, agt_ctx_t>)
        release(unsafe_nonnull_object_cast<ObjectType>(obj), ctx);
      else if constexpr (impl::has_adl_destroy<ObjectType, hazard_obj_sized_list&>)
        release(unsafe_nonnull_object_cast<ObjectType>(obj), children);
      else
        release(unsafe_nonnull_object_cast<ObjectType>(obj));
    }
  }


  agt_hazptr_t make_hazptr(agt_ctx_t ctx) noexcept;

  void         make_hazptrs(agt_ctx_t ctx, std::span<agt_hazptr_t> hazptrs) noexcept;

  void         drop_hazptr(agt_ctx_t ctx, agt_hazptr_t hazptr) noexcept;

  void         drop_hazptrs(agt_ctx_t ctx, std::span<const agt_hazptr_t> hazptrs) noexcept;

  AGT_forceinline static void set_hazptr(agt_hazptr_t hazptr, const void* ptr) noexcept {
    atomic_store(hazptr->ptr, ptr);
  }

  AGT_forceinline static void reset_hazptr(agt_hazptr_t hazptr) noexcept {
    set_hazptr(hazptr, nullptr);
  }

  template <std::derived_from<hazard_obj> T>
  inline static bool try_protect(agt_hazptr_t hazptr, T*& value, T* const & hazard) noexcept {
    T* prev = value;
    set_hazptr(hazptr, prev);
    atomic_store(hazptr->ptr, prev);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    value = atomic_load(hazard);
    if (prev == value) [[unlikely]] {
      reset_hazptr(hazptr);
      return false;
    }
    return true;
  }

  template <std::derived_from<hazard_obj> T>
  inline static T*   protect(agt_hazptr_t hazptr, T* const & hazard) noexcept {
    auto value = atomic_relaxed_load(hazard);
    while (!try_protect(hazptr, value, hazard));
    return value;
  }




  template <std::derived_from<hazard_obj> ObjectType>
  inline static void retire(agt_ctx_t ctx, ObjectType* obj) noexcept {
    AGT_invariant( obj != nullptr );
    obj->reclaimFunc = &impl::reclaim_callback<ObjectType>;
    obj->nextHazptrObj = nullptr;
    obj->domainTag = 0;
    impl::do_retire_hazard(ctx, obj);
  }


  // Used on the stack for automatic cleanup/reuse
  class hazard_pointer {
    agt_ctx_t ctx;
    agt_hazptr_t hazptr = nullptr;
  public:
    explicit hazard_pointer(agt_ctx_t ctx) noexcept : ctx(ctx), hazptr(nullptr) {}

    ~hazard_pointer() {
      drop_hazptr(ctx, hazptr);
    }

    void init() noexcept {
      if (!hazptr)
        hazptr = make_hazptr(ctx);
    }

    void set(const void* ptr) noexcept {
      set_hazptr(hazptr, ptr);
    }

    void reset() noexcept {
      reset_hazptr(hazptr);
    }

    template <std::derived_from<hazard_obj> T>
    bool try_protect(T*& value, T* const & hazard) noexcept {
      return agt::try_protect(hazptr, value, hazard);
    }

    template <std::derived_from<hazard_obj> T>
    T* protect(T* const & hazard) noexcept {
      return agt::protect(hazptr, hazard);
    }
  };


  // This should only be used on the stack, eg. to accumulate entries for retiring while enumerating over a list
  class hazard_retire_list {
    agt_ctx_t             m_ctx;
    agt_hazptr_domain_t   m_domain;
    hazard_obj_sized_list m_list;
  public:
    explicit hazard_retire_list(agt_ctx_t ctx, agt_hazptr_domain_t domain = AGT_DEFAULT_HAZPTR_DOMAIN) noexcept
        : m_ctx(ctx), m_domain(domain), m_list{} { }

    ~hazard_retire_list() {
      flush();
    }

    template <std::derived_from<hazard_obj> ObjectType>
    inline void push(ObjectType* obj) noexcept {
      AGT_invariant( obj != nullptr );
      obj->reclaimFunc   = &impl::reclaim_callback<ObjectType>;
      obj->domainTag     = reinterpret_cast<uintptr_t>(m_domain);
      agt::push(m_list, obj);
    }


    // Useful if the executing ctx might change over the course of whatever algorithm is running
    void set_ctx(agt_ctx_t ctx) noexcept {
      m_ctx = ctx;
    }

    void flush() noexcept;
  };

  void         retire_user_hazard(agt_ctx_t ctx, void* obj, agt_hazptr_domain_t domain, agt_hazptr_retire_func_t retireFunc, void* userData) noexcept;
}

#endif//AGATE_INTERNAL_CORE_HAZPTR_HPP
