//
// Created by maxwe on 2022-01-04.
//

#include "vtable.hpp"
#include "modules/channels/channel.hpp"

#include <concepts>

using namespace Agt;

template <std::derived_from<SharedObject> Obj>
inline constexpr static SharedVTable sharedVTableInstance = {
  .acquireRef = &Obj::sharedAcquire,
  .releaseRef = &Obj::sharedRelease,
  .destroy    = &Obj::sharedDestroy,
  .stage      = &Obj::sharedStage,
  .send       = &Obj::sharedSend,
  .receive    = &Obj::sharedReceive,
  .connect    = &Obj::sharedConnect
};

SharedVPtr Agt::lookupSharedVTable(ObjectType type) noexcept {
  static constexpr SharedVTable sharedVTableTable[] = {
    sharedVTableInstance<SharedSpScChannel>,
    sharedVTableInstance<SharedSpScChannelSender>,
    sharedVTableInstance<SharedSpScChannelReceiver>,
    sharedVTableInstance<SharedSpMcChannel>,
    sharedVTableInstance<SharedSpMcChannelSender>,
    sharedVTableInstance<SharedSpMcChannelReceiver>,
    sharedVTableInstance<SharedMpScChannel>,
    sharedVTableInstance<SharedMpScChannelSender>,
    sharedVTableInstance<SharedMpScChannelReceiver>,
    sharedVTableInstance<SharedMpMcChannel>,
    sharedVTableInstance<SharedMpMcChannelSender>,
    sharedVTableInstance<SharedMpMcChannelReceiver>
  };

  return &sharedVTableTable[static_cast<agt_u32_t>(type)];
}
