//
// Created by maxwe on 2023-08-14.
//

#ifndef AGATE_MODULE_EXPORTS_HPP
#define AGATE_MODULE_EXPORTS_HPP

#include "config.hpp"
#include "../init.hpp"

#define AGT_define_get_exports(_modExp, _attr) extern "C" void agatelib_get_exports(agt::init::module_exports& _modExp, const agt::init::attributes& _attr) noexcept

#define AGT_add_export(name, func) moduleExports.add_export("agt_" #name, &(func), offsetof(agt::export_table, _pfn_##name))

AGT_define_get_exports(moduleExports, attributes);

#endif//AGATE_MODULE_EXPORTS_HPP
