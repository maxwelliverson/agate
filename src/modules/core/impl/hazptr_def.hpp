//
// Created by maxwe on 2024-04-18.
//

#ifndef AGATE_INTERNAL_CORE_IMPL_HAZPTR_DEF_HPP
#define AGATE_INTERNAL_CORE_IMPL_HAZPTR_DEF_HPP


#include "config.hpp"
#include "object_defs.hpp"

#include "agate/atomic.hpp"

#include <span>

namespace agt::impl {
  struct locked_hazptr_list {
    uintptr_t head;
    agt_ctx_t lockOwner;
    agt_u32_t reenteranceCount;
  };
}

AGT_api_object(agt_hazptr_st, hazptr, extends(agt::object), aligned(AGT_CACHE_LINE)) {
  uint32_t       bits;
  const void*    ptr;
  agt_hazptr_st* next;
  agt_hazptr_st* nextAvail;
};


AGT_api_object(agt_hazptr_domain_st, hazptr_domain, ref_counted, aligned(AGT_CACHE_LINE)) {
  agt_bool_t isRetired;
  int        count;
  agt_name_t name;
  agt::impl::locked_hazptr_list list;
  hazard_obj* safeListTop;
};



namespace agt {

  // TODO: Implement hazptr_domains

  struct hazard_obj;
  struct hazard_obj_list;
  struct hazard_obj_sized_list;


  struct hazard_obj : object {
    union {
      void      (* reclaimFunc)(agt_ctx_t ctx, hazard_obj * obj, hazard_obj_sized_list & );
      agt_hazptr_retire_func_t userFunc;
    };

    hazard_obj * nextHazptrObj;
    uintptr_t    domainTag;
  };

  struct hazard_obj_list {
    hazard_obj * head;
    hazard_obj * tail;
  };

  struct hazard_obj_sized_list {
    hazard_obj_list list;
    int size;
  };


  AGT_forceinline static bool empty(const hazard_obj_list& list) noexcept {
    return list.head == nullptr;
  }

  AGT_forceinline static bool empty(const hazard_obj_sized_list& list) noexcept {
    return list.list.head == nullptr;
  }

  AGT_forceinline static void clear(hazard_obj_list& list) noexcept {
    list.head = nullptr;
    list.tail = nullptr;
  }

  AGT_forceinline static void clear(hazard_obj_sized_list& list) noexcept {
    clear(list.list);
    list.size = 0;
  }

  AGT_forceinline static void push(hazard_obj_list& list, hazard_obj* head, hazard_obj* tail) noexcept {
    AGT_invariant( head != nullptr && tail != nullptr );
    tail->nextHazptrObj = nullptr;
    if (list.tail)
      list.tail->nextHazptrObj = head;
    else
      list.head = head;
    list.tail = tail;
  }

  AGT_forceinline static void push(hazard_obj_list& list, hazard_obj* obj) noexcept {
    obj->nextHazptrObj = nullptr;
    if (list.tail)
      list.tail->nextHazptrObj = obj;
    else
      list.head = obj;
    list.tail = obj;
  }

  AGT_forceinline static void push(hazard_obj_sized_list& list, hazard_obj* obj) noexcept {
    push(list.list, obj);
    ++list.size;
  }

  AGT_forceinline static void splice(hazard_obj_list& dst, hazard_obj_list& src) noexcept {
    if (dst.tail)
      dst.tail->nextHazptrObj = src.head;
    else
      dst.head = src.head;
    dst.tail = src.tail;
    src.head = nullptr;
    src.tail = nullptr;
  }

  AGT_forceinline static void splice(hazard_obj_sized_list& dst, hazard_obj_sized_list& src) noexcept {
    dst.size += src.size;
    src.size = 0;
    splice(dst.list, src.list);
  }

}

#endif//AGATE_INTERNAL_CORE_IMPL_HAZPTR_DEF_HPP
