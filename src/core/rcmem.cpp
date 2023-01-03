//
// Created by maxwe on 2022-12-22.
//

#include "rc.hpp"
#include "rcpool.hpp"


#define AGT_return_type void*

#define AGT_exported_function_name rc_alloc

template <agt::thread_safety SafetyModel>
AGT_forceinline static void* rc_user_alloc(agt_rcpool_t pool, agt_u32_t initialRefCount) noexcept {
  AGT_invariant(initialRefCount > 0);
  AGT_invariant(pool != nullptr);
  return agt::impl::_cast_user_address_from_rc(agt::alloc_rc_object<SafetyModel>((agt::rcpool*)pool, initialRefCount));
}

AGT_export_family {

    AGT_function_entry(st)(agt_rcpool_t pool, agt_u32_t initialRefCount) {
      return rc_user_alloc<agt::thread_unsafe>(pool, initialRefCount);
    }

    AGT_function_entry(mt)(agt_rcpool_t pool, agt_u32_t initialRefCount) {
      return rc_user_alloc<agt::thread_user_safe>(pool, initialRefCount);
    }

}


#undef AGT_exported_function_name
#define AGT_exported_function_name rc_recycle

template <agt::thread_safety SafetyModel>
AGT_forceinline static void* rc_user_recycle(void* allocation, agt_u32_t releasedCount, agt_u32_t acquiredCount) noexcept {
    AGT_invariant(releasedCount > 0);
    AGT_invariant(acquiredCount > 0);
    AGT_invariant(allocation != nullptr);
    AGT_invariant(agt::impl::_is_user_rc_alloc(allocation));

    auto rc = agt::impl::_cast_rc_from_user_address(allocation);

    auto result = agt::recycle_rc_object<SafetyModel>(*rc, releasedCount, acquiredCount);

    return result ? agt::impl::_cast_user_address_from_rc(result) : nullptr;
}

AGT_export_family {

    AGT_function_entry(st)(void* allocation, agt_u32_t releasedCount, agt_u32_t acquiredCount) {
      return rc_user_recycle<agt::thread_unsafe>(allocation, releasedCount, acquiredCount);
    }

    AGT_function_entry(mt)(void* allocation, agt_u32_t releasedCount, agt_u32_t acquiredCount) {
      return rc_user_recycle<agt::thread_user_safe>(allocation, releasedCount, acquiredCount);
    }

}



#undef AGT_exported_function_name
#define AGT_exported_function_name rc_retain

template <bool ThreadsEnabled>
AGT_forceinline static void* rc_user_retain(void* allocation, agt_u32_t count) noexcept {
    AGT_invariant(count > 0);
    AGT_invariant(allocation != nullptr);
    AGT_invariant(agt::impl::_is_user_rc_alloc(allocation));

    auto rc = agt::impl::_cast_rc_from_user_address(allocation);

    agt::retain_rc_object<ThreadsEnabled ? agt::thread_user_safe : agt::thread_unsafe>(*rc, count);

    return allocation;
}


AGT_export_family {

    AGT_function_entry(st)(void* allocation, agt_u32_t count) {
      return rc_user_retain<false>(allocation, count);
    }

    AGT_function_entry(mt)(void* allocation, agt_u32_t count) {
      return rc_user_retain<true>(allocation, count);
    }

}


#undef AGT_return_type
#define AGT_return_type void
#undef AGT_exported_function_name
#define AGT_exported_function_name rc_release

template <bool ThreadsEnabled>
AGT_forceinline static void rc_user_release(void* allocation, agt_u32_t count) noexcept {
    AGT_invariant(count > 0);

    if (allocation) {
      auto rc = agt::impl::_cast_rc_from_user_address(allocation);

      AGT_invariant(agt::impl::_is_user_rc_alloc(rc));

      agt::release_rc_object<ThreadsEnabled ? agt::thread_user_safe : agt::thread_unsafe>(*rc, count);
    }
}

AGT_export_family {

    AGT_function_entry(st)(void* allocation, agt_u32_t count) {
      rc_user_release<false>(allocation, count);
    }

    AGT_function_entry(mt)(void* allocation, agt_u32_t count) {
      rc_user_release<true>(allocation, count);
    }

}