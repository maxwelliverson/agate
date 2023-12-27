//
// Created by maxwe on 2022-12-21.
//

#ifndef AGATE_INTERNAL_CONFIG_HPP
#define AGATE_INTERNAL_CONFIG_HPP



#include "agate.h"


#if defined(_MSC_VER)
#pragma warning(disable:4229)
#endif


#if defined(NDEBUG)
#define AGT_invariant(expr) AGT_assume(expr)
#else
#define AGT_invariant(expr) AGT_assert(expr)
#endif


#if defined(AGT_BUILD_SHARED)
# if AGT_system_windows
#  if AGT_compiler_msvc
#   define AGT_module_export __declspec(dllexport)
#   define AGT_module_private
#  else
#   define AGT_module_export __attribute((dllexport))
#   define AGT_module_private
#  endif
# else
#  define AGT_module_export __attribute__((visibility("default")))
#  define AGT_module_private __attribute__((visibility("hidden")))
# endif
#elif defined(AGT_BUILD_STATIC)
# define AGT_module_export
# define AGT_module_private
#endif


extern "C" {
#if defined(AGT_USE_NATIVE_WCHAR_TYPE)
using agt_char_t = wchar_t;
#else
using agt_char_t = char;
#endif
}

#define AGT_MODULE_VERSION_FUNCTION        _agate_module_get_version
#define AGT_MODULE_SET_PUBLIC_API_FUNCTION _agate_module_set_api



#define PP_AGT_impl_STRINGIFY_indirect(x) #x
#define PP_AGT_impl_STRINGIFY PP_AGT_impl_STRINGIFY_indirect
#define AGT_stringify(x) PP_AGT_impl_STRINGIFY(x)

#define PP_AGT_impl_CONCAT_indirect(a, b) a##b
#define PP_AGT_impl_CONCAT PP_AGT_impl_CONCAT_indirect
#define AGT_concat(...) PP_AGT_impl_CONCAT(__VA_ARGS__)


#define PP_AGT_impl_CONCAT2_indirect2(a, b) a##b
#define PP_AGT_impl_CONCAT2_indirect PP_AGT_impl_CONCAT2_indirect2
#define PP_AGT_impl_CONCAT2(...) PP_AGT_impl_CONCAT2_indirect(__VA_ARGS__)
#define AGT_concat2(...) PP_AGT_impl_CONCAT2(__VA_ARGS__)

#define AGT_GET_MODULE_VERSION_NAME AGT_stringify(AGT_MODULE_VERSION_FUNCTION)



#define AGT_export_family extern "C"

#define AGT_function_entry(dispatch) AGT_module_export AGT_return_type AGT_stdcall AGT_concat2(AGT_concat(_agate_api_, AGT_exported_function_name), __##dispatch)

#define PP_AGT_impl_DELAY_indirect(...) __VA_ARGS__
#define PP_AGT_impl_DELAY PP_AGT_impl_DELAY_indirect
#define AGT_delay_macro(...) PP_AGT_impl_DELAY(__VA_ARGS__)



#define AGT_ctx_api(func, ctx) ((ctx)->exports->_pfn_##func)
#define AGT_api(func) ((::agt::get_ctx())->exports->_pfn_##func)



#if defined(AGT_BUILD_SHARED) && defined(AGT_BUILDING_MODULE)
#include "module_fwd.hpp"
#endif







// Macro commands/helpers

#define PP_AGT_impl_VIRTUAL_OBJECT_TYPE_ object

#define PP_AGT_impl_VIRTUAL_OBJECT_TYPE_ref_counted rc_object

#define PP_AGT_impl_VIRTUAL_OBJECT_TYPE_extends_indirect(type) type
#define PP_AGT_impl_VIRTUAL_OBJECT_TYPE_extends PP_AGT_impl_VIRTUAL_OBJECT_TYPE_extends_indirect

#define PP_AGT_impl_OBJECT_BASE(...) AGT_concat(PP_AGT_impl_, VIRTUAL_OBJECT_TYPE_##__VA_ARGS__)
#define PP_AGT_impl_SPECIALIZE_OBJECT_ENUM(objType) \
  template <> \
  struct ::agt::impl::obj_types::object_type_id<objType> { \
    inline constexpr static object_type value = object_type::objType; \
  }

#define PP_AGT_impl_SPECIALIZE_OBJECT_ENUM_RANGE(objType) \
  template <> \
  struct ::agt::impl::obj_types::object_type_range<objType> { \
    inline constexpr static object_type minValue = object_type::objType##_begin; \
    inline constexpr static object_type maxValue = object_type::objType##_end;   \
  }


#define AGT_type_id_of(objType)  ::agt::impl::obj_types::object_type_id<objType>::value
#define AGT_type_id_min(objType) ::agt::impl::obj_types::object_type_range<objType>::minValue
#define AGT_type_id_max(objType) ::agt::impl::obj_types::object_type_range<objType>::maxValue

#include "agate/flags.hpp"
#include "export_table.hpp"
#include "fwd.hpp"

#endif//AGATE_INTERNAL_CONFIG_HPP
