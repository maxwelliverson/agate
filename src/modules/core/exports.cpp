//
// Created by maxwe on 2023-08-23.
//

#include "../module_exports.hpp"
#include "exports.hpp"

// This is an example of how the export selection works.

extern "C" void agatelib_get_exports(agt::init::module_exports& moduleExports, const agt::init::attributes& attributes) noexcept {
  bool exceptionsEnabled = true;
  bool isShared = false;
  bool usingNativeUnit = attributes.using_native_unit();

  if (auto maybeExceptionsEnabled = attributes.get_as<bool>(AGT_ATTR_CXX_EXCEPTIONS_ENABLED))
    exceptionsEnabled = *maybeExceptionsEnabled;
  if (auto maybeIsShared = attributes.get_as<bool>(AGT_ATTR_SHARED_CONTEXT))
    isShared = *maybeIsShared;


  if (exceptionsEnabled) {
    AGT_add_private_export(fiber_init, agt::fiber_init_except);
    AGT_add_public_export(fiber_fork, agt::fiber_fork_except);
    AGT_add_public_export(fiber_loop, agt::fiber_loop_except);
    AGT_add_public_export(fiber_switch, agt::fiber_switch_except);

  }
  else {
    AGT_add_private_export(fiber_init, agt::fiber_init_noexcept);
    AGT_add_public_export(fiber_fork, agt::fiber_fork_noexcept);
    AGT_add_public_export(fiber_loop, agt::fiber_loop_noexcept);
    AGT_add_public_export(fiber_switch, agt::fiber_switch_noexcept);
  }

  if (isShared) {
    AGT_add_private_export(initialize_async, agt::init_async_shared);
    AGT_add_public_export(copy_async, agt::copy_async_shared);
    if (usingNativeUnit)
      AGT_add_public_export(wait, agt::async_wait_native_unit_shared);
    else
      AGT_add_public_export(wait, agt::async_wait_foreign_unit_shared);
  }
  else {
    if (usingNativeUnit)
      AGT_add_public_export(wait, agt::async_wait_native_unit_private);
    else
      AGT_add_public_export(wait, agt::async_wait_foreign_unit_private);
  }

}
