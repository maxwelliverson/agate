//
// Created by maxwe on 2023-01-03.
//

#ifndef AGATE_THREAD_ID_HPP
#define AGATE_THREAD_ID_HPP

#include "config.hpp"


#include <immintrin.h>


namespace agt {
  [[nodiscard]] AGT_forceinline uintptr_t get_thread_id() noexcept {
    return _readgsbase_u64();
  }

  [[nodiscard]] AGT_forceinline uintptr_t get_ctx_thread_id(agt_ctx_t ctx) noexcept;
}

#endif//AGATE_THREAD_ID_HPP
