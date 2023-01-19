//
// Created by maxwe on 2023-01-13.
//

#ifndef AGATE_AGATE_MODULE_PLUGINS_H
#define AGATE_AGATE_MODULE_PLUGINS_H

#if !defined(AGT_BUILDING_CORE_LIBRARY)
// Defined by user modules, as they are loaded dynamically,
// and thus the header version used in the compilation of the
// module is not at all guaranteed to match the header
// version used by any libraries that may use it. Actual
// struct sizes are provided to modules during load time as
// attributes in agtmod_load_info_t.

// A guard is needed before defining this though, as the
// core library imports this file during compilation.
# define AGT_DISABLE_STATIC_STRUCT_SIZES
#endif


#include <agate/core.h>


#if AGT_compiler_msvc
# if !defined(AGT_STATIC_IMPORT_USER_MODULE)
#  define AGT_module_api __declspec(dllexport)
# else
#  define AGT_module_api __declspec(dllimport)
# endif
#else
# define AGT_module_api __attribute__((visibility("default")))
#endif




AGT_begin_c_namespace


typedef agt_u64_t agtmod_id_t;


typedef struct agtmod_pool_st* agtmod_pool_t;


typedef struct agtmod_loader_st* agtmod_loader_t;


typedef enum agtmod_hook_id_t {
  AGTMOD_HOOK_PRE_SEND,
  AGTMOD_HOOK_POST_SEND,

} agtmod_hook_id_t;

typedef struct agtmod_hook_t {
  agtmod_hook_id_t id;
  union {
    void (* AGT_stdcall preSend)(void* moduleState, agt_ctx_t ctx, agt_agent_t self, agt_agent_t target);
    void (* AGT_stdcall postSend)(void* moduleState);
  };
} agtmod_hook_t;

typedef struct agtmod_object_header_t {
  agt_u32_t reserved;
} agtmod_object_header_t;

typedef struct agtmod_loader_interface_t {
  void (* setState)(agtmod_loader_t loader, void* state);
  void (* addHooks)(agtmod_loader_t loader, const agtmod_hook_t* pHooks, size_t hookCount);
  void (* )();
} agtmod_loader_interface_t;

typedef struct agtmod_load_info_t {
  agt_name_t                       name;
  const agt_attr_t*                attributes;
  agt_size_t                       attributeCount;
  const void*                      moduleFilePath; ///< Not necessarily const char*, to accommodate for goddamn utf16 encoded windows paths.....
  agt_attr_type_t                  filePathType;   ///< STRING or WIDE_STRING, determines char type of moduleFilePath.
} agtmod_load_info_t;

typedef enum   agtmod_log_severity_t {

} agtmod_log_severity_t;

typedef struct agtmod_log_info_t {
  agt_status_t                 status;
  agtmod_log_severity_t        severity;
  const char*                  message;
  agt_size_t                   messageLength;
  void**                       pObjects;
  size_t                       objectCount;
} agtmod_log_info_t;

typedef struct agtmod_export_table_t {
  void         (* AGT_stdcall ctxLog)(agt_ctx_t ctx, const agtmod_log_info_t* logInfo);

  agt_status_t (* AGT_stdcall ctxGetPool)(agt_ctx_t ctx, size_t objSize, agtmod_pool_t* pPool);
  void*        (* AGT_stdcall ctxPoolAlloc)(agtmod_pool_t pool);
  agt_status_t (* AGT_stdcall ctxAlloc)(agt_ctx_t ctx, size_t objSize, void** pObj);
  void         (* AGT_stdcall ctxRelease)(void* object);

  agt_status_t (* AGT_stdcall newPool)(agt_ctx_t ctx, agt_pool_t* pPool, agt_size_t fixedSize, agt_pool_flags_t flags);
  agt_status_t (* AGT_stdcall resetPool)(agt_pool_t pool);
  void         (* AGT_stdcall destroyPool)(agt_pool_t pool);

} agtmod_export_table_t;

typedef const agtmod_export_table_t* agtmod_exports_t;

typedef struct agtmod_info_t {
  agtmod_id_t      id;
  agtmod_exports_t exports;
  agt_instance_t   instance;
} agtmod_info_t;

// AGT_module_api functions must be defined and exported by the module in question. These are not functions to be *called* by a given module. Rather, these are functions that the agate core will call when initializing/finalizing a user module.



// This will be called initially, to ascertain that the module version falls within the requested range.
AGT_module_api int          agtmod_get_version() AGT_noexcept;

// This will subsequently be called, during which the module may make calls to the function pointers in interface to declare its capabilities.
AGT_module_api agt_status_t agtmod_load(agtmod_loader_t loader, const agtmod_loader_interface_t* interface, const agtmod_load_info_t* loadInfo, const void* initUserData) AGT_noexcept;

// This will be called after instance initialization has finished successfully, providing the module with the function interface it may use to interact with the Agate instance, allowing access to both the public interface, but also a limited private interface.
AGT_module_api void         agtmod_set_info(void* moduleState, const agtmod_info_t* pInfo) AGT_noexcept;

// This will be called when the module is finalized, typically when the instance is finalized. Intended to allow the module to clean up and release any resources it may have been holding onto.
AGT_module_api void         agtmod_finalize(void* moduleState, agt_instance_t instance) AGT_noexcept;



AGT_end_c_namespace

#endif//AGATE_AGATE_MODULE_PLUGINS_H
