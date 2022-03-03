//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_VTABLE_HPP
#define JEMSYS_AGATE2_VTABLE_HPP


#include "fwd.hpp"

#include <concepts>



#define AGT_declare_vtable(type_)  \
  template <> AgtStatus ObjectInfo<type_>::acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept; \
  template <> void      ObjectInfo<type_>::pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept;                    \
  template <> AgtStatus ObjectInfo<type_>::popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept;           \
  template <> void      ObjectInfo<type_>::releaseMessage(HandleHeader* object, AgtMessage message) noexcept;                                   \
  template <> AgtStatus ObjectInfo<type_>::connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept;                  \
  template <> AgtStatus ObjectInfo<type_>::acquireRef(HandleHeader* object) noexcept;                                                           \
  template <> void      ObjectInfo<type_>::releaseRef(HandleHeader* object) noexcept;                                                           \
                                   \
  extern template struct ObjectInfo<type_>


namespace Agt {

  namespace Impl {
    template <typename T, auto N = 2>
    struct GetHandleType : GetHandleType<T, N-1>{};
    template <typename T, auto N = 2>
    struct GetObjectType : GetObjectType<T, N-1>{};

    template <typename T>
    struct GetHandleType<T, 0>;

    template <typename T>
    struct GetObjectType<T, 0>;

    template <std::derived_from<HandleHeader> T>
    struct GetHandleType<T, 1> {
      using HandleType = T;
    };
    template <typename T> requires ( requires { typename T::HandleType; })
    struct GetHandleType<T> {
      using HandleType = typename T::HandleType;
    };

    template <std::derived_from<HandleHeader> T>
    struct GetObjectType<T, 1> {
      using ObjectType = T;
    };
    template <typename T> requires ( requires { typename T::ObjectType; } )
    struct GetObjectType<T> {
      using ObjectType = typename T::ObjectType;
    };
  }

  struct VTable {
    AgtStatus (* const pfnAcquireMessage )(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept;
    void      (* const pfnPushQueue )(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept;
    AgtStatus (* const pfnPopQueue )(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept;
    void      (* const pfnReleaseMessage )(HandleHeader* object, AgtMessage message) noexcept;
    AgtStatus (* const pfnConnect )(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept;
    AgtStatus (* const pfnAcquireRef )(HandleHeader* object) noexcept;
    void      (* const pfnReleaseRef )(HandleHeader* object) noexcept;
    void      (* const pfnSetErrorState )(HandleHeader* object, ErrorState errorState) noexcept;
  };

  template <typename T>
  struct ObjectInfo{

    inline constexpr static ObjectType TypeValue = T::TypeValue;

    using HandleType = typename Impl::GetHandleType<T>::HandleType;
    using ObjectType = typename Impl::GetObjectType<T>::ObjectType;


    static AgtStatus acquireMessage(HandleHeader* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept;
    static void      pushQueue(HandleHeader* object, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus popQueue(HandleHeader* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept;
    static void      releaseMessage(HandleHeader* object, AgtMessage message) noexcept;
    static AgtStatus connect(HandleHeader* object, HandleHeader* handle, ConnectAction action) noexcept;
    static AgtStatus acquireRef(HandleHeader* object) noexcept;
    static void      releaseRef(HandleHeader* object) noexcept;

    static void      setErrorState(HandleHeader* object, ErrorState errorState) noexcept;
  };


  template <typename Hdl>
  inline AgtStatus handleAcquireMessage(Hdl* handle, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept {
    return ObjectInfo<Hdl>::acquireMessage(handle, pStagedMessage, timeout);
  }
  template <typename Hdl>
  inline void      handlePushQueue(Hdl* handle, AgtMessage message, AgtSendFlags flags) noexcept {
    ObjectInfo<Hdl>::pushQueue(handle, message, flags);
  }
  template <typename Hdl>
  inline AgtStatus handlePopQueue(Hdl* handle, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept {
    return ObjectInfo<Hdl>::popQueue(handle, pMessageInfo, timeout);
  }
  template <typename Hdl>
  inline void      handleReleaseMessage(Hdl* handle, AgtMessage message) noexcept {
    ObjectInfo<Hdl>::releaseMessage(handle, message);
  }
  template <typename Hdl>
  inline AgtStatus handleConnect(Hdl* handle, HandleHeader* otherHandle, ConnectAction connectAction) noexcept {
    return ObjectInfo<Hdl>::connect(handle, otherHandle, connectAction);
  }
  template <typename Hdl>
  inline AgtStatus handleAcquireRef(Hdl* handle) noexcept {
    return ObjectInfo<Hdl>::acquireRef(handle);
  }
  template <typename Hdl>
  inline void      handleReleaseRef(Hdl* handle) noexcept {
    ObjectInfo<Hdl>::releaseRef(handle);
  }
  template <typename Hdl>
  inline void      handleSetErrorState(Hdl* handle, ErrorState errorState) noexcept {
    ObjectInfo<Hdl>::setErrorState(handle, errorState);
  }


  template <typename T>
  inline constexpr static VTable VTableInstance = {
    .pfnAcquireMessage = &ObjectInfo<typename ObjectInfo<T>::HandleType>::acquireMessage,
    .pfnPushQueue      = &ObjectInfo<typename ObjectInfo<T>::HandleType>::pushQueue,
    .pfnPopQueue       = &ObjectInfo<typename ObjectInfo<T>::HandleType>::popQueue,
    .pfnReleaseMessage = &ObjectInfo<typename ObjectInfo<T>::HandleType>::releaseMessage,
    .pfnConnect        = &ObjectInfo<typename ObjectInfo<T>::HandleType>::connect,
    .pfnAcquireRef     = &ObjectInfo<typename ObjectInfo<T>::HandleType>::acquireRef,
    .pfnReleaseRef     = &ObjectInfo<typename ObjectInfo<T>::HandleType>::releaseRef
  };


  /*struct SharedVTable {
    AgtStatus (* const acquireRef)(SharedObject* object, AgtContext ctx) noexcept;
    AgtSize   (* const releaseRef)(SharedObject* object, AgtContext ctx) noexcept;
    void      (* const destroy)(SharedObject* object, AgtContext ctx) noexcept;
    AgtStatus (* const stage)(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    void      (* const send)(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    AgtStatus (* const receive)(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    AgtStatus (* const connect)(SharedObject* object, AgtContext ctx, Handle* handle, ConnectAction action) noexcept;
  };*/

  // SharedVPtr lookupSharedVTable(ObjectType type) noexcept;

}

#endif//JEMSYS_AGATE2_VTABLE_HPP
