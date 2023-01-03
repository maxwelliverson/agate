//
// Created by maxwe on 2022-12-21.
//

#ifndef AGATE_INTERNAL_CONFIG_HPP
#define AGATE_INTERNAL_CONFIG_HPP



#include "agate.h"


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

#if defined(AGT_BUILD_SHARED)
# if defined(AGT_BUILD_STATIC)
#  error Compilation may not define both AGT_BUILD_SHARED and AGT_BUILD_STATIC.
# endif
# define AGT_api_call(func, ctx) ((ctx)->exports->_##func)
#else
# if !defined(AGT_BUILD_STATIC)
#  error Compilation must define either AGT_BUILD_SHARED or AGT_BUILD_STATIC.
# endif
# define AGT_api_call(func, ctx) AGT_delay_macro(agt_##func)
#endif



#if defined(AGT_BUILD_SHARED) && defined(AGT_BUILDING_MODULE)
#include "module_fwd.hpp"
#endif







// Macro commands/helpers

#define PP_AGT_impl_VIRTUAL_OBJECT_TYPE_ object

#define PP_AGT_impl_VIRTUAL_OBJECT_TYPE_extends_indirect(type) type
#define PP_AGT_impl_VIRTUAL_OBJECT_TYPE_extends PP_AGT_impl_VIRTUAL_OBJECT_TYPE_extends_indirect

#define PP_AGT_impl_OBJECT_BASE(...) AGT_concat(PP_AGT_impl_, VIRTUAL_OBJECT_TYPE_##__VA_ARGS__)
#define PP_AGT_impl_SPECIALIZE_OBJECT_ENUM(objType) \
  template <> \
  struct ::agt::impl::obj_types::object_type_id<objType> { \
    inline constexpr static object_type value = object_type::objType; \
  }





#include "agate/flags.hpp"
#include "fwd.hpp"
#include "agate/export_table.hpp"

#endif//AGATE_INTERNAL_CONFIG_HPP
