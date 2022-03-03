//
// Created by maxwe on 2021-11-25.
//

#ifndef JEMSYS_AGATE2_FWD_HPP
#define JEMSYS_AGATE2_FWD_HPP

#include <agate.h>

extern "C" {

AGT_DEFINE_HANDLE(AgtAsyncData);
AGT_DEFINE_HANDLE(AgtMessageData);
AGT_DEFINE_HANDLE(AgtQueuedMessage);
AGT_DEFINE_HANDLE(AgtMessageChainLink);
AGT_DEFINE_HANDLE(AgtMessageChain);

AGT_DEFINE_HANDLE(AgtSharedMessageChainLink);
AGT_DEFINE_HANDLE(AgtSharedMessageChain);
AGT_DEFINE_HANDLE(AgtSharedMessage);
AGT_DEFINE_HANDLE(AgtLocalMessageChainLink);
AGT_DEFINE_HANDLE(AgtLocalMessageChain);
AGT_DEFINE_HANDLE(AgtLocalMessage);

}

namespace Agt {

  enum class SharedAllocationId : AgtUInt64;

  enum class ObjectType : AgtUInt32;
  enum class ObjectFlags : AgtHandleFlags;
  enum class ConnectAction : AgtUInt32;

  enum class ErrorState : AgtUInt32;


  struct HandleHeader;
  struct SharedObjectHeader;

  class Id;

  class Object;

  class Handle;
  class LocalHandle;
  class SharedHandle;
  class SharedObject;
  using LocalObject = LocalHandle;

  struct LocalChannel;
  struct SharedChannel;

  struct VTable;
  using  VPointer = const VTable*;

  struct SharedVTable;

  using SharedVPtr = const SharedVTable*;


  struct LocalMpScChannel;
  struct SharedMpScChannelSender;
}

#endif//JEMSYS_AGATE2_FWD_HPP
