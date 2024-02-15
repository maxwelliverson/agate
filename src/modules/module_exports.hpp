//
// Created by maxwe on 2023-08-14.
//

#ifndef AGATE_MODULE_EXPORTS_HPP
#define AGATE_MODULE_EXPORTS_HPP

#include "config.hpp"
#include "../init.hpp"

#if !defined(AGT_BUILDING_STATIC_LOADER)
#if defined(_WIN32)
# define AGT_export __declspec(dllexport)
#else
# define AGT_export __attribute__((visibility="public"))
#endif
#else
# define AGT_export
#endif

#define AGT_define_get_exports(_modExp, _attr) extern "C" AGT_export void agatelib_get_exports(agt::init::module_exports& _modExp, const agt::init::attributes& _attr) noexcept

#define AGT_add_export(kind, name, func) moduleExports.add_export(kind, "agt_" #name, &(func), offsetof(agt::export_table, _pfn_##name))

#define AGT_add_virtual_export(name) moduleExports.add_virtual_export("agt_" #name, &(agt_##name))

#define AGT_add_private_export(name, func) AGT_add_export(agt::init::ePrivateExport, name, func)

#define AGT_add_public_export(name, func) AGT_add_export(agt::init::ePublicExport, name, func)


AGT_define_get_exports(moduleExports, attributes);

#endif//AGATE_MODULE_EXPORTS_HPP
