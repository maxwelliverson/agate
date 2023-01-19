//
// Created by maxwe on 2023-01-05.
//

#define AGT_COMPILE_MT_SHARED_LIB

#include "config.hpp"





#define AGT_exported_function_name log
#define AGT_return_type void


AGT_export_family {

  AGT_function_entry(mt)(agt_agent_t self, agt_u32_t category, const void* buffer, size_t bufferLength) {
    if (self == nullptr)
      return;


  }


}

