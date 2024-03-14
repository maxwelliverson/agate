//
// Created by maxwe on 2022-12-05.
//

#ifndef AGATE_OBJECT_POOL_HPP
#define AGATE_OBJECT_POOL_HPP

#include "config.hpp"

#include "impl/pool.hpp"

#include <bit>
#include <type_traits>
#include <concepts>
#include <array>
#include <algorithm>


namespace agt {


  
  namespace impl {

    /*AGT_BITFLAG_ENUM(pool_chunk_flags, agt_u32_t) {
      user_safe       = static_cast<agt_u32_t>(thread_user_safe),
      owner_safe      = static_cast<agt_u32_t>(thread_owner_safe),
      privately_owned = 0x4,
      is_shared       = 0x8,
      is_small        = 0x10000,
      is_big          = 0x20000,
    };

    AGT_BITFLAG_ENUM(pool_chunk_allocator_flags, agt_u32_t) {
      user_safe       = static_cast<agt_u32_t>(thread_user_safe),
      owner_safe      = static_cast<agt_u32_t>(thread_owner_safe),
      privately_owned = 0x4,
      is_shared       = 0x8,
      is_dual         = 0x10,
    };

    AGT_BITFLAG_ENUM(pool_flags, agt_flags32_t) {
      user_safe                  = static_cast<agt_u32_t>(thread_user_safe),
      owner_safe                 = static_cast<agt_u32_t>(thread_owner_safe),
      is_dual                    = 0x4,
      default_allocator_is_small = 0x8,
      default_allocator_is_large = 0x10,
      privately_owned            = 0x20,
      is_shared                  = 0x40
    };*/

    /*inline constexpr static pool_chunk_flags chunk_default_flags = flags_not_set;
    inline constexpr static pool_chunk_flags chunk_user_safe     = pool_chunk_flags::user_safe;
    inline constexpr static pool_chunk_flags chunk_owner_safe    = pool_chunk_flags::owner_safe;
    inline constexpr static pool_chunk_flags chunk_is_small      = pool_chunk_flags::is_small;
    inline constexpr static pool_chunk_flags chunk_is_big        = pool_chunk_flags::is_big;
    inline constexpr static pool_chunk_flags chunk_is_dual       = chunk_is_small | chunk_is_big;
    inline constexpr static pool_chunk_flags chunk_is_private    = pool_chunk_flags::privately_owned;
    inline constexpr static pool_chunk_flags chunk_is_shared     = pool_chunk_flags::is_shared;

    inline constexpr static pool_chunk_allocator_flags pca_default_flags = flags_not_set;
    inline constexpr static pool_chunk_allocator_flags pca_user_safe     = pool_chunk_allocator_flags::user_safe;
    inline constexpr static pool_chunk_allocator_flags pca_owner_safe    = pool_chunk_allocator_flags::owner_safe;
    inline constexpr static pool_chunk_allocator_flags pca_is_dual       = pool_chunk_allocator_flags::is_dual;
    inline constexpr static pool_chunk_allocator_flags pca_is_private    = pool_chunk_allocator_flags::privately_owned;
    inline constexpr static pool_chunk_allocator_flags pca_is_shared     = pool_chunk_allocator_flags::is_shared;

    inline constexpr static pool_flags pool_default_flags = flags_not_set;
    inline constexpr static pool_flags pool_user_safe     = pool_flags::user_safe;
    inline constexpr static pool_flags pool_owner_safe    = pool_flags::owner_safe;*/

    /*struct AGT_cache_aligned pool_chunk {
      slot_list             freeSlotList;
      agt_flags32_t         flags;
      agt_u32_t             slotSize;
      agt_u32_t             weakRefCount;
      agt_u32_t             chunkSize;
      object_pool*          ownerPool;
      pool_chunk**          stackPos;
      delayed_free_list     delayedFreeList;
      agt_instance_t        instance;
      // pool_chunk_allocator* allocator;
      uintptr_t             threadId;
    };
    
    static_assert(sizeof(pool_chunk) == 64);

    using pool_chunk_t = pool_chunk*;*/

    struct private_pool : basic_pool { };

    struct locked_pool  : basic_pool { };

    struct user_pool    : basic_pool { };

    struct AGT_cache_aligned ctx_pool : basic_pool {
      agt_flags32_t flags;
      agt_i32_t     partnerPoolOffset;
      // ctx_pool* partnerPool;
    };

    template <>
    struct pool_traits<ctx_pool> {
      using chunk_type = mpsc_pool_chunk_header;
      inline constexpr static pool_chunk_type chunk_type_enum = ctx_pool_chunk;
      inline constexpr static bool is_owner_safe = false;
      inline constexpr static bool is_user_safe  = true;
      inline constexpr static bool has_deferred_release = true;
    };
    template <>
    struct pool_traits<locked_pool> {
      using chunk_type = mpmc_pool_chunk_header;
      inline constexpr static pool_chunk_type chunk_type_enum = locked_pool_chunk;
      inline constexpr static bool is_owner_safe = true;
      inline constexpr static bool is_user_safe  = true;
      inline constexpr static bool has_deferred_release = false;
    };
    template <>
    struct pool_traits<user_pool> {
      using chunk_type = mpsc_pool_chunk_header;
      inline constexpr static pool_chunk_type chunk_type_enum = user_pool_chunk;
      inline constexpr static bool is_owner_safe = false;
      inline constexpr static bool is_user_safe  = true;
      inline constexpr static bool has_deferred_release = true;
    };
    template <>
    struct pool_traits<private_pool> {
      using chunk_type = private_pool_chunk_header;
      inline constexpr static pool_chunk_type chunk_type_enum = private_pool_chunk;
      inline constexpr static bool is_owner_safe = false;
      inline constexpr static bool is_user_safe  = false;
      inline constexpr static bool has_deferred_release = false;
    };


    static_assert(sizeof(ctx_pool) == AGT_CACHE_LINE);


    template <typename PoolType>
    AGT_noinline    inline static void _push_new_chunk_to_stack(PoolType& pool, agt_instance_t inst) noexcept {
      assert( false && "Not yet implemented");
      std::unreachable();
    }

    template <typename PoolType>
    AGT_forceinline inline static void _adjust_stack_post_free(PoolType& pool, pool_chunk_t chunk) noexcept {
      assert( false && "Not yet implemented");
      std::unreachable();
    }

    template <typename PoolType>
    AGT_forceinline inline static void _adjust_stack_post_alloc(PoolType& pool, pool_chunk_t allocChunk, agt_u16_t remainingCount) noexcept {
      assert( false && "Not yet implemented");
      std::unreachable();
    }

    AGT_noinline    inline static void _destroy_pool(ctx_pool& pool) noexcept {
      for (auto pChunk = pool.stackHead - 1; pool.stackBase <= pChunk; --pChunk)
        _release_chunk(*pChunk);
      std::free(pool.stackBase);
    }


    /*AGT_forceinline static pool_chunk_t _get_alloc_chunk(ctx_pool& pool, agt_instance_t instance) noexcept {

    }*/

    
    /*struct object_chunk_allocator {
      agt_ctx_t       context;
      agt_u32_t       flags;
      agt_u16_t       slotsPerChunk; // totalSlotsPerChunk - slots occupied by chunk header
      agt_u16_t       totalSlotsPerChunk;
      size_t          chunkSize;
      size_t          totalSlotSize;
    };

    struct solo_object_chunk_allocator : object_chunk_allocator {
      object_pool* pool;
    };

    struct dual_object_chunk_allocator : object_chunk_allocator {
      object_pool* smallPool;
      object_pool* largePool;
    };*/
  /*AGT_object_type(object_pool) {
    agt_u32_t                   flags;
    impl::pool_chunk_allocator* chunkAllocator;
    size_t                      slotSize;
    impl::pool_chunk_t          allocChunk;
    impl::pool_chunk_t*         stackBase;
    impl::pool_chunk_t*         stackTop;
    impl::pool_chunk_t*         stackHead;
    impl::pool_chunk_t*         stackFullHead;
    impl::pool_chunk_t*         stackEmptyTail;
    
    // pos(chunk)   := position of chunk in stack
    // empty(chunk) := true IFF chunk has no active allocations
    // full(chunk)  := true IFF chunk has no free slots
    //
    // invariants:
    //
    //     stackBase < stackTop
    //
    //     stackBase <= stackFullHead <= stackHead <= stackTop
    //
    //     stackBase <= pos(chunk) < stackHead
    //
    //     full(chunk)  IFF pos(chunk) < stackFullHead
    //
    //     stackBase == stackFullHead  IFF no chunks are full
  };*/

    /*AGT_noinline inline void _pop_stack(object_pool& pool) noexcept {
      --pool.stackHead;

      const ptrdiff_t stackCapacity = pool.stackTop - pool.stackBase;

      // Shrink the stack capacity by half if stack size is 1/4th of the current capacity.
      if (pool.stackHead - pool.stackBase == stackCapacity / 4) [[unlikely]]
        _resize_stack(pool, stackCapacity / 2);
    }

    AGT_noinline inline void _remove_chunk_from_stack(object_pool& pool, pool_chunk_t newlyEmptyChunk, pool_chunk_t chunkToBeReleased) noexcept {

      if (chunkToBeReleased->stackPos == pool.stackHead - 1) {
        _swap(newlyEmptyChunk, *--pool.stackEmptyTail);
      }
      else {
        auto topChunk  = _top_of_stack(pool);
        *newlyEmptyChunk->stackPos = topChunk;
        topChunk->stackPos         = newlyEmptyChunk->stackPos;
        newlyEmptyChunk->stackPos  = chunkToBeReleased->stackPos;
        *newlyEmptyChunk->stackPos = newlyEmptyChunk;
      }

      instance_free_pages(_get_pool_instance(pool), chunkToBeReleased, chunkToBeReleased->chunkSize);
      _pop_stack(pool);
    }


    inline void         _move_empty_chunk_to_top(object_pool& pool, pool_chunk_t chunk, pool_chunk_t chunkToBeReleased) noexcept {
      if (chunkToBeReleased->stackPos == pool.stackHead - 1) {
        _swap(chunk, *--pool.stackEmptyTail);
      }
      else {
        auto topChunk = _top_of_stack(pool);
        *chunk->stackPos   = topChunk;
        topChunk->stackPos = chunk->stackPos;
        chunk->stackPos    = chunkToBeReleased->stackPos;
        *chunk->stackPos   = chunk;
      }
    }


    inline bool         _adjust_stack_post_free(object_pool& pool, pool_chunk_t chunk) noexcept {

      // Checks if the chunk had previously been full by checking whether it is in the "full block segment" of the stack.
      if (chunk->stackPos < pool.stackFullHead) {
        _swap(chunk, *--pool.stackFullHead);
      }
      else if (_chunk_is_empty(chunk)) [[unlikely]] {

        // Iterate over the empty chunks at the top of the stack.
        // These can only accumulate in the case of dual chunks
        // where the partner chunk is still in use. Otherwise there
        // should be at most 1 empty chunk on the stack.
        const auto stackEmptyTail = pool.stackEmptyTail;
        for (auto pChunk = pool.stackHead - 1; stackEmptyTail <= pChunk; --pChunk) {
          auto otherChunk = *pChunk;
          AGT_invariant( _chunk_is_empty(otherChunk) );

          if (_chunk_is_truly_empty(otherChunk)) {
            _remove_chunk_from_stack(pool, chunk, otherChunk);
            return true;
          }
        }

        _swap(chunk, *--pool.stackEmptyTail);
      }

      return false;
    }*/

    template <typename PoolType>
    AGT_forceinline pool_chunk_t _get_alloc_chunk(PoolType& pool, agt_instance_t instance) noexcept {
      /// Always select the block on top of the stack. As all the full blocks are kept
      /// on the bottom of the stack, the only case where the top of the stack isn't a full
      /// block is when the entire stack is full, which is true if and only if
      /// \code pool->blockStackFullHead == pool->blockStackHead \endcode.
      /// In this case, a new block must be pushed to the stack, after which we are once
      /// more guaranteed that the block on top of the stack is a valid allocation block.

      /// Note that this guarantee is also true when selecting the first block past the end
      /// of the full block segment. This setup is less desirable however, as when a
      /// previously full block has a slot freed, it becomes the first block past the end
      /// of the full block segment.

      /// Selecting the block on top of the stack is also beneficial given that empty blocks
      /// are moved to the end of the stack, increasing the likelihood of block reuse.


      using traits = pool_traits<PoolType>;

      if constexpr (traits::is_owner_safe) {
        // TODO:
      }
      else {
        if (!pool.allocChunk) [[unlikely]] {
          if (_stack_is_full(pool)) [[unlikely]] {
            if constexpr (traits::is_user_safe) {
              if (!_resolve_all_deferred_ops_on_full(pool))
                _push_new_chunk_to_stack(pool, instance);
            }
            else
              _push_new_chunk_to_stack(pool, instance);
          }
          pool.allocChunk = _top_of_stack(pool);
        }

        return pool.allocChunk;
      }
    }

    template <typename PoolType>
    [[nodiscard]] AGT_forceinline object* raw_pool_alloc(PoolType& pool, agt_instance_t instance) noexcept {
      pool_chunk_t allocChunk;
      do {
        allocChunk = _get_alloc_chunk(pool, instance);
        if (auto result = _pop_slot_from_chunk<pool_traits<PoolType>::is_owner_safe>(allocChunk)) {
          auto&& [ slot, remainingCount ] = result.value();
          _adjust_stack_post_alloc(pool, allocChunk, remainingCount);
          /*if ( remainingCount == 0 ) {
            _swap(allocChunk, *pool.stackFullHead);
            ++pool.stackFullHead;
            pool.allocChunk = nullptr;
          }
          else if ( pool.stackEmptyTail <= allocChunk->stackPos ) {
            _swap(allocChunk, *pool.stackEmptyTail);
            ++pool.stackEmptyTail;
          }*/
          return &_get_object(slot);
        }
      } while(true);
    }

    template <typename PoolType>
    AGT_forceinline static void raw_release(object& obj, pool_chunk_t chunk) noexcept {

      using traits = pool_traits<PoolType>;

      auto&& slot = _get_slot(obj);

      if constexpr(traits::has_deferred_release) {
        if (static_cast<typename traits::chunk_type*>(chunk)->threadId != get_thread_id()) {
          _deferred_push_to_chunk(chunk, slot);
          return;
        }
      }

      auto newSlotsAvailable = _push_slot_to_chunk<traits::is_owner_safe>(chunk, slot);

      _adjust_stack_post_free(*static_cast<PoolType*>(chunk->ownerPool), chunk);
    }
  }


  AGT_virtual_object_type(object_pool) {
    agt_flags32_t  flags;
    agt_instance_t instance;
  };


  AGT_final_object_type(user_object_pool, extends(object_pool)) {
    impl::user_pool pool;
  };

  AGT_final_object_type(locked_object_pool, extends(object_pool)) {
    impl::locked_pool pool;
  };

  AGT_final_object_type(private_object_pool, extends(object_pool)) {
    impl::private_pool pool;
  };


  template <typename ObjectType,
            std::derived_from<object_pool> PoolType>
  [[nodiscard]] AGT_forceinline ObjectType*  pool_alloc(PoolType& pool) noexcept {
    static_assert(std::default_initializable<ObjectType>);

    object* obj = impl::raw_pool_alloc(pool.pool, pool.instance);

    if constexpr (std::is_trivially_default_constructible_v<ObjectType>) {
      obj->type = AGT_type_id_of(ObjectType);
      return std::launder(static_cast<ObjectType*>(obj));
    }
    else {
      const auto offset = obj->poolChunkOffset;
      const auto newObj = new (obj) ObjectType;
      newObj->type = AGT_type_id_of(ObjectType);
      newObj->poolChunkOffset = offset; // This is just to ensure that the compiler doesn't modify this field when the object is constructed...
      return newObj;
    }
  }

  template <typename ObjectType>
  [[nodiscard]] AGT_forceinline ObjectType*  pool_alloc(impl::ctx_pool& ctxPool, agt_instance_t instance) noexcept {
    static_assert(std::default_initializable<ObjectType>);

    object* obj = impl::raw_pool_alloc(ctxPool, instance);

    if constexpr (std::is_trivially_default_constructible_v<ObjectType>) {
      obj->type = AGT_type_id_of(ObjectType);
      return std::launder(static_cast<ObjectType*>(obj));
    }
    else {
      const auto offset = obj->poolChunkOffset;
      const auto newObj = new (obj) ObjectType;
      newObj->type = AGT_type_id_of(ObjectType);
      newObj->poolChunkOffset = offset; // This is just to ensure that the compiler doesn't modify this field when the object is constructed...
      return newObj;
    }



  }


  template <std::derived_from<object_pool> PoolType>
  AGT_forceinline static void pool_release(PoolType&, object* obj) noexcept {
    if (obj)
      impl::raw_release<std::remove_cvref_t<decltype(std::declval<PoolType&>().pool)>>(impl::_get_chunk(*obj));
  }

  AGT_forceinline static void pool_release(impl::ctx_pool&, object* obj) noexcept {
    if (obj)
      impl::raw_release<impl::ctx_pool>(*obj, impl::_get_chunk(*obj));
  }



  template <size_t Size>
  [[nodiscard]] AGT_forceinline impl::ctx_pool&    get_ctx_pool(agt_ctx_t ctx = get_ctx()) noexcept;

  [[nodiscard]] AGT_forceinline impl::ctx_pool&    get_ctx_pool_dyn(size_t size, agt_ctx_t ctx = get_ctx()) noexcept;

  [[nodiscard]] inline static private_object_pool* make_private_pool(size_t size, size_t slotsPerChunk, agt_ctx_t ctx = get_ctx()) noexcept {
    auto pool = alloc<private_object_pool>(ctx);
    pool->flags = impl::IS_PRIVATELY_OWNED_FLAG;
    pool->instance = get_instance(ctx);
    impl::_init_basic_pool(pool->pool, size, slotsPerChunk);
    return pool;
  }

  [[nodiscard]] inline static user_object_pool*    make_user_pool(size_t size, size_t slotsPerChunk, agt_ctx_t ctx = get_ctx()) noexcept {
    auto pool = alloc<user_object_pool>(ctx);
    pool->flags = impl::IS_PRIVATELY_OWNED_FLAG | impl::IS_USER_SAFE_FLAG;
    pool->instance = get_instance(ctx);
    impl::_init_basic_pool(pool->pool, size, slotsPerChunk);
    return pool;
  }

  [[nodiscard]] inline static locked_object_pool*  make_locked_pool(size_t size, size_t slotsPerChunk, agt_ctx_t ctx = get_ctx()) noexcept {
    auto pool = alloc<locked_object_pool>(ctx);
    pool->flags = impl::IS_PRIVATELY_OWNED_FLAG | impl::IS_USER_SAFE_FLAG | impl::IS_OWNER_SAFE_FLAG;
    pool->instance = get_instance(ctx);
    impl::_init_basic_pool(pool->pool, size, slotsPerChunk);
    return pool;
  }


  template <std::derived_from<object_pool> PoolType>
  AGT_forceinline agt_status_t reset_pool(PoolType& pool, size_t retainedChunkCount) noexcept {

  }

  template <std::derived_from<object_pool> PoolType>
  AGT_forceinline void         destroy_pool(PoolType* pool) noexcept {

  }
}

template <typename ObjectType>
[[nodiscard]] AGT_forceinline ObjectType* agt::alloc(agt_ctx_t ctx) noexcept {
  constexpr static size_t ObjectSize = sizeof(ObjectType);
  return pool_alloc<ObjectType>(get_ctx_pool<ObjectSize>(ctx), get_instance(ctx));
}
template <typename ObjectType>
[[nodiscard]] AGT_forceinline ObjectType* agt::dyn_alloc(size_t size, agt_ctx_t ctx) noexcept {
  return pool_alloc<ObjectType>(get_ctx_pool_dyn(size, ctx), get_instance(ctx));
}

AGT_forceinline void                      agt::release(object* obj) noexcept {

  using namespace impl;

  if (obj) {
    auto chunk = _get_chunk(*obj);

    switch (chunk->chunkType) {
      case private_pool_chunk: return impl::raw_release<private_pool>(*obj, chunk);
      case ctx_pool_chunk:     return impl::raw_release<ctx_pool>(*obj, chunk);
      case user_pool_chunk:    return impl::raw_release<user_pool>(*obj, chunk);
      case locked_pool_chunk:  return impl::raw_release<locked_pool>(*obj, chunk);
      case shared_pool_chunk: // TODO: implement
        AGT_no_default;
    }
  }
}



#endif//AGATE_OBJECT_POOL_HPP
