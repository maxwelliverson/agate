//
// Created by Maxwell on 2024-02-03.
//

#include "dll_interface.hpp"

#include <algorithm>


AGT_third_dll agt_status_t dll::initThirdLibrary(agt_config_t parent, agt_ctx_t& secondCtx) noexcept {
  /*if (auto config = agt_get_config(parent)) {
    agt_config_option_t options[] = {
      {
        .id = AGT_CONFIG_THREAD_COUNT,
        .flags = (allowOverride ? AGT_ALLOW_SUBMODULE_OVERRIDE : 0u) | AGT_VALUE_GREATER_THAN_OR_EQUALS,
        .necessity = AGT_INIT_REQUIRED,
        .type = AGT_TYPE_UINT32,
        .value = {
          .uint32 = 2
        }
      },
      {
        .id = AGT_CONFIG_DEFAULT_FIBER_STACK_SIZE,
        .flags = (allowOverride ? AGT_ALLOW_SUBMODULE_OVERRIDE : 0u),
        .necessity = AGT_INIT_OPTIONAL,
        .type = AGT_TYPE_UINT64,
        .value = {
          .uint64 = (0x1 << 20)
        }
      },
      {
        .id = AGT_CONFIG_SHARED_NAMESPACE,
        .flags = allowOverride ? AGT_ALLOW_SUBMODULE_OVERRIDE : 0u,
        .necessity = AGT_INIT_HINT,
        .type = AGT_TYPE_STRING,
        .value = {
          .string = "firstDllNamespace"
        }
      }
    };

    agt_config_set_options(config, std::size(options), options);

    auto thirdResult = dll::initThirdLibrary(config, thirdCtx);

    auto result = agt_init(&firstCtx, config);

    return result;
  }

  return AGT_SUCCESS;*/

  return agt_init(&secondCtx, agt_get_config(parent));
}

AGT_third_dll agt_status_t dll::closeThirdLibrary(agt_ctx_t firstCtx) noexcept {
  return agt_finalize(firstCtx);
}