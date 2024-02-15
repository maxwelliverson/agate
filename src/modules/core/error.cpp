//
// Created by maxwe on 2022-07-29.
//

#include "error.hpp"

#include "ctx.hpp"
#include "instance.hpp"


#include <cstdlib>


void agt::raise(agt_instance_t instance, agt_status_t status, void* errorData) noexcept {
  auto errorHandler = instance->errorHandler;
  if (!errorHandler)
    abort();

  // TODO: Implement actual, library aware abort functions

  switch ((*errorHandler)(status, errorData)) {
    case AGT_ERROR_HANDLED:
      break;
    case AGT_ERROR_IGNORED:
      if (instance_may_ignore_errors(instance))
        break;
    [[fallthrough]];
    case AGT_ERROR_NOT_HANDLED:
      abort();
  }
}

void agt::raise(agt_status_t status, void *errorData, agt_ctx_t context) noexcept {
  if (!context)
    context = get_ctx();
  if (!context)
    abort();

  const auto instance = context->instance;

  raise(instance, status, errorData);
}

