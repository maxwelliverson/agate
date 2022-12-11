//
// Created by maxwe on 2021-11-22.
//

#include "objects.hpp"


template class agt::impl::strong_ref_base<true>;
template class agt::impl::strong_ref_base<false>;
template class agt::impl::weak_ref_base<true>;
template class agt::impl::weak_ref_base<false>;



/*

using namespace Agt;

namespace {

  AGT_noinline agt_status_t sharedHandleDuplicate(Handle* self_, Handle*& out) noexcept {
    const auto self = static_cast<SharedHandle*>(self_);
    if (agt_status_t result = self->sharedAcquire())
      return result;
    if (SharedHandle* pNewShared = ctxNewSharedHandle(self->getContext(), self->getInstance()))
      out = pNewShared;
    else
      return AGT_ERROR_BAD_ALLOC;
    return AGT_SUCCESS;
  }

}



agt_status_t Handle::duplicate(Handle*& out) noexcept {
  if (!isShared()) [[likely]] {
    if (agt_status_t result = static_cast<LocalHandle*>(this)->localAcquire())
      return result;
    out = this;
    return AGT_SUCCESS;
  }
  else
    return sharedHandleDuplicate(this, out);
}

void Handle::close() noexcept  {
  if (!isShared()) [[likely]]
    static_cast<LocalHandle*>(this)->localRelease();
  else
    ctxDestroySharedHandle(context, static_cast<SharedHandle*>(this));
}

agt_status_t Handle::stage(agt_staged_message_t& stagedMessage, agt_timeout_t timeout) noexcept {
  if (!isShared()) [[likely]]
    return static_cast<LocalHandle*>(this)->localStage(stagedMessage, timeout);
  else
    return static_cast<SharedHandle*>(this)->sharedStage(stagedMessage, timeout);
}
void      Handle::send(agt_message_t message, agt_send_flags_t flags) noexcept {
  if (!isShared()) [[likely]]
    static_cast<LocalHandle*>(this)->localSend(message, flags);
  else
    static_cast<SharedHandle*>(this)->sharedSend(message, flags);
}
agt_status_t Handle::receive(agt_message_info_t& messageInfo, agt_timeout_t timeout) noexcept {
  if (!isShared()) [[likely]]
    return static_cast<LocalHandle*>(this)->localReceive(messageInfo, timeout);
  else
    return static_cast<SharedHandle*>(this)->sharedReceive(messageInfo, timeout);
}

agt_status_t Handle::connect(Handle* otherHandle, ConnectAction action) noexcept {
  if (!isShared()) [[likely]]
    return static_cast<LocalHandle*>(this)->localConnect(otherHandle, action);
  else
    return static_cast<SharedHandle*>(this)->sharedConnect(otherHandle, action);
}


SharedHandle::SharedHandle(SharedObject* pInstance, agt_ctx_t context, Id localId) noexcept
    : Handle(pInstance->getType(), pInstance->getFlags(), context, localId),
      vptr(lookupSharedVTable(pInstance->getType())),
      instance(pInstance)
{}*/
