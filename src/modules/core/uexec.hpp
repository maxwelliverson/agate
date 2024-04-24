//
// Created by maxwe on 2024-04-09.
//

#ifndef AGATE_INTERNAL_CORE_UEXEC_HPP
#define AGATE_INTERNAL_CORE_UEXEC_HPP

#include "config.hpp"
#include "core/object.hpp"


AGT_virtual_api_object(agt_uexec_st, uexec, ref_counted) {
  // agt_uexec_flags_t         flags;
  const agt_uexec_vtable_t* vtable;
};

namespace agt {

  void bind_default_uexec(agt_ctx_t ctx) noexcept;

  void unbind_default_uexec(agt_ctx_t ctx) noexcept;

}

#endif//AGATE_INTERNAL_CORE_UEXEC_HPP
