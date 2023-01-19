//
// Created by maxwe on 2023-01-16.
//

#ifndef AGATE_INTERNAL_TIME_HPP
#define AGATE_INTERNAL_TIME_HPP


#include "config.hpp"

#include <immintrin.h>



union win32_large_integer {
  agt_u64_t quadPart;
  struct {
    agt_u32_t lowPart;
    agt_u32_t highPart;
  };
};

extern "C" int QueryPerformanceCounter(win32_large_integer* pLargeInteger);




namespace agt {

  bool      has_static_time_unit() noexcept;

  agt_u64_t get_dynamic_time_unit() noexcept;

  AGT_forceinline static agt_timestamp_t get_fast_timestamp() noexcept {
    return __rdtsc();
  }

  AGT_forceinline static agt_timestamp_t get_stable_timestamp() noexcept {
    win32_large_integer largeInteger;
    QueryPerformanceCounter(&largeInteger);
    return largeInteger.quadPart;
  }
}

#endif//AGATE_INTERNAL_TIME_HPP
