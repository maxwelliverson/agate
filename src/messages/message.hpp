//
// Created by maxwe on 2021-11-28.
//

#ifndef JEMSYS_AGATE2_MESSAGE_HPP
#define JEMSYS_AGATE2_MESSAGE_HPP

#include "fwd.hpp"
#include "support/flags.hpp"

namespace agt {



  struct AGT_cache_aligned inline_buffer {};

  /*struct StagedMessage {
    HandleHeader* receiver;
    void*         message;
    void*         reserved[2];
    HandleHeader* returnHandle;
    size_t       messageSize;
    agt_message_id_t  id;
    void*         payload;
  };*/

  AGT_BITFLAG_ENUM(message_flags, agt_u32_t) {
    // dispatchKindById    = AGT_DISPATCH_BY_ID,
    // dispatchKindByName  = AGT_DISPATCH_BY_NAME,

    isOutOfLine         = 0x04,
    isMultiFrame        = 0x08,

    ownedByChannel      = 0x40,
    isAgentInvocation   = 0x80,
    isShared            = 0x100,
    shouldDoFastCleanup = 0x200,
  };

  AGT_BITFLAG_ENUM(message_state, agt_u32_t) {
    isQueued    = 0x1,
    isOnHold    = 0x2,
    isCondemned = 0x4
  };

  inline constexpr static message_state default_message_state = {};

  /**
   * @returns true if successful, false if there was an allocation error
   * */
  bool  initMessageArray(local_channel_header* owner) noexcept;

  /**
   * @returns the address of the message array if successful, or a null point on allocation error
   * */
  void* initMessageArray(shared_handle_header* owner, shared_channel_header* channel) noexcept;

  agt_status_t getMultiFrameMessage(inline_buffer* inlineBuffer, agt_multi_frame_message_info_t& messageInfo) noexcept;
  bool      getNextFrame(agt_message_frame_t& frame, agt_multi_frame_message_info_t& messageInfo) noexcept;


  void initMessage(agt_message_t message) noexcept;

  void setMessageId(agt_message_t message, agt_message_id_t id) noexcept;
  void setMessageReturnHandle(agt_message_t message, Handle* returnHandle) noexcept;
  void setMessageAsyncHandle(agt_message_t message, agt_async_t async) noexcept;

  void cleanupMessage(agt_message_t message) noexcept;

}

struct agt_message_st {
  union {
    agt_message_t             next;
    size_t                    nextOffset;
  };
  union {
    agt_handle_t              owner;
    agt::message_pool_t       pool;
  };
  union {
    agt_handle_t              returnHandle;
    agt_object_id_t           returnHandleId;
  };
  union {
    agt_async_data_t          asyncData;
    agt::shared_allocation_id asyncDataAllocId;
  };
  agt::async_key_t            asyncDataKey;
  agt::message_flags          flags;
  agt::message_state          state;
  agt_u32_t                   refCount;
  agt_u32_t                   messageType;
  // Free 32 bits
  size_t                      payloadSize;
  agt::inline_buffer          inlineBuffer[];
};

static_assert(sizeof(agt_message_st) == AGT_CACHE_LINE);


#endif//JEMSYS_AGATE2_MESSAGE_HPP
