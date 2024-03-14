//
// Created by maxwe on 2022-07-29.
//

#ifndef AGATE_CONTEXT_ERROR_HPP
#define AGATE_CONTEXT_ERROR_HPP

#include "config.hpp"

namespace agt {

  namespace err {

    enum class code {
      badFiberExit
    };



    struct fiber_exit_info {
      agt_status_t returnCode;
      int          exitCode;
      agt_string_t name;
    };

    struct overflow_info {
      void*       obj;
      const char* msg;
      agt_u32_t   fieldBits;
    };

    struct attribute_info {
      agt_attr_t    attribute;
      agt_type_id_t requiredType;
      const char*   msg;
    };

    struct envvar_info {
      const char* envvar;
      const char* value;
      const char* msg;
    };

    struct module_info {
      const char* name;
      agt_u32_t   version;
      bool        isFound;
    };

    namespace impl {
      template <typename T>
      struct status_value;

#define AGT_status_info(infoStruct, statusValue) template <> struct status_value<infoStruct> { inline constexpr static agt_status_t value = statusValue; }

      AGT_status_info(overflow_info, AGT_ERROR_INTERNAL_OVERFLOW);

      AGT_status_info(fiber_exit_info, AGT_ERROR_FCTX_EXCEPTION);
      // AGT_status_info(attribute_info, );
    }

    struct internal_info_header {
      agt_status_t status;
    };

    template <typename T>
    struct internal : internal_info_header, T {
      internal() {
        this->status = impl::status_value<T>::value;
      }
    };
  }

  void raise(agt_status_t status, void* errorData, agt_ctx_t context = nullptr) noexcept;

  void raise(agt_instance_t instance, agt_status_t status, void* errorData) noexcept;

  template <typename T>
  void raise(err::internal<T>& info, agt_ctx_t context = nullptr) noexcept {
    raise(AGT_ERROR_UNKNOWN, &info, context);
  }

}


#define AGT_raise(ctx, info) AGT_ctx_api(raise, ctx)(ctx, ::agt::err::impl::status_value<std::remove_cvref_t<decltype(info)>>::value, &info)

#endif//AGATE_CONTEXT_ERROR_HPP
