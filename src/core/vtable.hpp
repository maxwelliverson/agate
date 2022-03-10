//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_VTABLE_HPP
#define JEMSYS_AGATE2_VTABLE_HPP


#include "fwd.hpp"

#include <concepts>



#define AGT_declare_vtable(type_)  \
  template <> agt_status_t object_info<type_>::acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept; \
  template <> void      object_info<type_>::pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept;                    \
  template <> agt_status_t object_info<type_>::popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept;           \
  template <> void      object_info<type_>::releaseMessage(handle_header* object, agt_message_t message) noexcept;                                   \
  template <> agt_status_t object_info<type_>::connect(handle_header* object, handle_header* handle, connect_action action) noexcept;                  \
  template <> agt_status_t object_info<type_>::acquireRef(handle_header* object) noexcept;                                                           \
  template <> void      object_info<type_>::releaseRef(handle_header* object) noexcept;                                                           \
                                   \
  extern template struct object_info<type_>


namespace agt {

  namespace impl {
    template <typename T, auto N = 2>
    struct get_handle_type : get_handle_type<T, N-1>{};
    template <typename T, auto N = 2>
    struct get_object_type : get_object_type<T, N-1>{};

    template <typename T>
    struct get_handle_type<T, 0>;

    template <typename T>
    struct get_object_type<T, 0>;

    template <std::derived_from<handle_header> T>
    struct get_handle_type<T, 1> {
      using handle_type = T;
    };
    template <typename T> requires ( requires { typename T::handle_type; })
    struct get_handle_type<T> {
      using handle_type = typename T::handle_type;
    };

    template <std::derived_from<handle_header> T>
    struct get_object_type<T, 1> {
      using object_type = T;
    };
    template <typename T> requires ( requires { typename T::object_type; } )
    struct get_object_type<T> {
      using object_type = typename T::object_type;
    };
  }

  struct vtable {
    agt_status_t (* const pfnAcquireMessage )(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept;
    void         (* const pfnPushQueue )(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept;
    agt_status_t (* const pfnPopQueue )(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept;
    void         (* const pfnReleaseMessage )(handle_header* object, agt_message_t message) noexcept;
    agt_status_t (* const pfnConnect )(handle_header* object, handle_header* handle, connect_action action) noexcept;
    agt_status_t (* const pfnAcquireRef )(handle_header* object) noexcept;
    void         (* const pfnReleaseRef )(handle_header* object) noexcept;
    void         (* const pfnSetErrorState )(handle_header* object, error_state errorState) noexcept;
  };

  template <typename T>
  struct object_info{

    inline constexpr static object_type type_value = T::type_value;

    using handle_type = typename impl::get_handle_type<T>::handle_type;
    using object_type = typename impl::get_object_type<T>::object_type;


    static agt_status_t acquireMessage(handle_header* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept;
    static void         pushQueue(handle_header* object, agt_message_t message, agt_send_flags_t flags) noexcept;
    static agt_status_t popQueue(handle_header* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept;
    static void         releaseMessage(handle_header* object, agt_message_t message) noexcept;
    static agt_status_t connect(handle_header* object, handle_header* handle, connect_action action) noexcept;
    static agt_status_t acquireRef(handle_header* object) noexcept;
    static void         releaseRef(handle_header* object) noexcept;

    static void         setErrorState(handle_header* object, error_state errorState) noexcept;
  };


  template <typename Hdl>
  inline agt_status_t handleAcquireMessage(Hdl* handle, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept {
    return object_info<Hdl>::acquireMessage(handle, pStagedMessage, timeout);
  }
  template <typename Hdl>
  inline void         handlePushQueue(Hdl* handle, agt_message_t message, agt_send_flags_t flags) noexcept {
    object_info<Hdl>::pushQueue(handle, message, flags);
  }
  template <typename Hdl>
  inline agt_status_t handlePopQueue(Hdl* handle, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept {
    return object_info<Hdl>::popQueue(handle, pMessageInfo, timeout);
  }
  template <typename Hdl>
  inline void         handleReleaseMessage(Hdl* handle, agt_message_t message) noexcept {
    object_info<Hdl>::releaseMessage(handle, message);
  }
  template <typename Hdl>
  inline agt_status_t handleConnect(Hdl* handle, handle_header* otherHandle, connect_action connectAction) noexcept {
    return object_info<Hdl>::connect(handle, otherHandle, connectAction);
  }
  template <typename Hdl>
  inline agt_status_t handleAcquireRef(Hdl* handle) noexcept {
    return object_info<Hdl>::acquireRef(handle);
  }
  template <typename Hdl>
  inline void         handleReleaseRef(Hdl* handle) noexcept {
    object_info<Hdl>::releaseRef(handle);
  }
  template <typename Hdl>
  inline void         handleSetErrorState(Hdl* handle, error_state errorState) noexcept {
    object_info<Hdl>::setErrorState(handle, errorState);
  }


  template <typename T>
  inline constexpr static vtable vtable_instance = {
    .pfnAcquireMessage = &object_info<typename object_info<T>::handle_type>::acquireMessage,
    .pfnPushQueue      = &object_info<typename object_info<T>::handle_type>::pushQueue,
    .pfnPopQueue       = &object_info<typename object_info<T>::handle_type>::popQueue,
    .pfnReleaseMessage = &object_info<typename object_info<T>::handle_type>::releaseMessage,
    .pfnConnect        = &object_info<typename object_info<T>::handle_type>::connect,
    .pfnAcquireRef     = &object_info<typename object_info<T>::handle_type>::acquireRef,
    .pfnReleaseRef     = &object_info<typename object_info<T>::handle_type>::releaseRef
  };


  /*struct SharedVTable {
    agt_status_t (* const acquireRef)(SharedObject* object, agt_ctx_t ctx) noexcept;
    size_t   (* const releaseRef)(SharedObject* object, agt_ctx_t ctx) noexcept;
    void      (* const destroy)(SharedObject* object, agt_ctx_t ctx) noexcept;
    agt_status_t (* const stage)(SharedObject* object, agt_ctx_t ctx, agt_staged_message_t& pStagedMessage, agt_timeout_t timeout) noexcept;
    void      (* const send)(SharedObject* object, agt_ctx_t ctx, agt_message_t message, agt_send_flags_t flags) noexcept;
    agt_status_t (* const receive)(SharedObject* object, agt_ctx_t ctx, agt_message_info_t& pMessageInfo, agt_timeout_t timeout) noexcept;
    agt_status_t (* const connect)(SharedObject* object, agt_ctx_t ctx, Handle* handle, ConnectAction action) noexcept;
  };*/

  // SharedVPtr lookupSharedVTable(ObjectType type) noexcept;

}

#endif//JEMSYS_AGATE2_VTABLE_HPP
