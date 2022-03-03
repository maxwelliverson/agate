//
// Created by maxwe on 2021-11-22.
//

#ifndef JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
#define JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP

#include "fwd.hpp"

#include "context/context.hpp"
#include "support/atomic.hpp"
#include "support/flags.hpp"
#include "vtable.hpp"

namespace Agt {

  enum class ObjectType : AgtUInt32 {
    localMpScChannel,
    localMpMcChannel,
    localSpMcChannel,
    localSpScChannel,
    localMpScChannelSender,
    localMpMcChannelSender,
    localSpMcChannelSender,
    localSpScChannelSender,
    localMpScChannelReceiver,
    localMpMcChannelReceiver,
    localSpMcChannelReceiver,
    localSpScChannelReceiver,
    sharedMpScChannel,
    sharedMpMcChannel,
    sharedSpMcChannel,
    sharedSpScChannel,
    sharedMpScChannelHandle,
    sharedMpMcChannelHandle,
    sharedSpMcChannelHandle,
    sharedSpScChannelHandle,
    sharedMpScChannelSender,
    sharedMpMcChannelSender,
    sharedSpMcChannelSender,
    sharedSpScChannelSender,
    sharedMpScChannelReceiver,
    sharedMpMcChannelReceiver,
    sharedSpMcChannelReceiver,
    sharedSpScChannelReceiver,
    agent,
    socket,
    channelSender,
    channelReceiver,
    thread,
    agency,
    localAsyncData,
    sharedAsyncData,
    privateChannel,
    localBlockingThread,
    sharedBlockingThread
  };

  enum class ConnectAction : AgtUInt32 {
    connectSender,
    disconnectSender,
    connectReceiver,
    disconnectReceiver
  };

  AGT_forceinline static AgtHandleType toHandleType(ObjectType type) noexcept;

  AGT_BITFLAG_ENUM(ObjectFlags, AgtHandleFlags) {
    isShared                  = 0x01,
    isBuiltinType             = 0x02,
    isSender                  = 0x04,
    isReceiver                = 0x08,
    isUnderlyingType          = 0x10,
    supportsOutOfLineMsg      = 0x10000,
    supportsMultiFrameMsg     = 0x20000,
    supportsExternalMsgMemory = 0x40000
  };

  constexpr static auto getHandleFlagMask() noexcept {
    if constexpr (AGT_HANDLE_FLAGS_MAX > 0x1000'0001) {
      // 64 bit path
      if (AGT_HANDLE_FLAGS_MAX == 0x1000'0000'0000'0001ULL)
        return static_cast<AgtUInt64>(-1);
      return AgtUInt64 ((AGT_HANDLE_FLAGS_MAX - 1) << 1) - 1ULL;
    }
    else {
      // 32 bit path
      if (AGT_HANDLE_FLAGS_MAX == 0x1000'0001)
        return static_cast<AgtUInt32>(-1);
      return AgtUInt32 ((AGT_HANDLE_FLAGS_MAX - 1) << 1) - 1;
    }
  }

  AGT_forceinline static AgtHandleFlags toHandleFlags(ObjectFlags flags) noexcept {
    constexpr static auto Mask = getHandleFlagMask();
    return static_cast<AgtHandleFlags>(flags) & Mask;
  }


  class ProxyObject {
    // The reserved member field is here to enable the spoofing of Handles as objects (Handles are virtual, while general objects are not).
    // Note that while this relies on technically undefined behaviour, the placement of the virtual pointer at the start of objects has been
    // the the behaviour of the three major C++ compilers for long enough that I feel comfortable taking advantage of that particular implementation detail.
    void* reserved;
    AgtObjectId id;
    ObjectType  type;
    ObjectFlags flags;

  public:

    AGT_nodiscard AGT_forceinline AgtObjectId getId() const noexcept {
      return id;
    }
    AGT_nodiscard AGT_forceinline ObjectType  getType() const noexcept {
      return type;
    }
    AGT_nodiscard AGT_forceinline ObjectFlags getFlags() const noexcept {
      return flags;
    }
  };


  struct SharedObjectHeader {
    AgtSize     objectSize;
    AgtObjectId id;
    ObjectType  type;
    ObjectFlags flags;
  };

  struct HandleHeader {
    VPointer    vptr;
    AgtObjectId id;
    ObjectType  type;
    ObjectFlags flags;
    AgtContext  context;
  };

  struct SharedHandleHeader : HandleHeader {
    SharedObjectHeader* sharedInstance;
  };


  /*class Handle {

  protected:

    AgtObjectId id;
    ObjectType  type;
    ObjectFlags flags;
    AgtContext  context;



    virtual AgtStatus acquire() noexcept = 0;
    // Releases a single handle reference, and if the reference count has dropped to zero, destroys the object.
    // By delegating responsibility of destruction to this function, a redundant virtual call is saved on the fast path
    // The object reference on which this function is called will be invalid after, and must not be used for anything else.
    virtual void      release() noexcept = 0;



  public:

    Handle(const Handle&) = delete;



    AGT_nodiscard AGT_forceinline ObjectType  getType() const noexcept {
      return type;
    }
    AGT_nodiscard AGT_forceinline ObjectFlags getFlags() const noexcept {
      return flags;
    }
    AGT_nodiscard AGT_forceinline AgtContext  getContext() const noexcept {
      return context;
    }
    AGT_nodiscard AGT_forceinline AgtObjectId getId() const noexcept {
      return id;
    }

    AGT_nodiscard AGT_forceinline bool        isShared() const noexcept {
      return static_cast<bool>(getFlags() & ObjectFlags::isShared);
    }


    virtual AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept = 0;
    virtual void      send(AgtMessage message, AgtSendFlags flags) noexcept = 0;
    virtual AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept = 0;

    virtual void      releaseMessage(AgtMessage message) noexcept = 0;

    virtual AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept = 0;


    AgtStatus               duplicate(Handle*& newHandle) noexcept;

    void                    close() noexcept;

  };

  class LocalHandle : public Handle {
  protected:

    ~LocalHandle() = default;
  };

  class SharedObject {

    void*       reserved;
    AgtObjectId id;
    ObjectType  type;
    ObjectFlags flags;


  protected:

    SharedObject(ObjectType type, ObjectFlags flags, AgtObjectId id) noexcept
        : type(type),
          flags(flags),
          id(id)
    {}

  public:

    AGT_nodiscard ObjectType  getType() const noexcept {
      return type;
    }
    AGT_nodiscard ObjectFlags getFlags() const noexcept {
      return flags;
    }
    AGT_nodiscard AgtObjectId getId() const noexcept {
      return id;
    }
  };

  class SharedHandle : public Handle {
  protected:
    SharedVPtr    const vptr;
    SharedObject* const instance;

    SharedHandle(SharedObject* pInstance, AgtContext context, Id localId) noexcept;

  public:

    AGT_forceinline AgtStatus sharedAcquire() const noexcept {
      return vptr->acquireRef(instance, getContext());
    }
    AGT_forceinline AgtSize   sharedRelease() const noexcept {
      return vptr->releaseRef(instance, getContext());
    }

    AGT_nodiscard AGT_forceinline SharedObject* getInstance() const noexcept {
      return instance;
    }


    AGT_nodiscard AGT_forceinline AgtStatus sharedStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept {
      return vptr->stage(instance, getContext(), pStagedMessage, timeout);
    }
    AGT_forceinline void sharedSend(AgtMessage message, AgtSendFlags flags) noexcept {
      vptr->send(instance, getContext(), message, flags);
    }
    AGT_nodiscard AGT_forceinline AgtStatus sharedReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept {
      return vptr->receive(instance, getContext(), pMessageInfo, timeout);
    }

    AGT_nodiscard AGT_forceinline AgtStatus sharedConnect(Handle* otherHandle, ConnectAction action) noexcept {
      return vptr->connect(instance, getContext(), otherHandle, action);
    }


  };*/

  template <typename Hdl>
  inline Hdl*      allocHandle(AgtContext ctx) noexcept {
    using HandleType = typename ObjectInfo<Hdl>::HandleType;
    auto handle      = (HandleHeader*)ctxAllocHandle(ctx, sizeof(HandleType), alignof(HandleType));
    handle->vptr     = &VTableInstance<Hdl>;
    handle->type     = ObjectInfo<Hdl>::TypeValue;
    handle->context  = ctx;
    return static_cast<Hdl*>(handle);
  }


}

#endif//JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
