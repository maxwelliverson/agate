//
// Created by maxwe on 2022-12-19.
//

#include "agents.hpp"



#define AGT_exported_function_name send
#define AGT_return_type agt_status_t


AGT_export_family {

  AGT_function_entry(p)(agt_agent_t recipient, const agt_send_info_t* pSendInfo) {

  }

  AGT_function_entry(s)(agt_agent_t recipient, const agt_send_info_t* pSendInfo) {

  }

}

#undef  AGT_exported_function_name
#define AGT_exported_function_name send_as

AGT_export_family {

  AGT_function_entry(p)(agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo) {

  }

  AGT_function_entry(s)(agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo) {

  }

}

#undef  AGT_exported_function_name
#define AGT_exported_function_name send_many


AGT_export_family {

    AGT_function_entry(p)(const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) {

    }

    AGT_function_entry(s)(const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) {

    }

}


#undef  AGT_exported_function_name
#define AGT_exported_function_name send_many_as


AGT_export_family {

    AGT_function_entry(p)(agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) {

    }

    AGT_function_entry(s)(agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) {

    }

}


#undef  AGT_exported_function_name
#define AGT_exported_function_name reply

AGT_export_family {

    AGT_function_entry(p)(const agt_send_info_t* pSendInfo) {

    }

    AGT_function_entry(s)(const agt_send_info_t* pSendInfo) {

    }

}


#undef  AGT_exported_function_name
#define AGT_exported_function_name reply_as


AGT_export_family {

    AGT_function_entry(p)(agt_agent_t spoofSender, const agt_send_info_t* pSendInfo) {

    }

    AGT_function_entry(s)(agt_agent_t spoofSender, const agt_send_info_t* pSendInfo) {

    }

}






