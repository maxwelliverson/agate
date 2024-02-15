//
// Created by maxwe on 2023-08-14.
//

#include "../module_exports.hpp"

#include <Windows.h>


namespace agt {
  void AGT_stdcall log_private(agt_self_t self, agt_u32_t category, const void* msg, size_t msgLength) noexcept {
    HANDLE outConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!outConsole)
      return;

    DWORD bytesWritten;
    WriteFile(outConsole, msg, msgLength, &bytesWritten, nullptr);
  }
}

AGT_define_get_exports(moduleExports, attributes) {
  AGT_add_public_export(log, agt::log_private);
}