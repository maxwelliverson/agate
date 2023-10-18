//
// Created by maxwe on 2023-08-23.
//

#ifndef AGATE_CORE_EXPORTS_HPP
#define AGATE_CORE_EXPORTS_HPP

#include "config.hpp"
#include "export_table.hpp"

namespace agt {
  agt_fiber_transfer_t AGT_stdcall fiber_switch_noexcept(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags) noexcept;
  agt_fiber_transfer_t AGT_stdcall fiber_switch_except(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags) noexcept;
}

#endif//AGATE_CORE_EXPORTS_HPP
