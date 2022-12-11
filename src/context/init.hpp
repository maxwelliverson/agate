//
// Created by maxwe on 2022-08-17.
//

#ifndef AGATE_INTERNAL_CONTEXT_INIT_HPP
#define AGATE_INTERNAL_CONTEXT_INIT_HPP

#include "fwd.hpp"

#include "support/vector.hpp"

namespace agt {
  struct ctx_init_options {
    agt_i32_t    apiHeaderVersion;
    agt_u32_t    asyncStructSize;
    agt_u32_t    signalStructSize;
    bool         sharedLibIsEnabled;
    vector<char> sharedCtxNamespace;
    agt_u32_t    channelDefaultCapacity;
    agt_u32_t    channelDefaultMessageSize;
    agt_u32_t    defaultTimeoutMs;
  };

  void getInitOptions(ctx_init_options& opt, int headerVersion) noexcept;
}

#endif//AGATE_INTERNAL_CONTEXT_INIT_HPP
