//
// Created by Maxwell on 2023-12-16.
//

#ifndef AGATE_CORE_ATTR_HPP
#define AGATE_CORE_ATTR_HPP



#include "config.hpp"

#include "agate/version.hpp"

#include "instance.hpp"


#include <optional>

namespace agt::attr {

  // Returns AGT_ATTR_ASYNC_STRUCT_SIZE
  static agt_u32_t async_struct_size(agt_instance_t instance) noexcept {
    return instance->attrValues[AGT_ATTR_ASYNC_STRUCT_SIZE].uint32;
  }

  // Returns AGT_ATTR_THREAD_COUNT
  static agt_u32_t thread_count(agt_instance_t instance)      noexcept {
    return instance->attrValues[AGT_ATTR_THREAD_COUNT].uint32;
  }

  // Returns AGT_ATTR_LIBRARY_VERSION
  static version   library_version(agt_instance_t instance)   noexcept {
    return version::from_integer(instance->attrValues[AGT_ATTR_LIBRARY_VERSION].uint32);
  }

  // Returns AGT_ATTR_SHARED_CONTEXT
  static bool      shared_context(agt_instance_t instance)    noexcept {
    return instance->attrValues[AGT_ATTR_SHARED_CONTEXT].boolean;
  }

  // Returns AGT_ATTR_CXX_EXCEPTIONS_ENABLED
  static bool      cxx_exceptions_enabled(agt_instance_t instance) noexcept {
    return instance->attrValues[AGT_ATTR_CXX_EXCEPTIONS_ENABLED].boolean;
  }

  // Returns AGT_ATTR_CHANNEL_DEFAULT_TIMEOUT_MS
  static agt_u32_t default_timeout_ms(agt_instance_t instance) noexcept;

  // Returns AGT_ATTR_DURATION_UNIT_NS
  static agt_u64_t duration_unit_ns(agt_instance_t instance) noexcept;

  // Returns AGT_ATTR_NATIVE_DURATION_UNIT_NS
  static agt_u64_t native_duration_unit_ns(agt_instance_t instance) noexcept;

  // Returns AGT_ATTR_STACK_SIZE_ALIGNMENT
  static agt_u32_t stack_size_alignment(agt_instance_t instance) noexcept {
    return instance->attrValues[AGT_ATTR_STACK_SIZE_ALIGNMENT].uint32;
  }

  // Returns AGT_ATTR_DEFAULT_THREAD_STACK_SIZE
  static agt_u64_t default_thread_stack_size(agt_instance_t instance) noexcept;

  // Returns AGT_ATTR_DEFAULT_FIBER_STACK_SIZE
  static agt_u64_t default_fiber_stack_size(agt_instance_t instance) noexcept {
    return instance->attrValues[AGT_ATTR_DEFAULT_FIBER_STACK_SIZE].uint64;
  }

  // Returns AGT_ATTR_FULL_STATE_SAVE_MASK
  static agt_u64_t full_state_save_mask(agt_instance_t instance) noexcept {
    return instance->attrValues[AGT_ATTR_FULL_STATE_SAVE_MASK].uint64;
  }

  // Returns AGT_ATTR_MAX_STATE_SAVE_SIZE
  static agt_u32_t max_state_save_size(agt_instance_t instance) noexcept {
    return instance->attrValues[AGT_ATTR_MAX_STATE_SAVE_SIZE].uint32;
  }

  static agt_u64_t max_name_length(agt_instance_t instance) noexcept {
    return instance->attrValues[AGT_ATTR_MAX_NAME_LENGTH].uint64;
  }

  // Returns AGT_ATTR_LIBRARY_PATH if stored value is of type STRING, otherwise returns nullopt
  static std::optional<const char*>    library_path(agt_instance_t instance) noexcept {
    if (instance->attrTypes[AGT_ATTR_LIBRARY_PATH] == AGT_TYPE_STRING)
      return instance->attrValues[AGT_ATTR_LIBRARY_PATH].string;
    return std::nullopt;
  }

  //  Returns AGT_ATTR_LIBRARY_PATH if stored value is of type WIDE_STRING, otherwise returns nullopt
  static std::optional<const wchar_t*> library_path_wide(agt_instance_t instance) noexcept {
    if (instance->attrTypes[AGT_ATTR_LIBRARY_PATH] == AGT_TYPE_WIDE_STRING)
      return instance->attrValues[AGT_ATTR_LIBRARY_PATH].wideString;
    return std::nullopt;
  }

  // Returns AGT_ATTR_SHARED_NAMESPACE if stored value is of type STRING, otherwise returns nullopt
  static std::optional<const char*>    shared_namespace(agt_instance_t instance) noexcept;

  // Returns AGT_ATTR_SHARED_NAMESPACE if stored value is of type WIDE_STRING, otherwise returns nullopt
  static std::optional<const wchar_t*> shared_namespace_wide(agt_instance_t instance) noexcept;

}

#endif //AGATE_CORE_ATTR_HPP
