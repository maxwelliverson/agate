//
// Created by maxwe on 2021-11-25.
//

#ifndef JEMSYS_AGATE2_CONTEXT_HPP
#define JEMSYS_AGATE2_CONTEXT_HPP

#include "fwd.hpp"


namespace Agt {

  enum class NameToken : AgtUInt64;

  enum class BuiltinValue : AgtUInt32 {
    processName,
    defaultPrivateChannelMessageSize,
    defaultLocalChannelMessageSize,
    defaultSharedChannelMessageSize,
    defaultPrivateChannelSlotCount,
    defaultLocalChannelSlotCount,
    defaultSharedChannelSlotCount
  };


  void*         ctxLocalAlloc(AgtContext ctx, AgtSize size, AgtSize alignment) noexcept;
  void          ctxLocalFree(AgtContext ctx, void* memory, AgtSize size, AgtSize alignment) noexcept;

  void*         ctxSharedAlloc(AgtContext ctx, AgtSize size, AgtSize alignment, SharedAllocationId& allocationId) noexcept;
  void          ctxSharedFree(AgtContext ctx, SharedAllocationId allocId) noexcept;
  void          ctxSharedFree(AgtContext ctx, void* memory, AgtSize size, AgtSize alignment) noexcept;

  HandleHeader* ctxAllocHandle(AgtContext ctx, AgtSize size, AgtSize alignment) noexcept;
  void          ctxFreeHandle(AgtContext ctx, HandleHeader* handle, AgtSize size, AgtSize alignment) noexcept;

  SharedObjectHeader* ctxAllocSharedObject(AgtContext context, AgtSize size, AgtSize alignment) noexcept;
  void                ctxFreeSharedObject(AgtContext ctx, SharedObjectHeader* object, AgtSize size, AgtSize alignment) noexcept;

  // void*         ctxAllocAsyncData(AgtContext context, AgtObjectId& id) noexcept;
  // void          ctxFreeAsyncData(AgtContext context, void* memory) noexcept;




  void*         ctxGetLocalAddress(SharedAllocationId allocId) noexcept;

  AgtStatus     ctxOpenHandleById(AgtContext ctx, AgtObjectId id, HandleHeader*& handle) noexcept;
  AgtStatus     ctxOpenHandleByName(AgtContext ctx, const char* name, HandleHeader*& handle) noexcept;




  AgtStatus     ctxClaimLocalName(AgtContext ctx, const char* pName, AgtSize nameLength, NameToken& token) noexcept;
  void          ctxReleaseLocalName(AgtContext ctx, NameToken nameToken) noexcept;
  AgtStatus     ctxClaimSharedName(AgtContext ctx, const char* pName, AgtSize nameLength, NameToken& token) noexcept;
  void          ctxReleaseSharedName(AgtContext ctx, NameToken nameToken) noexcept;
  void          ctxBindName(AgtContext ctx, NameToken nameToken, HandleHeader* handle) noexcept;

  AgtStatus     ctxEnumerateNamedObjects(AgtContext ctx, AgtSize& count, const char** pNames) noexcept;
  AgtStatus     ctxEnumerateSharedObjects(AgtContext ctx, AgtSize& count, const char** pNames) noexcept;



  VPointer      ctxLookupVTable(AgtContext ctx, AgtTypeId typeId) noexcept;

  bool          ctxSetBuiltinValue(AgtContext ctx, BuiltinValue value, const void* data, AgtSize dataSize) noexcept;
  bool          ctxGetBuiltinValue(AgtContext ctx, BuiltinValue value, void* data, AgtSize& outSize) noexcept;

  AgtStatus     createCtx(AgtContext& pCtx) noexcept;
  void          destroyCtx(AgtContext ctx) noexcept;




}

#endif//JEMSYS_AGATE2_CONTEXT_HPP
