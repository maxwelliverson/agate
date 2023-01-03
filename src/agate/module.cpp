//
// Created by maxwe on 2022-12-21.
//

#include "config.hpp"


#include <cstring>
#include <utility>



/*inline static constinit agt::export_table g_moduleExports = { };


inline const agt::export_table& agt::get_exports() noexcept {
  return g_moduleExports;
}*/

extern "C" {

  AGT_module_export agt_u32_t AGT_stdcall AGT_MODULE_VERSION_FUNCTION() {
    // Populated by target_compile_definitions command in CMake script
    return AGT_MAKE_VERSION(AGT_MODULE_VERSION_MAJOR, AGT_MODULE_VERSION_MINOR, AGT_MODULE_VERSION_PATCH);
  }

}



