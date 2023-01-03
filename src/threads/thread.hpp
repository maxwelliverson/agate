//
// Created by maxwe on 2022-02-19.
//

#ifndef JEMSYS_AGATE2_INTERNAL_THREAD_HPP
#define JEMSYS_AGATE2_INTERNAL_THREAD_HPP

#include "../core/object.hpp"
#include "../fwd.hpp"

namespace agt {

  agt_u32_t currentThreadId() noexcept;

  /*namespace impl {
    struct system_thread {
      void* handle_;
      int   id_;
    };
  }


  struct thread : HandleHeader {
    Impl::SystemThread sysThread;
  };

  struct local_blocking_thread : handle_header {
    local_mpsc_channel* channel;
    impl::system_thread sysThread;

    inline constexpr static object_type type_value = object_type::localBlockingThread;
  };

  struct shared_blocking_thread : handle_header {
    shared_mpsc_channel_sender* channel;

    inline constexpr static object_type type_value = object_type::sharedBlockingThread;
  };

  struct thread_pool : handle_header {
    size_t               threadCount;
    impl::system_thread* pThreads;
  };


  agt_status_t createInstance(local_blocking_thread*& handle, agt_ctx_t ctx, const agt_blocking_thread_create_info_t& createInfo) noexcept;
  agt_status_t createInstance(shared_blocking_thread*& handle, agt_ctx_t ctx, const agt_blocking_thread_create_info_t& createInfo) noexcept;*/
}

#endif//JEMSYS_AGATE2_INTERNAL_THREAD_HPP
