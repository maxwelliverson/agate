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

namespace agt {

  enum class object_type : agt_u32_t {
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

  enum class connect_action : agt_u32_t {
    connectSender,
    disconnectSender,
    connectReceiver,
    disconnectReceiver
  };

  AGT_forceinline static agt_handle_type_t toHandleType(object_type type) noexcept;

  AGT_BITFLAG_ENUM(object_flags, agt_handle_flags_t) {
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
        return static_cast<agt_u64_t>(-1);
      return agt_u64_t ((AGT_HANDLE_FLAGS_MAX - 1) << 1) - 1ULL;
    }
    else {
      // 32 bit path
      if (AGT_HANDLE_FLAGS_MAX == 0x1000'0001)
        return static_cast<agt_u32_t>(-1);
      return agt_u32_t ((AGT_HANDLE_FLAGS_MAX - 1) << 1) - 1;
    }
  }

  AGT_forceinline static agt_handle_flags_t toHandleFlags(object_flags flags) noexcept {
    constexpr static auto Mask = getHandleFlagMask();
    return static_cast<agt_handle_flags_t>(flags) & Mask;
  }


  class proxy_object {
    // The reserved member field is here to enable the spoofing of Handles as objects (Handles are virtual, while general objects are not).
    // Note that while this relies on technically undefined behaviour, the placement of the virtual pointer at the start of objects has been
    // the the behaviour of the three major C++ compilers for long enough that I feel comfortable taking advantage of that particular implementation detail.
    void*           reserved;
    agt_object_id_t id;
    object_type     type;
    object_flags    flags;

  public:

    AGT_nodiscard AGT_forceinline agt_object_id_t get_id() const noexcept {
      return id;
    }
    AGT_nodiscard AGT_forceinline object_type  get_type() const noexcept {
      return type;
    }
    AGT_nodiscard AGT_forceinline object_flags get_flags() const noexcept {
      return flags;
    }
  };


  struct shared_object_header {
    size_t     objectSize;
    agt_object_id_t id;
    object_type  type;
    object_flags flags;
  };

  struct handle_header {
    vpointer        vptr;
    agt_object_id_t id;
    object_type  type;
    object_flags flags;
    agt_ctx_t  context;
  };

  struct shared_handle_header : handle_header {
    shared_object_header* sharedInstance;
  };


  /*class Handle {

  protected:

    agt_object_id_t id;
    ObjectType  type;
    ObjectFlags flags;
    agt_ctx_t  context;



    virtual agt_status_t acquire() noexcept = 0;
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
    AGT_nodiscard AGT_forceinline agt_ctx_t  getContext() const noexcept {
      return context;
    }
    AGT_nodiscard AGT_forceinline agt_object_id_t getId() const noexcept {
      return id;
    }

    AGT_nodiscard AGT_forceinline bool        isShared() const noexcept {
      return static_cast<bool>(getFlags() & ObjectFlags::isShared);
    }


    virtual agt_status_t stage(agt_staged_message_t& pStagedMessage, agt_timeout_t timeout) noexcept = 0;
    virtual void      send(agt_message_t message, agt_send_flags_t flags) noexcept = 0;
    virtual agt_status_t receive(agt_message_info_t& pMessageInfo, agt_timeout_t timeout) noexcept = 0;

    virtual void      releaseMessage(agt_message_t message) noexcept = 0;

    virtual agt_status_t connect(Handle* otherHandle, ConnectAction action) noexcept = 0;


    agt_status_t               duplicate(Handle*& newHandle) noexcept;

    void                    close() noexcept;

  };

  class LocalHandle : public Handle {
  protected:

    ~LocalHandle() = default;
  };

  class SharedObject {

    void*       reserved;
    agt_object_id_t id;
    ObjectType  type;
    ObjectFlags flags;


  protected:

    SharedObject(ObjectType type, ObjectFlags flags, agt_object_id_t id) noexcept
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
    AGT_nodiscard agt_object_id_t getId() const noexcept {
      return id;
    }
  };

  class SharedHandle : public Handle {
  protected:
    SharedVPtr    const vptr;
    SharedObject* const instance;

    SharedHandle(SharedObject* pInstance, agt_ctx_t context, Id localId) noexcept;

  public:

    AGT_forceinline agt_status_t sharedAcquire() const noexcept {
      return vptr->acquireRef(instance, getContext());
    }
    AGT_forceinline size_t   sharedRelease() const noexcept {
      return vptr->releaseRef(instance, getContext());
    }

    AGT_nodiscard AGT_forceinline SharedObject* getInstance() const noexcept {
      return instance;
    }


    AGT_nodiscard AGT_forceinline agt_status_t sharedStage(agt_staged_message_t& pStagedMessage, agt_timeout_t timeout) noexcept {
      return vptr->stage(instance, getContext(), pStagedMessage, timeout);
    }
    AGT_forceinline void sharedSend(agt_message_t message, agt_send_flags_t flags) noexcept {
      vptr->send(instance, getContext(), message, flags);
    }
    AGT_nodiscard AGT_forceinline agt_status_t sharedReceive(agt_message_info_t& pMessageInfo, agt_timeout_t timeout) noexcept {
      return vptr->receive(instance, getContext(), pMessageInfo, timeout);
    }

    AGT_nodiscard AGT_forceinline agt_status_t sharedConnect(Handle* otherHandle, ConnectAction action) noexcept {
      return vptr->connect(instance, getContext(), otherHandle, action);
    }


  };*/

  template <typename Hdl>
  inline Hdl*      allocHandle(agt_ctx_t ctx) noexcept {
    using HandleType = typename object_info<Hdl>::handle_type;
    auto handle      = (handle_header*)ctxAllocHandle(ctx, sizeof(HandleType), alignof(HandleType));
    handle->vptr     = &vtable_instance<Hdl>;
    handle->type     = object_info<Hdl>::TypeValue;
    handle->context  = ctx;
    return static_cast<Hdl*>(handle);
  }


}

#endif//JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
