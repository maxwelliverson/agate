//
// Created by maxwe on 2022-12-22.
//


#include "rc.hpp"
#include "rcpool.hpp"


#define AGT_return_type agt_weak_ref_t
#define AGT_exported_function_name weak_ref_take


template <agt::thread_safety SafetyModel>
AGT_forceinline static agt_weak_ref_t take_user_rc_weak_ref(void* rcObj, agt_epoch_t* pEpoch, agt_u32_t count) noexcept {
  AGT_invariant(pEpoch != nullptr);
  AGT_invariant(count > 0);
  AGT_invariant(rcObj != nullptr);

  using namespace agt::impl;

  auto rc = _cast_rc_from_user_address(rcObj);

  AGT_invariant(_is_user_rc_alloc(*rc));

  agt::take_rc_weak_ref<SafetyModel>(*rc, *pEpoch, count);

  return _cast_weak_ref_from_rc(rc);
}

AGT_export_family {

  AGT_function_entry(st)(void* rcObj, agt_epoch_t* pEpoch, agt_u32_t count) {
    return take_user_rc_weak_ref<agt::thread_unsafe>(rcObj, pEpoch, count);
  }

  AGT_function_entry(mt)(void* rcObj, agt_epoch_t* pEpoch, agt_u32_t count) {
    return take_user_rc_weak_ref<agt::thread_user_safe>(rcObj, pEpoch, count);
  }

}



#undef AGT_exported_function_name
#define AGT_exported_function_name weak_ref_retain


template <agt::thread_safety SafetyModel>
AGT_forceinline static agt_weak_ref_t retain_user_rc_weak_ref(agt_weak_ref_t ref, agt_epoch_t epoch, agt_u32_t count) noexcept {
  AGT_invariant(count > 0);
  AGT_invariant(ref != AGT_NULL_WEAK_REF);

  using namespace agt::impl;

  auto rc = _cast_rc_from_weak_ref(ref);

  if (!agt::retain_rc_weak_ref<SafetyModel>(*rc, epoch, count)) {
    agt::drop_rc_weak_ref<SafetyModel>(*rc, 1);
    return AGT_NULL_WEAK_REF;
  }

  return ref;
}

AGT_export_family {

  AGT_function_entry(st)(agt_weak_ref_t ref, agt_epoch_t epoch, agt_u32_t count) {
    return retain_user_rc_weak_ref<agt::thread_unsafe>(ref, epoch, count);
  }

  AGT_function_entry(mt)(agt_weak_ref_t ref, agt_epoch_t epoch, agt_u32_t count) {
    return retain_user_rc_weak_ref<agt::thread_user_safe>(ref, epoch, count);
  }

}


#undef AGT_return_type
#define AGT_return_type void
#undef AGT_exported_function_name
#define AGT_exported_function_name weak_ref_drop

template <agt::thread_safety SafetyModel>
AGT_forceinline static void drop_user_rc_weak_ref(agt_weak_ref_t ref, agt_u32_t count) noexcept {
  AGT_invariant(count > 0);
  AGT_invariant(ref != AGT_NULL_WEAK_REF);

  using namespace agt::impl;


  auto rc = _cast_rc_from_weak_ref(ref);

  agt::drop_rc_weak_ref<SafetyModel>(*rc, count);
}

AGT_export_family {

  AGT_function_entry(st)(agt_weak_ref_t ref, agt_u32_t count) {
    drop_user_rc_weak_ref<agt::thread_unsafe>(ref, count);
  }

  AGT_function_entry(mt)(agt_weak_ref_t ref, agt_u32_t count) {
    drop_user_rc_weak_ref<agt::thread_user_safe>(ref, count);
  }

}



#undef AGT_return_type
#define AGT_return_type void*
#undef AGT_exported_function_name
#define AGT_exported_function_name acquire_from_weak_ref

template <agt::thread_safety SafetyModel>
AGT_forceinline static void* acquire_from_user_rc_weak_ref(agt_weak_ref_t token, agt_epoch_t epoch, agt_weak_ref_flags_t flags) noexcept {
  if (token == AGT_NULL_WEAK_REF)
    return nullptr;

  using namespace agt;

  auto rc = impl::_cast_rc_from_weak_ref(token);

  rc_object* result;

  // Always drop weak ref UNLESS successfully acquires ownership AND flags contains the AGT_WEAK_REF_RETAIN_IF_ACQUIRED bit
  if (!((result = acquire_ownership_of_weak_ref<SafetyModel>(*rc, epoch)) && (flags & AGT_WEAK_REF_RETAIN_IF_ACQUIRED))) {
    drop_rc_weak_ref<SafetyModel>(*rc, 1);
  }

  return result ? impl::_cast_user_address_from_rc(result) : nullptr;
}

AGT_export_family {

  AGT_function_entry(st)(agt_weak_ref_t token, agt_epoch_t epoch, agt_weak_ref_flags_t flags) {
    return acquire_from_user_rc_weak_ref<agt::thread_unsafe>(token, epoch, flags);
  }

  AGT_function_entry(mt)(agt_weak_ref_t token, agt_epoch_t epoch, agt_weak_ref_flags_t flags) {
    return acquire_from_user_rc_weak_ref<agt::thread_user_safe>(token, epoch, flags);
  }

}