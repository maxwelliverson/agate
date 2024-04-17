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



#if defined(__GNUC__)
# define AGT_counted_by(name) __attribute__((counted_by(name)))
# define AGT_use_only_gprs __attribute((target("general-regs-only")))
#else
# define AGT_counted_by(name)
# define AGT_use_only_gprs
#endif






#define AGT_export_family extern "C"

#define AGT_function_entry(dispatch) AGT_module_export AGT_return_type AGT_stdcall AGT_concat2(AGT_concat(_agate_api_, AGT_exported_function_name), __##dispatch)

#define PP_AGT_impl_DELAY_indirect(...) __VA_ARGS__
#define PP_AGT_impl_DELAY PP_AGT_impl_DELAY_indirect
#define AGT_delay_macro(...) PP_AGT_impl_DELAY(__VA_ARGS__)



#define AGT_ctx_api(func, ctx) ((ctx)->exports->_pfn_##func)
#define AGT_api(func) ((::agt::get_ctx())->exports->_pfn_##func)


#if defined(__cpp_if_consteval)
# define AGT_if_consteval if consteval
# define AGT_if_not_consteval if not consteval
#else
# define AGT_if_consteval if (std::is_constant_evaluated())
# define AGT_if_not_consteval if (!std::is_constant_evaluated())
#endif



#if defined(AGT_BUILD_SHARED) && defined(AGT_BUILDING_MODULE)
#include "module_fwd.hpp"
#endif



#define PP_AGT_impl_TAKE_8TH(a, b, c, d, e, f, g, h, ...) h
#define PP_AGT_impl_COUNT_ARGS6_indirect(...) PP_AGT_impl_TAKE_8TH(,##__VA_ARGS__, 6, 5, 4, 3, 2, 1, 0)
#define PP_AGT_impl_COUNT_ARGS6 PP_AGT_impl_COUNT_ARGS6_indirect



#define PP_AGT_impl_FOREACH_iter6(macro, emptyMacro, a, ...) macro(a) PP_AGT_impl_FOREACH_iter5(macro, emptyMacro, __VA_ARGS__)
#define PP_AGT_impl_FOREACH_iter5(macro, emptyMacro, a, ...) macro(a) PP_AGT_impl_FOREACH_iter4(macro, emptyMacro, __VA_ARGS__)
#define PP_AGT_impl_FOREACH_iter4(macro, emptyMacro, a, ...) macro(a) PP_AGT_impl_FOREACH_iter3(macro, emptyMacro, __VA_ARGS__)
#define PP_AGT_impl_FOREACH_iter3(macro, emptyMacro, a, ...) macro(a) PP_AGT_impl_FOREACH_iter2(macro, emptyMacro, __VA_ARGS__)
#define PP_AGT_impl_FOREACH_iter2(macro, emptyMacro, a, ...) macro(a) PP_AGT_impl_FOREACH_iter1(macro, emptyMacro, __VA_ARGS__)
#define PP_AGT_impl_FOREACH_iter1(macro, emptyMacro, a) macro(a)
#define PP_AGT_impl_FOREACH_iter0(macro, emptyMacro) emptyMacro()


#define PP_AGT_impl_FOREACH_LAZY_CONCAT2(a, b) a##b
#define PP_AGT_impl_FOREACH_LAZY_CONCAT(...) PP_AGT_impl_FOREACH_LAZY_CONCAT2(__VA_ARGS__)
#define PP_AGT_impl_FOREACH_indirect2(macro, emptyMacro, count, ...) PP_AGT_impl_FOREACH_LAZY_CONCAT(PP_AGT_impl_FOREACH_iter, count)(macro, emptyMacro, ##__VA_ARGS__)
#define PP_AGT_impl_FOREACH_indirect PP_AGT_impl_FOREACH_indirect2
#define PP_AGT_impl_FOREACH(macro, emptyMacro, ...) PP_AGT_impl_FOREACH_indirect(macro, emptyMacro, PP_AGT_impl_COUNT_ARGS6(__VA_ARGS__), ##__VA_ARGS__)



// Macro commands/helpers

// #define PP_AGT_impl_VIRTUAL_OBJECT_TYPE_ object

#define PP_AGT_impl_VIRTUAL_OBJECT_TYPE_ref_counted agt::rc_object

#define PP_AGT_impl_VIRTUAL_OBJECT_TYPE_extends_indirect(type) type
#define PP_AGT_impl_VIRTUAL_OBJECT_TYPE_extends PP_AGT_impl_VIRTUAL_OBJECT_TYPE_extends_indirect

#define PP_AGT_impl_VIRTUAL_OBJECT_TYPE_aligned(...)


#define PP_AGT_impl_GET_SUPERTYPE(a) AGT_concat(PP_AGT_impl_VIRTUAL_OBJECT_TYPE_, a)
#define PP_AGT_impl_DEFAULT_SUPERTYPE() agt::object

#define PP_AGT_impl_OBJECT_BASE(...) PP_AGT_impl_FOREACH(PP_AGT_impl_GET_SUPERTYPE, PP_AGT_impl_DEFAULT_SUPERTYPE, ##__VA_ARGS__)

// #define PP_AGT_impl_OBJECT_ATTR_

#define PP_AGT_impl_OBJECT_ATTR_ref_counted

#define PP_AGT_impl_OBJECT_ATTR_extends(...)

#define PP_AGT_impl_OBJECT_ATTR_aligned_indirect(val) alignas(val)
#define PP_AGT_impl_OBJECT_ATTR_aligned PP_AGT_impl_OBJECT_ATTR_aligned_indirect


#define PP_AGT_impl_GET_OBJECT_ATTR(a) AGT_concat(PP_AGT_impl_OBJECT_ATTR_, a)
#define PP_AGT_impl_DEFAULT_OBJECT_ATTR()
#define PP_AGT_impl_OBJECT_ATTR(...) PP_AGT_impl_FOREACH(PP_AGT_impl_GET_OBJECT_ATTR, PP_AGT_impl_DEFAULT_OBJECT_ATTR, ##__VA_ARGS__)


#define PP_AGT_impl_OBJECT_ENUM(v) AGT_concat(PP_AGT_impl_OBJECT_ENUM_, v)


#define PP_AGT_impl_OBJECT_ENUM_value(objType)  \
  inline constexpr static object_type value = object_type::objType;
#define PP_AGT_impl_OBJECT_ENUM_range(from, to)  \
  inline constexpr static object_type minValue = object_type::from; \
  inline constexpr static object_type maxValue = object_type::to;
#define PP_AGT_impl_DEF_OBJECT_ENUM(objType, ...) \
  template <> \
  struct ::agt::impl::obj_types::object_type_id<objType> { \
    PP_AGT_impl_FOREACH(PP_AGT_impl_OBJECT_ENUM, , ##__VA_ARGS__) \
  }


// #define PP_AGT_impl_OBJECT_BASE(...) AGT_concat(PP_AGT_impl_, VIRTUAL_OBJECT_TYPE_##__VA_ARGS__)
#define PP_AGT_impl_SPECIALIZE_VIRTUAL_OBJECT_ENUM(apiType, objType) \
  PP_AGT_impl_DEF_OBJECT_ENUM(apiType, value(objType), range(objType##_begin, objType##_end))

#define PP_AGT_impl_SPECIALIZE_OBJECT_ENUM(apiType, objType) \
  PP_AGT_impl_DEF_OBJECT_ENUM(apiType, value(objType))

#define PP_AGT_impl_SPECIALIZE_ABSTRACT_OBJECT_ENUM(apiType, objType) \
  PP_AGT_impl_DEF_OBJECT_ENUM(apiType, range(objType##_begin, objType##_end))


#define AGT_type_id_of(objType)  ::agt::impl::obj_types::object_type_id<objType>::value
#define AGT_type_id_min(objType) ::agt::impl::obj_types::object_type_range<objType>::minValue
#define AGT_type_id_max(objType) ::agt::impl::obj_types::object_type_range<objType>::maxValue

#include "agate/flags.hpp"
#include "export_table.hpp"
#include "fwd.hpp"

#endif//AGATE_INTERNAL_CONFIG_HPP
