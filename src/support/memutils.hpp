                                    //
// Created by maxwe on 2021-06-18.
//

#ifndef JEMSYS_MEMUTILS_INTERNAL_HPP
#define JEMSYS_MEMUTILS_INTERNAL_HPP

#include "agate.h"


                                    namespace {
  struct memory_desc{
    jem_size_t size;
    jem_size_t alignment;
  };

  template <typename T>
  inline constexpr static memory_desc default_memory_requirements = { sizeof(T), alignof(T) };

  AGT_forceinline void align_size(memory_desc& mem) noexcept {
    mem.size = ((mem.size - 1) | (mem.alignment - 1)) + 1;
  }
}

#endif//JEMSYS_MEMUTILS_INTERNAL_HPP
