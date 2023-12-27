//
// Created by maxwe on 2022-03-14.
//

#ifndef AGATE_INTERNAL_CONTEXT_SHARED_HPP
#define AGATE_INTERNAL_CONTEXT_SHARED_HPP

#include "config.hpp"

namespace agt {

  struct shared_ctx;


  struct shared_cb;
  struct shared_cache;

  struct tmem_cb; // Transient memory control block; is a normal shared allocation



  enum class shared_mem  : agt_u64_t; // Normal allocation; does not move, may be indefinitely cached.
  enum class shared_tmem : agt_u64_t; // Transient memory;  may move and/or change size, may not be indefinitely cached

  struct imported_transient_memory {
    void*       localAddress;
    agt_u32_t   epoch;
    agt_u32_t   flags;
    tmem_cb*    cb;
    shared_tmem handle;
  };


  struct shared_allocator;



  struct shared_allocation {
    agt_shmem_t handle;
    void*       localAddress;
  };



  void         alloc_tmem(agt_ctx_t ctx, imported_transient_memory* memory, size_t size, size_t alignment) noexcept;



  void         free_tmem(agt_ctx_t ctx, imported_transient_memory* memory) noexcept;


  void*        map_tmem(agt_ctx_t ctx, imported_transient_memory* memory) noexcept;

  void         unmap_tmem(agt_ctx_t ctx, imported_transient_memory* memory) noexcept;


  agt_status_t sharedControlBlockInit(shared_ctx* pSharedCtx) noexcept;


  void         sharedControlBlockClose(shared_ctx* pSharedCtx) noexcept;


  bool         isContextAlive(shared_ctx* sharedCtx, context_id id) noexcept;


  void*        lookupLocalAddress(shared_ctx& sharedCtx, shared_handle sharedHandle) noexcept;

  void*        importSharedHandle(shared_ctx& sharedCtx, shared_handle sharedHandle, agt_ctx_t ctx) noexcept;

  void*        shalloc(shared_ctx& sharedCtx, size_t size, size_t alignment, shared_handle& sharedHandle) noexcept;

  agt_status_t claim_shared_name(shared_ctx& sharedCtx, const char* pName, size_t nameLength, name_token& token) noexcept;
  void         release_shared_name(shared_ctx& sharedCtx, name_token nameToken) noexcept;
  void         bind_shared_name(shared_ctx& sharedCtx, name_token nameToken, HandleHeader* handle) noexcept;



}

#endif //AGATE_INTERNAL_CONTEXT_SHARED_HPP
