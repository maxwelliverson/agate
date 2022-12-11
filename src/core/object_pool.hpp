//
// Created by maxwe on 2022-12-05.
//

#ifndef AGATE_OBJECT_POOL_HPP
#define AGATE_OBJECT_POOL_HPP

#include "fwd.hpp"

#include <bit>
#include <new>

namespace agt {

  struct object_pool_chunk;

  using object_chunk_t = object_pool_chunk*;

  namespace impl {
    struct dual_pool_chunks {
      size_t          slotSize;
      object_chunk_t  allocChunk;
      object_chunk_t* stackBase;
      object_chunk_t* stackTop;
      object_chunk_t* stackHead;
      object_chunk_t* stackFullHead;
      object_chunk_t* stackEmptyTail;
      
      // pos(chunk)   := position of chunk in stack
      // empty(chunk) := true IFF chunk has no active allocations
      // full(chunk)  := true IFF chunk has no free slots
      //
      // invariants:
      //
      //     stackBase < stackTop
      //
      //     stackBase <= stackFullHead <= stackEmptyTail <= stackHead <= stackTop
      //
      //     stackBase <= pos(chunk) < stackHead
      //
      //     full(chunk)  IFF pos(chunk) < stackFullHead
      //
      //     empty(chunk) IFF stackEmptyTail <= pos(chunk)
      //
      //     stackBase == stackFullHead  IFF no chunks are full
      //
      //     stackEmptyTail == stackHead IFF no chunks are empty
    };

    struct object_pool_common {
      agt_ctx_t       context;
      agt_u32_t       flags;
      agt_u16_t       slotsPerChunk; // totalSlotsPerChunk - slots occupied by chunk header
      agt_u16_t       totalSlotsPerChunk;
      size_t          slotSize;
      size_t          chunkSize;
    };
    struct object_pool_stack {
      size_t          slotSize;
      object_chunk_t  allocChunk;
      object_chunk_t* stackBase;
      object_chunk_t* stackTop;
      object_chunk_t* stackHead;
      object_chunk_t* stackFullHead;
      object_chunk_t* stackEmptyTail;
    };

    struct object_pool_solo : object_pool_common {
      object_pool_stack stack;
    };

    struct object_pool_dual : object_pool_common {
      object_pool_stack smallStack;
      object_pool_stack largeStack;
    };
  }

  struct object_pool {
    agt_ctx_t       context;
    size_t          slotSize;
    agt_u16_t       slotsPerChunk; // totalSlotsPerChunk - slots occupied by chunk header
    agt_u16_t       totalSlotsPerChunk;
    agt_u32_t       ownerThreadId;
    size_t          chunkSize;
    object_chunk_t  allocChunk;
    object_chunk_t* chunkStackBase;
    object_chunk_t* chunkStackTop;
    object_chunk_t* chunkStackHead;
    object_chunk_t* chunkStackFullHead;
  };
  
  struct object_dual_pool {
    agt_ctx_t              context;
    agt_u16_t              slotsPerChunk; // totalSlotsPerChunk - slots occupied by chunk header, either 1 or 2.
    agt_u16_t              totalSlotsPerChunk;
    agt_u32_t              ownerThreadId;
    size_t                 totalSlotSize;
    size_t                 chunkSize;
    
    impl::dual_pool_chunks small;
    impl::dual_pool_chunks large;
  };



  void           init_pool(agt_ctx_t ctx, agt_u32_t threadId, object_pool* pool, size_t slotSize, size_t slotsPerChunk) noexcept;

  void           init_dual_pool(agt_ctx_t ctx, agt_u32_t threadId, object_dual_pool* pool, size_t smallSlotSize, size_t largeSlotSize, size_t slotsPerChunk) noexcept;

  void           destroy_pool(object_pool* pool) noexcept;

  void           destroy_dual_pool(object_dual_pool* pool) noexcept;

  void           trim_pool(object_pool* pool) noexcept;

  void           trim_dual_pool(object_dual_pool* pool) noexcept;



  /*template <std::derived_from<object> Obj> requires (!std::same_as<std::remove_cv_t<Obj>, object>)
  Obj*           retain(Obj* obj) noexcept {
    constexpr static size_t CeilSize = std::bit_ceil(sizeof(Obj));
    return static_cast<Obj*>(retain_obj(obj, CeilSize));
  }

  template <std::derived_from<object> Obj> requires (!std::same_as<std::remove_cv_t<Obj>, object>)
  void           release(Obj* obj) noexcept {
    constexpr static size_t CeilSize = std::bit_ceil(sizeof(Obj));
    release_obj(obj, CeilSize);
  }

  template <std::derived_from<object> Obj> requires (!std::same_as<std::remove_cv_t<Obj>, object>)
  void           free(thread_context* ctx, Obj* obj) noexcept {
    constexpr static size_t CeilSize = std::bit_ceil(sizeof(Obj));
    free_obj(ctx, obj, CeilSize);
  }*/
}




#endif//AGATE_OBJECT_POOL_HPP
