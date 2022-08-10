//
// Created by maxwe on 2022-07-29.
//

#ifndef AGATE_CONTEXT_ERROR_HPP
#define AGATE_CONTEXT_ERROR_HPP

#include "agate.h"

namespace agt {

  void raiseError(agt_status_t status, void* errorData, agt_ctx_t context = nullptr) noexcept;

}

#endif//AGATE_CONTEXT_ERROR_HPP
