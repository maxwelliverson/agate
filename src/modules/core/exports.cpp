//
// Created by maxwe on 2023-08-23.
//

#include "../module_exports.hpp"
#include "exports.hpp"

// This is an example of how the export selection works.

extern "C" void agatelib_get_exports(agt::init::module_exports& moduleExports, const agt::init::attributes& attributes) noexcept {
  bool exceptionsEnabled = true;
  if (auto maybeExceptionsEnabled = attributes.get_as<bool>(AGT_ATTR_CXX_EXCEPTIONS_ENABLED))
    exceptionsEnabled = *maybeExceptionsEnabled;

  if (exceptionsEnabled)
    AGT_add_export(fiber_switch, agt::fiber_switch_except);
  else
    AGT_add_export(fiber_switch, agt::fiber_switch_noexcept);
}
