//
// Created by maxwe on 2024-04-15.
//

#ifndef AGATE_HAZPTR_H
#define AGATE_HAZPTR_H

#include <agate/core.h>

AGT_begin_c_namespace

#define AGT_DEFAULT_HAZPTR_DOMAIN ((agt_hazptr_domain_t)AGT_NULL_HANDLE)
#define AGT_NULL_HAZPTR ((agt_hazptr_t)AGT_NULL_HANDLE)


typedef struct agt_hazptr_domain_st* agt_hazptr_domain_t; // domain*
typedef struct agt_hazptr_st*        agt_hazptr_t;        // hazptr_rec*
typedef void*                        agt_hazptr_obj_t;    // hazptr_obj*


typedef void (* agt_hazptr_retire_func_t)(agt_ctx_t ctx, void* obj, void* userData);


typedef struct agt_hazptr_domain_desc_t {
  agt_name_t name;
  agt_u32_t  maxReclaimCount;
} agt_hazptr_domain_desc_t;



AGT_core_api agt_status_t AGT_stdcall agt_new_hazptr_domain(agt_ctx_t ctx, agt_hazptr_domain_t* pDomain, const agt_hazptr_domain_desc_t* pDomainDesc) AGT_noexcept;

AGT_core_api void         AGT_stdcall agt_close_hazptr_domain(agt_ctx_t ctx, agt_hazptr_domain_t domain) AGT_noexcept;



AGT_core_api void*        AGT_stdcall agt_alloc_hazptr_obj(agt_ctx_t ctx, size_t size, size_t alignment) AGT_noexcept;

AGT_core_api
AGT_callback_param(4, 1, 2, 5)
void         AGT_stdcall agt_retire_hazptr_obj(agt_ctx_t ctx, void* obj, agt_hazptr_domain_t domain, agt_hazptr_retire_func_t retireFunc, void* userData) AGT_noexcept;




// The following functions all have basically no error checking, because they are intended to be *fast*
AGT_core_api agt_hazptr_t AGT_stdcall agt_new_hazptr(agt_ctx_t ctx) AGT_noexcept;

AGT_core_api void         AGT_stdcall agt_destroy_hazptr(agt_ctx_t ctx, agt_hazptr_t hazptr) AGT_noexcept;

AGT_core_api void         AGT_stdcall agt_set_hazptr(agt_hazptr_t hazptr, void* obj) AGT_noexcept;





AGT_end_c_namespace

#endif//AGATE_HAZPTR_H
