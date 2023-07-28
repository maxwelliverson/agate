//
// Created by maxwe on 2023-07-28.
//


#include "instance.hpp"

#if AGT_system_windows
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif AGT_system_linux
#include <unistd.h>
#endif


#include <utility>



namespace {
  inline agt_u32_t _get_thread_id() noexcept {
#if AGT_system_windows
    return static_cast<agt_u32_t>(GetCurrentThreadId());
#elif AGT_system_linux
    return static_cast<agt_u32_t>(gettid());
#endif
  }
}