//
// Created by maxwe on 2022-12-19.
//

#include "core/agents.hpp"



#define AGT_exported_function_name send
#define AGT_return_type agt_status_t


AGT_export_family {

  AGT_function_entry(p)(agt_self_t self_, agt_agent_t recipient_, const agt_send_info_t* pSendInfo) {

    auto self      = (agt::agent_self*)self_;
    auto recipient = (agt::agent*)recipient_;
    auto ctx       = agt::get_ctx();
    // auto ctx       = agt::tl_state.context;

    auto messageSize = pSendInfo->size;

    agt_message_t message   = agt::acquire_message(ctx, recipient->messagePool, messageSize);
    if (!message)
      return AGT_ERROR_MESSAGE_TOO_LARGE;

    message->messageType = agt::AGT_CMD_SEND;
    message->sender      = self;
    message->receiver    = recipient->receiver;
    message->payloadSize = static_cast<uint32_t>(messageSize);

    std::memcpy(message->inlineBuffer, pSendInfo->buffer, messageSize);

    if (pSendInfo->asyncHandle) {
      auto& async = *pSendInfo->asyncHandle;
      agt::asyncAttachLocal(async, 1, 1);
      auto asyncData = agt::asyncGetData(async);
      message->asyncData = asyncData;
      message->asyncDataKey = agt::asyncDataGetKey(asyncData, nullptr);
    }
#if !defined(NDEBUG)
    else {
      message->asyncData    = {};
      message->asyncDataKey = {};
    }
#endif

    if (auto enqueueStatus = agt::enqueueMessage(recipient->messageQueue, message)) {
      agt::release_message(ctx, recipient->messagePool, message);
      return enqueueStatus;
    }

    return AGT_SUCCESS;
  }

  AGT_function_entry(s)(agt_self_t self, agt_agent_t recipient, const agt_send_info_t* pSendInfo) {

  }

}

#undef  AGT_exported_function_name
#define AGT_exported_function_name send_as

AGT_export_family {

  AGT_function_entry(p)(agt_self_t self, agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo) {

  }

  AGT_function_entry(s)(agt_self_t self, agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo) {

  }

}

#undef  AGT_exported_function_name
#define AGT_exported_function_name send_many


AGT_export_family {

    AGT_function_entry(p)(agt_self_t self, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) {

    }

    AGT_function_entry(s)(agt_self_t self, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) {

    }

}


#undef  AGT_exported_function_name
#define AGT_exported_function_name send_many_as


AGT_export_family {

    AGT_function_entry(p)(agt_self_t self, agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) {

    }

    AGT_function_entry(s)(agt_self_t self, agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) {

    }

}


#undef  AGT_exported_function_name
#define AGT_exported_function_name reply

AGT_export_family {

    AGT_function_entry(p)(agt_self_t self, const agt_send_info_t* pSendInfo) {

    }

    AGT_function_entry(s)(agt_self_t self, const agt_send_info_t* pSendInfo) {

    }

}


#undef  AGT_exported_function_name
#define AGT_exported_function_name reply_as


AGT_export_family {

    AGT_function_entry(p)(agt_self_t self, agt_agent_t spoofSender, const agt_send_info_t* pSendInfo) {

    }

    AGT_function_entry(s)(agt_self_t self, agt_agent_t spoofSender, const agt_send_info_t* pSendInfo) {

    }

}






