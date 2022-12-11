//
// Created by maxwe on 2022-07-29.
//

#ifndef AGATE_CONTEXT_ERROR_HPP
#define AGATE_CONTEXT_ERROR_HPP

#include "fwd.hpp"

namespace agt {

  namespace err {
    struct internal_overflow {
      object*     obj;
      const char* msg;
      agt_u32_t   fieldBits;
    };
  }

  void raiseError(agt_status_t status, void* errorData, agt_ctx_t context = nullptr) noexcept;

}

#endif//AGATE_CONTEXT_ERROR_HPP
