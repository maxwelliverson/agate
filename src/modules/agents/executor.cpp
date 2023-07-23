//
// Created by maxwe on 2022-08-07.
//

#include "executor.hpp"



#include <memory>


#if AGT_system_windows

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>



namespace {


  struct busy_thread_info {
    agt::local_busy_executor* executor;
  };

  struct single_thread_info {};

  struct thread_pool_info {};




  DWORD busy_thread_proc(_In_ LPVOID userData) noexcept {
    std::unique_ptr<busy_thread_info> args{static_cast<busy_thread_info*>(userData)};
    auto executor = args->executor;


  }

  DWORD single_thread_proc(_In_ LPVOID userData) noexcept {
    std::unique_ptr<single_thread_info> args{static_cast<single_thread_info*>(userData)};
  }

  DWORD thread_pool_proc(_In_ LPVOID userData) noexcept {
    std::unique_ptr<thread_pool_info> args{static_cast<thread_pool_info*>(userData)};

    auto tp = CreateThreadpool(nullptr);
    PTP_WORK work;
  }


  void start_busy_thread() {
    SECURITY_ATTRIBUTES securityAttributes{
        .nLength              = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle       = FALSE
    };
    auto threadResult = CreateThread();
  }
}

#else
#endif