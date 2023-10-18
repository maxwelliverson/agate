//
// Created by maxwe on 2023-01-03.
//

#ifndef AGATE_THREAD_ID_HPP
#define AGATE_THREAD_ID_HPP

#include "config.hpp"


#include <immintrin.h>


namespace agt {
  [[nodiscard]] AGT_forceinline uintptr_t get_thread_id() noexcept {
    // TODO: Couch use of platform specific intrinsics in preprocessor conditionals
    //       if they need to be used in header files

    // Reads the value of the GS segment register, which on x64 Windows, holds the
    // linear address offset of the thread control block. Definitely hacky, but
    // super fast.
    return _readgsbase_u64();
  }

  [[nodiscard]] AGT_forceinline uintptr_t get_ctx_thread_id(agt_ctx_t ctx) noexcept;
}

#endif//AGATE_THREAD_ID_HPP
