//
// Created by Maxwell on 2024-02-03.
//

#ifndef AGATE_TESTS_DLL_INTERFACE_HPP
#define AGATE_TESTS_DLL_INTERFACE_HPP


#if defined(AGATE_COMPILING_FIRST_DLL)
# define AGT_first_dll __declspec(dllexport)
#else
# define AGT_first_dll __declspec(dllimport)
#endif

#if defined(AGATE_COMPILING_SECOND_DLL)
# define AGT_second_dll __declspec(dllexport)
#else
# define AGT_second_dll __declspec(dllimport)
#endif

#if defined(AGATE_COMPILING_THIRD_DLL)
# define AGT_third_dll __declspec(dllexport)
#else
# define AGT_third_dll __declspec(dllimport)
#endif

#include "agate/init.h"

namespace dll {

  AGT_first_dll  agt_status_t initFirstLibrary(agt_config_t config, bool allowOverride, agt_ctx_t& firstCtx, agt_ctx_t& thirdCtx) noexcept;

  AGT_first_dll  agt_status_t closeFirstLibrary(agt_ctx_t firstCtx) noexcept;

  AGT_second_dll agt_status_t initSecondLibrary(agt_config_t config, agt_ctx_t& secondCtx) noexcept;

  AGT_second_dll  agt_status_t closeSecondLibrary(agt_ctx_t firstCtx) noexcept;

  AGT_third_dll  agt_status_t initThirdLibrary(agt_config_t config, agt_ctx_t& thirdCtx) noexcept;

  AGT_third_dll  agt_status_t closeThirdLibrary(agt_ctx_t firstCtx) noexcept;

}

#endif //AGATE_TESTS_DLL_INTERFACE_HPP
