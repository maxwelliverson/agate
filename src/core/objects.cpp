//
// Created by maxwe on 2021-11-22.
//

#include "objects.hpp"


using namespace Agt;

namespace {

  AGT_noinline AgtStatus sharedHandleDuplicate(Handle* self_, Handle*& out) noexcept {
    const auto self = static_cast<SharedHandle*>(self_);
    if (AgtStatus result = self->sharedAcquire())
      return result;
    if (SharedHandle* pNewShared = ctxNewSharedHandle(self->getContext(), self->getInstance()))
      out = pNewShared;
    else
      return AGT_ERROR_BAD_ALLOC;
    return AGT_SUCCESS;
  }

}



AgtStatus Handle::duplicate(Handle*& out) noexcept {
  if (!isShared()) [[likely]] {
    if (AgtStatus result = static_cast<LocalHandle*>(this)->localAcquire())
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

AgtStatus Handle::stage(AgtStagedMessage& stagedMessage, AgtTimeout timeout) noexcept {
  if (!isShared()) [[likely]]
    return static_cast<LocalHandle*>(this)->localStage(stagedMessage, timeout);
  else
    return static_cast<SharedHandle*>(this)->sharedStage(stagedMessage, timeout);
}
void      Handle::send(AgtMessage message, AgtSendFlags flags) noexcept {
  if (!isShared()) [[likely]]
    static_cast<LocalHandle*>(this)->localSend(message, flags);
  else
    static_cast<SharedHandle*>(this)->sharedSend(message, flags);
}
AgtStatus Handle::receive(AgtMessageInfo& messageInfo, AgtTimeout timeout) noexcept {
  if (!isShared()) [[likely]]
    return static_cast<LocalHandle*>(this)->localReceive(messageInfo, timeout);
  else
    return static_cast<SharedHandle*>(this)->sharedReceive(messageInfo, timeout);
}

AgtStatus Handle::connect(Handle* otherHandle, ConnectAction action) noexcept {
  if (!isShared()) [[likely]]
    return static_cast<LocalHandle*>(this)->localConnect(otherHandle, action);
  else
    return static_cast<SharedHandle*>(this)->sharedConnect(otherHandle, action);
}


SharedHandle::SharedHandle(SharedObject* pInstance, AgtContext context, Id localId) noexcept
    : Handle(pInstance->getType(), pInstance->getFlags(), context, localId),
      vptr(lookupSharedVTable(pInstance->getType())),
      instance(pInstance)
{}