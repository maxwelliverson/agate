//
// Created by maxwe on 2021-11-25.
//

#ifndef JEMSYS_AGATE2_CONTEXT_HPP
#define JEMSYS_AGATE2_CONTEXT_HPP

#include "fwd.hpp"


namespace agt {



  enum class builtin_value : agt_u32_t {
    processName,
    defaultPrivateChannelMessageSize,
    defaultLocalChannelMessageSize,
    defaultSharedChannelMessageSize,
    defaultPrivateChannelSlotCount,
    defaultLocalChannelSlotCount,
    defaultSharedChannelSlotCount,
    asyncStructSize,
    signalStructSize
  };


  void*        ctxAllocPages(agt_ctx_t ctx, size_t totalSize) noexcept;
  void         ctxFreePages(agt_ctx_t ctx, void* addr, size_t totalSize) noexcept;



  object*       ctxAllocObject32(agt_ctx_t ctx)  noexcept;
  object*       ctxAllocObject64(agt_ctx_t ctx)  noexcept;
  object*       ctxAllocObject128(agt_ctx_t ctx) noexcept;
  object*       ctxAllocObject256(agt_ctx_t ctx) noexcept;



  context_id    ctxId(agt_ctx_t ctx) noexcept;

  bool          ctxMayIgnoreErrors(agt_ctx_t ctx) noexcept;


  void*         ctxLocalAlloc(agt_ctx_t ctx, size_t size, size_t alignment) noexcept;
  void          ctxLocalFree(agt_ctx_t ctx, void* memory, size_t size, size_t alignment) noexcept;

  void*         ctxSharedAlloc(agt_ctx_t ctx, size_t size, size_t alignment, shared_handle& allocationId) noexcept;
  void          ctxSharedFree(agt_ctx_t ctx, shared_handle allocId) noexcept;
  void          ctxSharedFree(agt_ctx_t ctx, void* memory, size_t size, size_t alignment) noexcept;

  HandleHeader* ctxAllocHandle(agt_ctx_t ctx, size_t size, size_t alignment) noexcept;
  void          ctxFreeHandle(agt_ctx_t ctx, HandleHeader* handle, size_t size, size_t alignment) noexcept;

  SharedObjectHeader* ctxAllocSharedObject(agt_ctx_t context, size_t size, size_t alignment) noexcept;
  void                ctxFreeSharedObject(agt_ctx_t ctx, SharedObjectHeader* object, size_t size, size_t alignment) noexcept;


  shared_handle ctxExportSharedHandle(agt_ctx_t ctx, const void* sharedObject) noexcept;
  void*         ctxImportSharedHandle(agt_ctx_t ctx, shared_handle sharedHandle) noexcept;



  agt_status_t ctxOpenHandleById(agt_ctx_t ctx, agt_object_id_t id, HandleHeader*& handle) noexcept;
  agt_status_t ctxOpenHandleByName(agt_ctx_t ctx, const char* name, HandleHeader*& handle) noexcept;




  agt_status_t ctxClaimLocalName(agt_ctx_t ctx, const char* pName, size_t nameLength, name_token& token) noexcept;
  void         ctxReleaseLocalName(agt_ctx_t ctx, name_token nameToken) noexcept;
  agt_status_t ctxClaimSharedName(agt_ctx_t ctx, const char* pName, size_t nameLength, name_token& token) noexcept;
  void         ctxReleaseSharedName(agt_ctx_t ctx, name_token nameToken) noexcept;
  void         ctxBindName(agt_ctx_t ctx, name_token nameToken, HandleHeader* handle) noexcept;

  agt_status_t ctxEnumerateNamedObjects(agt_ctx_t ctx, size_t& count, const char** pNames) noexcept;
  agt_status_t ctxEnumerateSharedObjects(agt_ctx_t ctx, size_t& count, const char** pNames) noexcept;



  VPointer      ctxLookupVTable(agt_ctx_t ctx, agt_type_id_t typeId) noexcept;

  // bool          ctxSetBuiltinValue(agt_ctx_t ctx, builtin_value value, const void* data, size_t dataSize) noexcept;
  // bool          ctxGetBuiltinValue(agt_ctx_t ctx, builtin_value value, void* data, size_t& outSize) noexcept;

  uintptr_t        ctxGetBuiltin(agt_ctx_t ctx, builtin_value value) noexcept;


  agt_status_t  createCtx(agt_ctx_t& pCtx) noexcept;
  void          destroyCtx(agt_ctx_t ctx) noexcept;


  void*         ctxAcquireLocalAsyncData(agt_ctx_t ctx) noexcept;
  void          ctxReleaseLocalAsyncData(agt_ctx_t ctx, void* data) noexcept;

  void*         ctxAcquireSharedAsyncData(agt_ctx_t ctx, async_data_t& handle) noexcept;
  void*         ctxMapSharedAsyncData(agt_ctx_t ctx, async_data_t handle) noexcept;
  void          ctxReleaseSharedAsyncData(agt_ctx_t ctx, async_data_t handle) noexcept;

}

#endif//JEMSYS_AGATE2_CONTEXT_HPP
