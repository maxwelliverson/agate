//
// Created by maxwe on 2022-03-08.
//

#include "random.hpp"
#include <immintrin.h>


namespace {
  // Pulled from https://en.wikipedia.org/wiki/Xorshift#Initialization
  inline agt_u64_t nextSeed(agt::rng_seeder* seeder) noexcept {
    agt_u64_t result = (seeder->seed += 0x9E3779B97f4A7C15);
    result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9;
    result = (result ^ (result >> 27)) * 0x94D049BB133111EB;
    return result ^ (result >> 31);
  }
}


void      agt::initRng(rng32 *rng, rng_seeder *seeder) noexcept {
  const agt_u64_t a = nextSeed(seeder);
  rng->x[0] = (agt_u32_t)a;
  rng->x[1] = (agt_u32_t)(a >> 32);
  const agt_u64_t b = nextSeed(seeder);
  rng->x[2] = (agt_u32_t)b;
  rng->x[3] = (agt_u32_t)(b >> 32);
}

void      agt::initRng(rng64 *rng, rng_seeder *seeder) noexcept {
  const agt_u64_t a = nextSeed(seeder);
  rng->x[0] = (agt_u32_t)a;
  rng->x[1] = (agt_u32_t)(a >> 32);
  const agt_u64_t b = nextSeed(seeder);
  rng->x[2] = (agt_u32_t)b;
  rng->x[3] = (agt_u32_t)(b >> 32);
}



agt_u32_t agt::rngNext(rng32 *rng) noexcept {
  auto& s = rng->x;
  const agt_u32_t result = s[0] + s[3];

  const agt_u32_t t = s[1] << 9;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = _rotl(s[3], 11);

  return result;
}

agt_u64_t agt::rngNext(rng64 *rng) noexcept {
  auto& s = rng->x;

  const agt_u64_t result = s[0] + s[3];

  const agt_u64_t t = s[1] << 17;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = _rotl64(s[3], 45);

  return result;
}