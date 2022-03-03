//
// Created by maxwe on 2022-02-19.
//

#ifndef JEMSYS_AGATE2_INTERNAL_THREAD_HPP
#define JEMSYS_AGATE2_INTERNAL_THREAD_HPP

#include "../core/objects.hpp"
#include "../fwd.hpp"

namespace Agt {

  namespace Impl {
    struct SystemThread {
      void* handle_;
      int   id_;
    };
  }


  struct Thread : HandleHeader {
    Impl::SystemThread sysThread;
  };

  struct LocalBlockingThread : HandleHeader {
    LocalMpScChannel*  channel;
    Impl::SystemThread sysThread;

    inline constexpr static ObjectType TypeValue = ObjectType::localBlockingThread;
  };

  struct SharedBlockingThread : HandleHeader {
    SharedMpScChannelSender* channel;

    inline constexpr static ObjectType TypeValue = ObjectType::sharedBlockingThread;
  };

  struct ThreadPool : HandleHeader {
    AgtSize             threadCount;
    Impl::SystemThread* pThreads;
  };


  AgtStatus createInstance(LocalBlockingThread*& handle, AgtContext ctx, const AgtBlockingThreadCreateInfo& createInfo) noexcept;
  AgtStatus createInstance(SharedBlockingThread*& handle, AgtContext ctx, const AgtBlockingThreadCreateInfo& createInfo) noexcept;
}

#endif//JEMSYS_AGATE2_INTERNAL_THREAD_HPP
