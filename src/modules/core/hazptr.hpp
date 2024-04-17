//
// Created by maxwe on 2024-04-15.
//

#ifndef AGATE_INTERNAL_CORE_HAZPTR_HPP
#define AGATE_INTERNAL_CORE_HAZPTR_HPP

#include "config.hpp"
#include "impl/object_defs.hpp"

AGT_api_object(agt_hazptr_st, hazptr, extends(agt::object), aligned(AGT_CACHE_LINE)) {
  uint32_t       bits;
  const void*    ptr;
  agt_hazptr_st* next;
  agt_hazptr_st* nextAvail;
};


AGT_api_object(agt_hazptr_domain_st, hazptr_domain) {

};



namespace agt {
  struct hazptr_base;
  struct hazptr_obj_list;

  struct hazptr_obj {
    void      (* reclaimFunc)(agt_ctx_t ctx, hazptr_obj* obj, hazptr_obj_list& );
    hazptr_base* nextHazptrObj;
    uintptr_t    domainTag;
  };


  struct hazptr_obj_list {

  };

  struct hazptr_base : object {
    void      (* reclaimFunc)(agt_ctx_t ctx, hazptr_base* obj, hazptr_obj_list& );
    hazptr_base* nextHazptrObj;
    uintptr_t    domainTag;
  };


}

#endif//AGATE_INTERNAL_CORE_HAZPTR_HPP
