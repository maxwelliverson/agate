//
// Created by maxwe on 2022-03-14.
//

#ifndef AGATE_INTERNAL_CONTEXT_SHARED_HPP
#define AGATE_INTERNAL_CONTEXT_SHARED_HPP

#include "fwd.hpp"

namespace agt {

  struct shared_ctx;



  agt_status_t sharedControlBlockInit(shared_ctx* pSharedCtx) noexcept;


  void         sharedControlBlockClose(shared_ctx* pSharedCtx) noexcept;


  bool         isContextAlive(shared_ctx* sharedCtx, context_id id) noexcept;


  void*        lookupLocalAddress(shared_ctx& sharedCtx, shared_handle sharedHandle) noexcept;

  void*        importSharedHandle(shared_ctx& sharedCtx, shared_handle sharedHandle, agt_ctx_t ctx) noexcept;

  void*        shalloc(shared_ctx& sharedCtx, size_t size, size_t alignment, shared_handle& sharedHandle) noexcept;

  agt_status_t claimSharedName(shared_ctx& sharedCtx, const char* pName, size_t nameLength, name_token& token) noexcept;
  void         releaseSharedName(shared_ctx& sharedCtx, name_token nameToken) noexcept;
  void         bindSharedName(shared_ctx& sharedCtx, name_token nameToken, HandleHeader* handle) noexcept;



}

#endif //AGATE_INTERNAL_CONTEXT_SHARED_HPP
