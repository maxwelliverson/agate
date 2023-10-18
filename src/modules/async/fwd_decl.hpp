//
// Created by maxwe on 2023-01-26.
//

#ifndef AGATE_INTERNAL_ASYNC_FWD_DECL_HPP
#define AGATE_INTERNAL_ASYNC_FWD_DECL_HPP


#include "config.hpp"
#include "agate/cast.hpp"
#include "core/object.hpp"
#include "core/rc.hpp"


namespace agt {
  AGT_BITFLAG_ENUM(async_flags, agt_u32_t) {
      eUnbound   = 0x0,
      eBound     = 0x100,
      eReady     = 0x200,
      eWaiting   = 0x400
  };


  inline constexpr async_flags eAsyncNoFlags = { };
  inline constexpr async_flags eAsyncUnbound = async_flags::eUnbound;
  inline constexpr async_flags eAsyncBound   = async_flags::eBound;
  inline constexpr async_flags eAsyncReady   = async_flags::eBound | async_flags::eReady;
  inline constexpr async_flags eAsyncWaiting = async_flags::eBound | async_flags::eWaiting;


  inline constexpr agt_u64_t IncNonWaiterRef = 0x0000'0000'0000'0001ULL;
  inline constexpr agt_u64_t IncWaiterRef    = 0x0000'0001'0000'0001ULL;
  inline constexpr agt_u64_t DecNonWaiterRef = 0xFFFF'FFFF'FFFF'FFFFULL;
  inline constexpr agt_u64_t DecWaiterRef    = 0xFFFF'FFFE'FFFF'FFFFULL;
  inline constexpr agt_u64_t DetachWaiter    = 0xFFFF'FFFF'0000'0000ULL;
  inline constexpr agt_u64_t AttachWaiter    = 0x0000'0001'0000'0000ULL;

  inline constexpr agt_u64_t ArrivedResponse = 0x0000'0000'0000'0001ULL;
  inline constexpr agt_u64_t DroppedResponse = 0x0000'0001'0000'0001ULL;
}


extern "C" {

struct agt_async_st {
  agt_ctx_t         context;
  agt::async_flags  flags;
  agt_status_t      status;
  agt_u32_t         desiredResponseCount;
  agt::async_key_t  dataKey;
  agt::async_data_t data;
  agt_u32_t         timeoutConstantMultiplier;
  agt_u32_t         timeoutConstantShift;
  // Version 1.0 end
};

}

static_assert(sizeof(agt_async_t) == AGT_ASYNC_STRUCT_SIZE);
static_assert(alignof(agt_async_t) == AGT_ASYNC_STRUCT_ALIGNMENT);




#endif//AGATE_INTERNAL_ASYNC_FWD_DECL_HPP
