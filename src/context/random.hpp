//
// Created by maxwe on 2022-03-08.
//

#ifndef AGATE_INTERNAL_CONTEXT_RANDOM_HPP
#define AGATE_INTERNAL_CONTEXT_RANDOM_HPP


#include "fwd.hpp"

namespace agt {

  struct rng_seeder{
    agt_u64_t seed;
  };

  struct rng32 {
    agt_u32_t x[4];
  };
  struct rng64 {
    agt_u64_t x[4];
  };

  void      initRng(rng32* rng, rng_seeder* seeder) noexcept;
  void      initRng(rng64* rng, rng_seeder* seeder) noexcept;

  agt_u32_t rngNext(rng32* rng) noexcept;
  agt_u64_t rngNext(rng64* rng) noexcept;
}


#endif //AGATE_INTERNAL_CONTEXT_RANDOM_HPP
