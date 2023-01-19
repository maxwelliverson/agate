//
// Created by maxwe on 2022-07-29.
//

#include "error.hpp"

#include "instance.hpp"
#include "modules/agents/state.hpp"


#include <cstdlib>


void agt::raiseError(agt_status_t status, void *errorData, agt_ctx_t context) noexcept {
  if (!context)
    context = tl_state.context;
  if (!context)
    abort();

  auto errorHandler = agt_get_error_handler(context);
  if (!errorHandler)
    abort();

  // TODO: Implement actual, library aware abort functions

  switch ((*errorHandler)(status, errorData)) {
    case AGT_ERROR_HANDLED:
      break;
    case AGT_ERROR_IGNORED:
      if (ctxMayIgnoreErrors(context))
        break;
      [[fallthrough]];
    case AGT_ERROR_NOT_HANDLED:
      abort();
  }
}