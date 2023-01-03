//
// Created by maxwe on 2022-12-05.
//

#ifndef AGATE_OBJECT_POOL_HPP
#define AGATE_OBJECT_POOL_HPP

#include "config.hpp"

#include "core/object.hpp"
#include "agate/align.hpp"
#include "core/context.hpp"
#include "core/error.hpp"


#include <bit>
#include <type_traits>
#include <concepts>
#include <array>
#include <algorithm>


#include <immintrin.h>

namespace agt {
  
  namespace impl {

    AGT_forceinline uintptr_t thread_identifier() noexcept {
      return _readgsbase_u64();
    }

    enum {
      IS_USER_SAFE_FLAG       = static_cast<agt_u32_t>(thread_user_safe),
      IS_OWNER_SAFE_FLAG      = static_cast<agt_u32_t>(thread_owner_safe),
      IS_DUAL_FLAG            = 0x4,
      IS_SMALL_FLAG           = 0x8,
      IS_BIG_FLAG             = 0x10,
      IS_PRIVATELY_OWNED_FLAG = 0x20,
      IS_SHARED_FLAG          = 0x40
    };

    AGT_BITFLAG_ENUM(pool_chunk_flags, agt_u32_t) {
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
    };

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

    
    AGT_virtual_object_type(pool_chunk_allocator) {
      agt_u32_t       flags;
      agt_u16_t       slotsPerChunk;      // totalSlotsPerChunk - slots occupied by chunk header
      agt_u16_t       totalSlotsPerChunk;
      size_t          chunkSize;
      size_t          totalSlotSize;
    };
    
    struct AGT_cache_aligned pool_chunk {
      agt_flags32_t         flags;
      agt_u16_t             nextFreeSlot;
      agt_u16_t             availableSlotCount;
      agt_u16_t             totalSlotCount;
      agt_u32_t             retainedRefCount;
      agt_u32_t             delayedFreeList;
      object_pool*          ownerPool;
      pool_chunk**          stackPos;
      pool_chunk_allocator* allocator;
      size_t                chunkSize;
      uintptr_t             threadId;
    };
    
    static_assert(sizeof(pool_chunk) == 64);

    using pool_chunk_t = pool_chunk*;

    enum object_pool_flags_t {
      AGT_OBJECT_POOL_IS_DUAL          = 0x1,
      AGT_OBJECT_POOL_DEFAULT_IS_SMALL = 0x2,
      AGT_OBJECT_POOL_DEFAULT_IS_LARGE = 0x4
    };

    enum object_chunk_allocator_flags_t {
      AGT_CHUNK_ALLOCATOR_IS_DUAL = 0x1
    };
    
    AGT_forceinline agt_ctx_t _get_local_ctx(thread_state* state) noexcept;

    AGT_final_object_type(solo_pool_chunk_allocator, extends(pool_chunk_allocator)) {
      object_pool* pool;
    };
  
    AGT_final_object_type(dual_pool_chunk_allocator, extends(pool_chunk_allocator)) {
      object_pool* smallPool;
      object_pool* largePool;
    };
    
    AGT_final_object_type(private_pool_chunk_allocator, extends(pool_chunk_allocator)) {
      agt_ctx_t context;
    };
    
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
  }
  
  AGT_object_type(object_pool) {
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
  };
  
  namespace impl {

    AGT_virtual_object_type(external_object_pool, extends(object_pool)) {
      agt_ctx_t context;
    };
    
    AGT_final_object_type(private_object_pool, extends(external_object_pool)) {
      private_pool_chunk_allocator privateChunkAllocator;
    };
  
    AGT_final_object_type(shared_object_pool, extends(external_object_pool)) {
        // TODO
    };
    
    
    
    AGT_noinline void _init_chunk_allocator(solo_pool_chunk_allocator& allocator, object_pool& pool, size_t slotsPerChunk) noexcept {
      AGT_assert( std::has_single_bit(slotsPerChunk) );
      
      const size_t slotSize = pool.slotSize;

      AGT_assert( std::has_single_bit(slotSize) );

      const size_t chunkSize = slotSize * slotsPerChunk;
      std::memset(&allocator, 0, sizeof(solo_pool_chunk_allocator));

      allocator.pool = &pool;
      allocator.chunkSize = chunkSize;
      allocator.totalSlotsPerChunk = slotsPerChunk;
      allocator.slotsPerChunk = slotsPerChunk - ((sizeof(pool_chunk) + slotSize - 1) / slotSize);
      allocator.flags = 0;
      allocator.totalSlotSize = slotSize;
    }

    AGT_noinline void _init_chunk_allocator(dual_pool_chunk_allocator& allocator, object_pool& smallPool, object_pool& largePool, size_t slotsPerChunk) noexcept {
      AGT_assert( std::has_single_bit(slotsPerChunk) );

      const size_t slotSize = smallPool.slotSize + largePool.slotSize;

      AGT_assert( std::has_single_bit(slotSize) );

      const size_t chunkSize = slotSize * slotsPerChunk;
      std::memset(&allocator, 0, sizeof(dual_pool_chunk_allocator));

      allocator.smallPool = &smallPool;
      allocator.largePool = &largePool;
      allocator.chunkSize = chunkSize;
      allocator.totalSlotsPerChunk = slotsPerChunk;
      allocator.slotsPerChunk = slotsPerChunk - (((2 * sizeof(pool_chunk)) + slotSize - 1) / slotSize);
      allocator.flags = AGT_CHUNK_ALLOCATOR_IS_DUAL;
      allocator.totalSlotSize = slotSize;
    }
    
    AGT_noinline void _init_chunk_allocator(private_pool_chunk_allocator& allocator, size_t slotsPerChunk) noexcept {
      
    }
    
    
    AGT_noinline void _init_local_pool(object_pool& pool, size_t slotSize, pool_chunk_allocator& defaultAllocator) noexcept {
      
      // const size_t chunkSize = slotSize * slotsPerChunk;
      std::memset(&pool, 0, sizeof(object_pool));
      

      const size_t initialStackSize = 2;
      auto chunkStack = (pool_chunk_t*)::malloc(initialStackSize * sizeof(void*));

      pool.chunkAllocator = &defaultAllocator;
      pool.slotSize       = slotSize;
      pool.stackBase      = chunkStack;
      pool.stackHead      = chunkStack;
      pool.stackTop       = chunkStack + initialStackSize;
      pool.stackFullHead  = chunkStack;
      pool.allocChunk     = nullptr;
    }

    AGT_noinline void _init_pool(private_object_pool& pool, size_t slotSize, size_t slotsPerChunk) noexcept {
      
      // const size_t chunkSize = slotSize * slotsPerChunk;
      // std::memset(&pool, 0, sizeof(private_object_pool));
      
      const size_t initialStackSize = 2;
      auto chunkStack = (pool_chunk_t*)::malloc(initialStackSize * sizeof(void*));

      pool.chunkAllocator = &pool.privateChunkAllocator;
      pool.slotSize       = slotSize;
      pool.stackBase      = chunkStack;
      pool.stackHead      = chunkStack;
      pool.stackTop       = chunkStack + initialStackSize;
      pool.stackFullHead  = chunkStack;
      pool.allocChunk     = nullptr;

      _init_chunk_allocator(pool.privateChunkAllocator, slotsPerChunk);
    }
    
    AGT_noinline void _init_pool(shared_object_pool& pool, size_t slotSize) noexcept {
      // TODO:
    }
    
    
    struct object_slot {
      agt_u16_t nextSlot;
      agt_u16_t slotNumber;
    };
    

    inline constexpr size_t ObjectSlotUnit = 8;


    AGT_forceinline object_slot*   _get_slot(pool_chunk_t chunk, agt_u16_t slot) noexcept {
      return reinterpret_cast<object_slot*>(reinterpret_cast<char*>(chunk) + (slot * ObjectSlotUnit));
    }

    AGT_forceinline object_slot*   _get_slot(object* obj) noexcept {
      return reinterpret_cast<object_slot*>(static_cast<void*>(obj));
    }

    AGT_forceinline pool_chunk_t   _get_chunk(object_slot* slot) noexcept {
      return reinterpret_cast<pool_chunk_t>(reinterpret_cast<char*>(slot) - (ObjectSlotUnit * slot->slotNumber));
    }

    AGT_forceinline pool_chunk_t   _get_chunk(object* obj) noexcept {
      return reinterpret_cast<pool_chunk_t>(reinterpret_cast<char*>(obj) - (ObjectSlotUnit * obj->poolChunkOffset));
    }


    inline pool_chunk_t _get_partner_chunk(pool_chunk_t chunk) noexcept {
      switch(chunk->flags & (IS_BIG_FLAG | IS_SMALL_FLAG)) {
        case 0:
          return nullptr;
        case IS_SMALL_FLAG:
          return reinterpret_cast<pool_chunk_t>(reinterpret_cast<char*>(chunk) + sizeof(pool_chunk));
        case IS_BIG_FLAG:
          return reinterpret_cast<pool_chunk_t>(reinterpret_cast<char*>(chunk) - sizeof(pool_chunk));
          AGT_no_default;
      }
    }

    inline bool         _chunk_is_empty(pool_chunk_t chunk) noexcept {
      return chunk->availableSlotCount == chunk->totalSlotCount;
    }

    inline bool         _chunk_is_truly_empty(pool_chunk_t chunk) noexcept {
      AGT_invariant(_chunk_is_empty(chunk));
      if (auto partnerChunk = _get_partner_chunk(chunk))
        return _chunk_is_empty(partnerChunk);
      return true;
    }

    inline bool         _chunk_is_in_use(pool_chunk_t chunk) noexcept {
      if (!_chunk_is_empty(chunk))
        return true;
      return !_chunk_is_truly_empty(chunk);
    }

    inline bool         _chunk_is_full(pool_chunk_t chunk) noexcept {
      return chunk->availableSlotCount == 0;
    }

    inline bool         _stack_is_full(const object_pool& pool) noexcept {
      return pool.stackFullHead == pool.stackHead;
    }

    inline void         _swap(pool_chunk_t chunkA, pool_chunk_t chunkB) noexcept {
      AGT_assert( chunkA->ownerPool == chunkB->ownerPool );
      if (chunkA != chunkB) {
        auto locA = chunkA->stackPos;
        auto locB = chunkB->stackPos;
        *locA = chunkB;
        chunkB->stackPos = locA;
        *locB = chunkA;
        chunkA->stackPos = locB;
      }
    }

    inline pool_chunk_t _top_of_stack(const object_pool& pool) noexcept {
      return *(pool.stackHead - 1);
    }


    AGT_forceinline agt_ctx_t      _get_pool_context(const object_pool& pool) noexcept {
      if (pool.type == object_type::private_object_pool || pool.type == object_type::shared_object_pool)
        return static_cast<const external_object_pool&>(pool).context;
      else
        return _get_local_ctx(get_thread_state());
    }


    AGT_forceinline void           _resize_stack(object_pool& pool, size_t newCapacity) noexcept {
      auto newBase = (pool_chunk_t*)std::realloc(pool.stackBase, newCapacity * sizeof(void*));
      auto oldBase = pool.stackBase;
      // pool->stackTop must always be updated, but the other fields only need to be updated in the case
      // that realloc moved the location of the stack in memory.
      pool.stackTop = newBase + newCapacity;

      if (newBase != oldBase) {
        pool.stackFullHead  = newBase + (pool.stackFullHead  - oldBase);
        pool.stackHead      = newBase + (pool.stackHead      - oldBase);
        pool.stackBase      = newBase;

        // Reassign the stackPosition field for every active chunk as the stack has been moved in memory.
        for (auto pChunk = newBase; pChunk < pool.stackHead; ++pChunk)
          (*pChunk)->stackPos = pChunk;
      }
    }

    /*inline void                    _push_chunk_to_stack(pool_chunk_t chunk, object_pool& pool) noexcept {
      // Grow the stack by a factor of 2 if the stack size is equal to the stack capacity

    }*/

    void _init_chunk_and_push_to_stack(pool_chunk_t          chunk,
                                       object_pool&          pool,
                                       pool_chunk_allocator* allocator,
                                       agt_u16_t             initialSlot,
                                       agt_u16_t             maxSlotIndex) noexcept {
      const size_t    slotSize         = pool.slotSize;
      const size_t    slotStride       = allocator->totalSlotSize;
      const agt_u16_t slotIndexStride  = slotStride / ObjectSlotUnit;

      auto slot = reinterpret_cast<char*>(chunk) + (initialSlot * ObjectSlotUnit);

      for (agt_u16_t i = initialSlot; i < maxSlotIndex; slot += slotStride) {
        auto currentSlot = reinterpret_cast<object_slot*>(slot);
        // currentSlot->epoch = 0;
        currentSlot->slotNumber = i;
        i += slotIndexStride;
        currentSlot->nextSlot   = i;
      }

      chunk->nextFreeSlot          = initialSlot;
      chunk->availableSlotCount    = allocator->slotsPerChunk;
      chunk->totalSlotCount        = allocator->slotsPerChunk;
      chunk->delayedFreeList       = 0;
      chunk->retainedRefCount      = 0;
      chunk->allocator             = allocator;
      chunk->ownerPool             = &pool;
      chunk->chunkSize             = allocator->chunkSize;
      // chunk->slotSize              = slotSize;
      chunk->threadId              = thread_identifier();


      // Push to stack :)
      if (pool.stackHead == pool.stackTop) [[unlikely]]
        _resize_stack(pool, (pool.stackTop - pool.stackBase) * 2);

      *pool.stackHead = chunk;
      chunk->stackPos = pool.stackHead;
      ++pool.stackHead;
    }


    void           _push_new_chunk_to_stack(object_pool& pool, agt_ctx_t context) noexcept {
      const auto allocator = pool.chunkAllocator;
      auto mem = ctxAllocPages(context, allocator->chunkSize);

      if (allocator->flags & IS_DUAL_FLAG) {
        auto alloc = static_cast<dual_pool_chunk_allocator*>(allocator);
        auto smallChunk = new(mem) pool_chunk;
        auto bigChunk   = new(static_cast<char*>(mem) + sizeof(pool_chunk)) pool_chunk;
        smallChunk->flags = IS_DUAL_FLAG | IS_SMALL_FLAG;
        bigChunk->flags   = IS_DUAL_FLAG | IS_BIG_FLAG;
        const agt_u16_t bigInitialSlot    = sizeof(pool_chunk) / ObjectSlotUnit;
        const agt_u16_t smallInitialSlot  = bigInitialSlot * 2;
        const agt_u16_t smallMaxSlotIndex = allocator->chunkSize / ObjectSlotUnit;
        const agt_u16_t bigMaxSlotIndex   = smallMaxSlotIndex - bigInitialSlot;
        // TODO: Refactor the following so that only a single iteration is done rather than two
        //       separate ones. This should improve cache performance by a decent bit.
        _init_chunk_and_push_to_stack(smallChunk, *alloc->smallPool, allocator, smallInitialSlot, smallMaxSlotIndex);
        _init_chunk_and_push_to_stack(bigChunk,   *alloc->largePool, allocator, bigInitialSlot,   bigMaxSlotIndex);
      }
      else {
        auto alloc = static_cast<solo_pool_chunk_allocator*>(allocator);
        auto chunk = new(mem) pool_chunk;
        chunk->flags = 0;
        const agt_u16_t initialSlot  = sizeof(pool_chunk) / ObjectSlotUnit;
        const agt_u16_t maxSlotIndex = allocator->chunkSize / ObjectSlotUnit;
        _init_chunk_and_push_to_stack(chunk, *alloc->pool, allocator, initialSlot, maxSlotIndex);
      }

      ++pool.stackEmptyTail; // This is only ever called immediately preceding an allocation call; simply incrementing the empty chunk cursor now saves the overhead of a couple checks we know deterministically will be true.

      // Note that in the case of a dual chunk allocator, nothing needs to be done to the stackEmptyTail after pushing to stack, despite not being guaranteed that the state of the partner pool is also full
      // If there already was an empty chunk on the partner pool's stack, the new chunk will be placed after it, and thus after stackEmptyTail.
      // If there aren't any empty chunks on the partner pool's stack, then before the new chunk is pushed, stackEmptyTail will have been equal to stackHead. During the push operation, stackHead is incremented, but stackEmptyTail is untouched, thus leaving stackEmptyTail pointing at the newly pushed chunk.
      // Thus, the invariants hold :)
    }

    /*void           _release_chunk(agt_ctx_t context, pool_chunk_t chunk) noexcept {
      ctxFreePages(context, chunk, chunk->chunkSize);
    }*/

    AGT_noinline inline void _pop_stack(object_pool& pool) noexcept {
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

      ctxFreePages(_get_pool_context(pool), chunkToBeReleased, chunkToBeReleased->chunkSize);
      // _release_chunk(context, chunkToBeReleased);

      _pop_stack(pool);
    }

    AGT_forceinline pool_chunk_t _get_alloc_chunk(object_pool& pool, agt_ctx_t context) noexcept {

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

      if (!pool.allocChunk) [[unlikely]] {
        if (_stack_is_full(pool)) [[unlikely]]
          _push_new_chunk_to_stack(pool, context);
        pool.allocChunk = _top_of_stack(pool);
      }
      return pool.allocChunk;
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
    }


    bool _trim_chunk(object_pool& pool, pool_chunk_t chunk) noexcept {
      union {
        struct {
          agt_u16_t head;
          agt_u16_t length;
        };
        agt_u32_t value = 0;
      };

      value = atomicExchange(chunk->delayedFreeList, 0);

      if (value != 0) {

        object_slot* tailSlot = _get_slot(chunk, head);

        if (length > 1) {
          agt_u16_t index = tailSlot->nextSlot;
          agt_u32_t slotsRemaining = length - 1;

          do {
            tailSlot = _get_slot(chunk, index);
            index = tailSlot->nextSlot;
            --slotsRemaining;
          } while (slotsRemaining);
        }


        tailSlot->nextSlot = chunk->nextFreeSlot;
        chunk->nextFreeSlot = head;
        chunk->availableSlotCount += length;

        return _adjust_stack_post_free(pool, chunk);
      }

      return false;
    }


    void  _append_to_delayed_list(pool_chunk_t chunk, object_slot* obj) noexcept {
      union {
        struct {
          agt_u16_t head;
          agt_u16_t length;
        };
        agt_u32_t value = 0;
      } old, self;

      old.value = atomicRelaxedLoad(chunk->delayedFreeList);
      self.head = obj->slotNumber;

      do {
        obj->nextSlot = old.head;
        self.length   = old.length + 1;
      } while(!atomicCompareExchangeWeak(chunk->delayedFreeList, old.value, self.value));
    }
    
    
    template <thread_safety ThreadSafety>
    [[nodiscard]] AGT_forceinline object* _raw_pool_alloc(object_pool& pool, agt_ctx_t context) noexcept {

      AGT_invariant( context != nullptr );

      auto allocChunk = _get_alloc_chunk(pool, context);

      auto slotIndex = allocChunk->nextFreeSlot;
      auto slot    = _get_slot(allocChunk, slotIndex);

      AGT_invariant( slotIndex == slot->slotNumber );

      allocChunk->nextFreeSlot = slot->nextSlot;
      agt_u16_t nextAvailableSlotCount = --allocChunk->availableSlotCount;

      if (nextAvailableSlotCount == 0) {
        _swap(allocChunk, *pool.stackFullHead);
        ++pool.stackFullHead;
        pool.allocChunk = nullptr;
      }
      else if ( pool.stackEmptyTail <= allocChunk->stackPos ) {
        _swap(allocChunk, *pool.stackEmptyTail);
        ++pool.stackEmptyTail;
      }

      return reinterpret_cast<object*>(static_cast<void*>(slot));
    }
  }

  
  
  template <typename ObjectType,
            thread_safety SafetyModel,
            bool IsDefaultPool = false>
  [[nodiscard]] AGT_forceinline ObjectType*  pool_alloc(object_pool& pool, agt_ctx_t context = nullptr) noexcept {
    if constexpr (IsDefaultPool) {
      AGT_invariant( context != nullptr );
    }
    else {
      if (!context) {
        if (pool.type == object_type::private_object_pool || pool.type == object_type::shared_object_pool) [[likely]]
          context = static_cast<impl::external_object_pool*>(&pool)->context;
        else
          context = impl::_get_local_ctx(get_thread_state());
      }
    }
    object* obj = impl::_raw_pool_alloc<SafetyModel>(pool, context);
    obj->type = AGT_type_id_of(ObjectType);
    return static_cast<ObjectType*>(obj);
  }
  
  template <size_t Size>
  [[nodiscard]] AGT_forceinline object_pool& get_local_pool(thread_state* state = get_thread_state()) noexcept;

  [[nodiscard]] AGT_forceinline object_pool& get_local_pool_dyn(size_t size, thread_state* state = get_thread_state()) noexcept;
  
  
  template <thread_safety SafetyModel>
  [[nodiscard]] AGT_forceinline object_pool* make_private_pool(size_t size, size_t slotsPerChunk, thread_state* state = get_thread_state()) noexcept {
    auto pool = alloc<impl::private_object_pool, SafetyModel>(state);
    impl::_init_pool(*pool, size, slotsPerChunk);
    return pool;
  }

  template <thread_safety SafetyModel>
  AGT_forceinline void reset_pool(object_pool& pool, bool retainChunks) noexcept;

  template <thread_safety SafetyModel>
  AGT_forceinline void destroy_pool(object_pool& pool) noexcept;
}

template <typename ObjectType, agt::thread_safety SafetyModel>
[[nodiscard]] AGT_forceinline ObjectType* agt::alloc(thread_state* state) noexcept {
  constexpr static size_t ObjectSize = sizeof(ObjectType);
  return pool_alloc<ObjectType, SafetyModel, true>(get_local_pool<ObjectSize>(state), impl::_get_local_ctx(state));
}
template <typename ObjectType, agt::thread_safety SafetyModel>
[[nodiscard]] AGT_forceinline ObjectType* agt::dyn_alloc(size_t size, thread_state* state) noexcept {
  return pool_alloc<ObjectType, SafetyModel, true>(get_local_pool_dyn(size, state), impl::_get_local_ctx(state));
}

template <agt::thread_safety SafetyModel>
AGT_forceinline void                      agt::release(object* obj) noexcept {

  using namespace impl;

  auto chunk = _get_chunk(obj);

  if constexpr (test(SafetyModel, thread_owner_safe)) {
    // TODO:
  }
  else {


    auto pool  = chunk->ownerPool;

    auto slot = reinterpret_cast<object_slot*>(static_cast<void*>(obj));


    if constexpr(test(SafetyModel, thread_user_safe)) {
      if (chunk->threadId != thread_identifier()) {
        _append_to_delayed_list(chunk, slot);
        return;
      }
    }

    slot->nextSlot = chunk->nextFreeSlot;
    chunk->nextFreeSlot = slot->slotNumber;
    ++chunk->availableSlotCount;

    _adjust_stack_post_free(*pool, chunk);
  }
}



#endif//AGATE_OBJECT_POOL_HPP
