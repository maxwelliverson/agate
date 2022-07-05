//
// Created by maxwe on 2022-05-27.
//


#include "agents.hpp"
#include "messages/message.hpp"


#include <cstring>


/**
 * Layout of a no dispatch message:
 *
 *    0-7
 * [ agent | data... ]
 *
 *
 * Layout of an id dispatch message:
 *
 *    0-7    8-15
 * [ agent | msgId | data... ]
 *
 *
 * Layout of a name dispatch message:
 *      Note: the padding field ensures that data is properly aligned to 8 bytes.
 *
 *    0..7   8..11    12..12+N-1   12+N..(12+N-1 | 7)+1
 * [ agent |   N   |     name    |        padding        |   data...     ]
 *
 *
 *
 *
 * */





namespace {

  inline constexpr agt_flags32_t DispatchMask = AGT_DISPATCH_BY_ID | AGT_DISPATCH_BY_NAME;

  agt_action_result_t local_dispatch_on_message(agt_message_t message, void* buffer, size_t bufferSize) noexcept {

    agt_agent_message_info_t messageInfo;

    assert( (message->flags & agt::message_flags::isAgentInvocation) != agt::message_flags{} );

    agt_agent_t agent;

    std::memcpy(&agent, message->inlineBuffer, sizeof(agt_agent_t));
    messageInfo.self = agent;
    messageInfo.messageHandle = message;
    messageInfo.flags         = (agt_message_flags_t)message->flags;
    messageInfo.dispatchKind  = (agt_dispatch_kind_t)(messageInfo.flags & DispatchMask);

    auto inlineBuffer = ((char*)message->inlineBuffer) + sizeof(agt_agent_t);
    size_t modifiedPayloadSize = message->payloadSize - sizeof(agt_agent_t);

    agt_action_result_t result;

    switch (messageInfo.dispatchKind) {
      case AGT_NO_DISPATCH: {
        std::memcpy(buffer, inlineBuffer, modifiedPayloadSize);
        messageInfo.messageSize = modifiedPayloadSize;
        result = agent->noDispatchProc(agent->state, buffer, &messageInfo);
      } break;
      case AGT_DISPATCH_BY_ID: {
        agt_message_id_t msgId;
        std::memcpy(&msgId, inlineBuffer, sizeof msgId);
        inlineBuffer += sizeof msgId;
        modifiedPayloadSize -= sizeof msgId;
        std::memcpy(buffer, inlineBuffer, modifiedPayloadSize);
        messageInfo.messageSize = modifiedPayloadSize;
        result = agent->idDispatchProc(agent->state, msgId, buffer, &messageInfo);
      } break;
      case AGT_DISPATCH_BY_NAME: {
        size_t nameLength;
        std::memcpy(&nameLength, inlineBuffer, sizeof nameLength);
        inlineBuffer += sizeof nameLength;
        modifiedPayloadSize -= sizeof nameLength;
        std::memcpy(buffer, inlineBuffer, modifiedPayloadSize);
        char* alignedInlineBuffer = (char*)((((uintptr_t)inlineBuffer + nameLength) | 0x7) + 1);
        modifiedPayloadSize -= (alignedInlineBuffer - inlineBuffer);
        messageInfo.messageSize = modifiedPayloadSize;
        result = agent->nameDispatchProc(agent->state, (const char*)inlineBuffer, nameLength, alignedInlineBuffer, &messageInfo);
      }
    }

    return result;
  }




  void dispatch_on_agent() noexcept {

  }

  struct agency_state_t {
    agt_handle_t       inHandle;
    agt_timeout_t      timeout;
    agt_message_info_t messageInfo;
    size_t             bufferSize;

    char               buffer[];



    void yield() noexcept;


    void handle_bad_read(agt_status_t readResult) noexcept;

    void read_next() noexcept {
      if (auto result = agt_receive_message_data(inHandle, buffer, bufferSize, &messageInfo, timeout)) [[unlikely]]
        handle_bad_read(result);

    }
  };
}


