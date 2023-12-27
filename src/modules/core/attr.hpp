//
// Created by Maxwell on 2023-12-16.
//

#ifndef AGATE_CORE_ATTR_HPP
#define AGATE_CORE_ATTR_HPP



#include "config.hpp"

#include "agate/version.hpp"


#include <optional>

namespace agt::attr {

  // Returns AGT_ATTR_ASYNC_STRUCT_SIZE
  agt_u32_t async_struct_size() noexcept;

  // Returns AGT_ATTR_THREAD_COUNT
  agt_u32_t thread_count()      noexcept;

  // Returns AGT_ATTR_LIBRARY_VERSION
  version   library_version()   noexcept;

  // Returns AGT_ATTR_SHARED_CONTEXT
  bool      shared_context()    noexcept;

  // Returns AGT_ATTR_CXX_EXCEPTIONS_ENABLED
  bool      cxx_exceptions_enabled() noexcept;

  // Returns AGT_ATTR_CHANNEL_DEFAULT_TIMEOUT_MS
  agt_u32_t default_timeout_ms() noexcept;

  // Returns AGT_ATTR_DURATION_UNIT_NS
  agt_u64_t duration_unit_ns() noexcept;

  // Returns AGT_ATTR_NATIVE_DURATION_UNIT_NS
  agt_u64_t native_duration_unit_ns() noexcept;

  // Returns AGT_ATTR_STACK_SIZE_ALIGNMENT
  agt_u64_t stack_size_alignment() noexcept;

  // Returns AGT_ATTR_DEFAULT_THREAD_STACK_SIZE
  agt_u64_t default_thread_stack_size() noexcept;

  // Returns AGT_ATTR_DEFAULT_FIBER_STACK_SIZE
  agt_u64_t default_fiber_stack_size() noexcept;

  // Returns AGT_ATTR_FULL_STATE_SAVE_MASK
  agt_u64_t full_state_save_mask() noexcept;

  // Returns AGT_ATTR_LIBRARY_PATH if stored value is of type STRING, otherwise returns nullopt
  std::optional<const char*>    library_path() noexcept;

  //  Returns AGT_ATTR_LIBRARY_PATH if stored value is of type WIDE_STRING, otherwise returns nullopt
  std::optional<const wchar_t*> library_path_wide() noexcept;

  // Returns AGT_ATTR_SHARED_NAMESPACE if stored value is of type STRING, otherwise returns nullopt
  std::optional<const char*>    shared_namespace() noexcept;

  // Returns AGT_ATTR_SHARED_NAMESPACE if stored value is of type WIDE_STRING, otherwise returns nullopt
  std::optional<const wchar_t*> shared_namespace_wide() noexcept;

}

#endif //AGATE_CORE_ATTR_HPP
