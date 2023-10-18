//
// Created by maxwe on 2023-01-16.
//

#ifndef AGATE_INTERNAL_TIME_HPP
#define AGATE_INTERNAL_TIME_HPP


#include "config.hpp"

#include <immintrin.h>


// Ideally would like to find a way to not include the entire windows header in a very basic,
// common utility header like this one.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>




namespace agt {

  bool      has_static_time_unit() noexcept;

  agt_u64_t get_dynamic_time_unit(agt_ctx_t ctx) noexcept;

  AGT_forceinline static agt_timestamp_t get_fast_timestamp() noexcept {
    return __rdtsc();
  }

  AGT_forceinline static agt_timestamp_t get_stable_timestamp() noexcept {
    LARGE_INTEGER largeInteger;
    QueryPerformanceCounter(&largeInteger);
    return largeInteger.QuadPart;
  }
}

#endif//AGATE_INTERNAL_TIME_HPP
