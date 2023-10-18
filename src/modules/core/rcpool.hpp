//
// Created by maxwe on 2022-12-20.
//

#ifndef AGATE_CORE_TRANSIENT_POOL_HPP
#define AGATE_CORE_TRANSIENT_POOL_HPP

#include "modules/core/impl/pool.hpp"
#include "object.hpp"
#include "rc.hpp"

#include <immintrin.h>

namespace agt {

  namespace impl {

    /*union slot_list {
      struct {
        agt_u16_t nextSlot;
        agt_u16_t length;
      };
      agt_u32_t bits;
    };

    union alignas(8) delayed_free_list {
      struct {
        agt_u16_t head;
        agt_u16_t next;
        agt_u16_t length;
        agt_u16_t padding;
      };
      agt_u64_t   bits = 0;
    };

    template <typename LoopFn> requires std::is_invocable_r_v<slot_list, LoopFn, slot_list>
    AGT_forceinline void slotListAtomicUpdate(slot_list& list, LoopFn&& fn) noexcept {
      slot_list old, next;

      old.bits = atomicRelaxedLoad(list.bits);

      do {
        next = fn(old);
      } while(!atomicCompareExchangeWeak(list.bits, old.bits, next.bits));
    }

    template <std::invocable<slot_list, slot_list&> LoopFn>
    AGT_forceinline void slotListAtomicUpdate(slot_list& list, slot_list init, LoopFn&& fn) noexcept {
      slot_list old, next;

      old.bits = atomicRelaxedLoad(list.bits);
      next = init;

      do {
        fn(old, next);
      } while(!atomicCompareExchangeWeak(list.bits, old.bits, next.bits));
    }

    AGT_forceinline void _init_delayed_free_list(delayed_free_list& list) noexcept {
      list.bits = 0;
      std::atomic_thread_fence(std::memory_order_release);
    }

    AGT_forceinline void _delayed_free(delayed_free_list& list, rc_object& obj) noexcept {
      delayed_free_list old, next;

      old.bits = atomicRelaxedLoad(list.bits);

      agt_u16_t thisSlot = obj.thisSlot;
      next.next = thisSlot;
      next.padding = 0;

      do {
        AGT_invariant(old.padding == 0);
        next.head = old.head == 0 ? thisSlot : old.head;
        next.length = old.length + 1;
      } while(!atomicCompareExchangeWeak(list.bits, old.bits, next.bits));
    }*/

    struct AGT_cache_aligned rcpool_chunk {
      slot_list         freeSlotList;
      agt_u32_t         flags;
      agt_u32_t         chunkWeakRefCount;
      agt::rcpool*      ownerPool;
      rcpool_chunk_t*   stackPos;
      agt_instance_t    instance;
      delayed_free_list delayedFreeList;
      size_t            chunkSize;
      uintptr_t         threadId;
    };

    /*struct st_rcpool_chunk : rcpool_chunk {

    };

    struct mt_rcpool_chunk : rcpool_chunk {};*/

    static_assert(sizeof(rcpool_chunk) == AGT_CACHE_LINE);
    static_assert(alignof(rcpool_chunk) == AGT_CACHE_LINE);

    inline constexpr size_t RCPoolBasicUnitSize = 16;
    inline constexpr size_t MaxRCPoolChunkSize  = RCPoolBasicUnitSize * (0x1 << 16); // Basic slots are indexed with a 16 bit unsigned integer, so there can be at most 2^16 basic units, so the maximum chunk size is 2^16 times the basic unit size

    AGT_forceinline rcpool_chunk_t _get_chunk(rc_object& obj) noexcept {
      return reinterpret_cast<agt::impl::rcpool_chunk_t>(reinterpret_cast<char*>(&obj) - (obj.thisSlot * RCPoolBasicUnitSize));
    }

    AGT_forceinline rc_object&     _get_slot(rcpool_chunk_t chunk, agt_u16_t slot) noexcept {
      return *reinterpret_cast<rc_object*>(reinterpret_cast<char*>(chunk) + (slot * RCPoolBasicUnitSize));
    }

    AGT_forceinline rcpool*        _get_pool(rcpool_chunk_t chunk) noexcept {
      return chunk->ownerPool;
    }
  }

  struct rcpool : object {
    agt_ctx_t             context;
    agt_u32_t             flags;
    agt_u16_t             slotsPerChunk; // totalSlotsPerChunk - slots occupied by chunk header
    agt_u16_t             totalSlotsPerChunk;
    size_t                initialChunkSize;
    size_t                lastChunkSize;
    // size_t                chunkSize;
    size_t                slotSize;
    impl::rcpool_chunk_t  allocChunk;
    impl::rcpool_chunk_t* stackBase;
    impl::rcpool_chunk_t* stackTop;
    impl::rcpool_chunk_t* stackHead;
    impl::rcpool_chunk_t* stackFullHead;
    impl::rcpool_chunk_t* stackEmptyTail;
    impl::rcpool_chunk_t  purgatoryChunk;

  };



  namespace impl {

    inline constexpr static agt_u32_t PoolIsUnsafe = 0x00010000;


    // A few helper query functions

    AGT_forceinline static bool           _chunk_is_empty(rcpool_chunk_t chunk) noexcept {
      return chunk->freeSlotList.length == chunk->totalSlotCount;
    }

    AGT_forceinline static bool           _chunk_is_full(rcpool_chunk_t chunk) noexcept {
      return chunk->freeSlotList.length == 0;
    }

    AGT_forceinline static bool           _stack_is_full(rcpool& pool) noexcept {
      return pool.stackFullHead == pool.stackHead;
    }

    AGT_forceinline static rcpool_chunk_t _top_of_stack(rcpool& pool) noexcept {
      return *(pool.stackHead - 1);
    }

    AGT_forceinline static size_t         _stack_size(rcpool& pool) noexcept {
      return pool.stackHead - pool.stackBase;
    }

    AGT_forceinline static size_t         _stack_capacity(rcpool& pool) noexcept {
      return pool.stackTop - pool.stackBase;
    }

    AGT_forceinline static rcpool_chunk_t _first_empty_chunk(rcpool& pool) noexcept {
      return *pool.stackEmptyTail;
    }

    AGT_forceinline static size_t         _usable_slot_size(rcpool& pool) noexcept {
      return pool.slotSize - RCPoolBasicUnitSize;
    }




    AGT_noinline void _resize_stack(rcpool& pool, size_t newCapacity) noexcept {
      const auto oldBase = pool.stackBase;
      auto newBase = (rcpool_chunk_t*)std::realloc(oldBase, newCapacity * sizeof(void*));

      // pool->stackTop must always be updated, but the other fields only need to be updated in the case
      // that realloc moved the location of the stack in memory.
      pool.stackTop = newBase + newCapacity;

      if (newBase != oldBase) {
        pool.stackFullHead  = newBase + (pool.stackFullHead  - oldBase);
        pool.stackEmptyTail = newBase + (pool.stackEmptyTail - oldBase);
        pool.stackHead      = newBase + (pool.stackHead      - oldBase);
        pool.stackBase      = newBase;

        // Reassign the stackPosition field for every active chunk as the stack has been moved in memory.
        for (auto pChunk = newBase; pChunk < pool.stackHead; ++pChunk)
          (*pChunk)->stackPos = pChunk;
      }
    }


    AGT_forceinline void _try_grow_stack() noexcept {

    }


    AGT_forceinline void _try_shrink_stack() noexcept {

    }


    // Allocate a new chunk and then push it to the top of the stack

    AGT_noinline void _push_new_chunk_to_stack(rcpool& pool) noexcept {

      // Determine chunk size

      size_t chunkSize;
      if (pool.lastChunkSize == MaxRCPoolChunkSize)
        chunkSize = MaxRCPoolChunkSize;
      else
        chunkSize = (pool.lastChunkSize *= 2);

      // Allocate chunk

      auto mem = ctxAllocPages(pool.context, chunkSize);
      auto chunk = new(mem) rcpool_chunk;

      // Initialize chunk slots

      const agt_u16_t initialSlot    = sizeof(rcpool_chunk) / RCPoolBasicUnitSize;
      const agt_u16_t maxSlotIndex   = chunkSize / RCPoolBasicUnitSize;
      const size_t    slotStride     = pool.slotSize;
      const agt_u16_t indexStride    = slotStride / RCPoolBasicUnitSize;

      const agt_u16_t totalSlotCount = ((maxSlotIndex - initialSlot) * RCPoolBasicUnitSize) / slotStride;

      auto slot = reinterpret_cast<char*>(chunk) + (initialSlot * RCPoolBasicUnitSize);

      for (agt_u16_t i = initialSlot; i < maxSlotIndex; slot += slotStride) {
        auto currentSlot = reinterpret_cast<rc_object*>(slot);
        currentSlot->epoch = 0;
        currentSlot->thisSlot = i;
        i += indexStride;
        currentSlot->nextFreeSlot = i;
      }

      // Initialize chunk fields

      chunk->ctx                   = pool.context;
      chunk->freeSlotList.nextSlot = initialSlot;
      chunk->freeSlotList.length   = totalSlotCount;
      chunk->totalSlotCount        = totalSlotCount;
      chunk->delayedFreeList.bits  = 0;
      chunk->chunkWeakRefCount     = 1;
      chunk->flags                 = pool.flags;
      chunk->ownerPool             = &pool;
      chunk->chunkSize             = chunkSize;
      if ((chunk->flags & PoolIsUnsafe) == 0)
        chunk->threadId            = _thread_id();


      // Reserve stack capacity

      if (pool.stackHead == pool.stackTop) [[unlikely]]
        _resize_stack(pool, (pool.stackTop - pool.stackBase) * 2);


      // Push chunk to stack

      *pool.stackHead = chunk;
      chunk->stackPos = pool.stackHead;
      ++pool.stackHead;
    }


    // Swap chunk positions on the pool's chunk stack

    AGT_forceinline static void _swap(rcpool_chunk_t chunkA, rcpool_chunk_t chunkB) noexcept {
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

    AGT_forceinline static void _swap_positions(rcpool_chunk_t* posA, rcpool_chunk_t* posB) noexcept {
      AGT_invariant( (*posA)->ownerPool == (*posB)->ownerPool );
      std::swap(*posA, *posB);
      (*posA)->stackPos = posA;
      (*posB)->stackPos = posB;
    }

    /*template <bool AtomicOp>
    AGT_forceinline static void _swap_positions(rcpool_chunk_t* posA, rcpool_chunk_t* posB) noexcept {
      AGT_assert( chunkA->ownerPool == chunkB->ownerPool );
      if (posA != posB) {
        if (posB < posA)
          std::swap(posA, posB);

        auto chunkA = *posA;
        auto chunkB = *posB;
        *posA = chunkB;
        chunkB->stackPos = posA;
        *posB = chunkA;
        chunkA->stackPos = posB;
      }
      if constexpr (AtomicOp) {

      }
      else
        _swap_positions(posA, posB);
    }*/


    template <bool AtomicOp>
    AGT_noinline    void _pop_chunk_from_stack(rcpool& pool) noexcept {
      if constexpr (AtomicOp) {
        // TODO:
      }
      else {

        auto chunk = _top_of_stack(pool);


        --pool.stackHead;

        const ptrdiff_t stackCapacity = _stack_capacity(pool);

        // Shrink the stack capacity by half if stack size is 1/4th of the current capacity.
        if (_stack_size(pool) == stackCapacity / 4) [[unlikely]]
          _resize_stack(pool, stackCapacity / 2);
      }

    }




    AGT_forceinline bool _try_resolve_delayed_frees(rcpool_chunk_t chunk) noexcept {
      delayed_free_list list;
      if ((list.bits = atomicExchange(chunk->delayedFreeList.bits, 0))) {
        // There are delayed free operations that have to be processed

        _get_slot(chunk, list.head).nextFreeSlot = chunk->freeSlotList.nextSlot;
        chunk->freeSlotList.nextSlot = list.next;
        chunk->freeSlotList.length += list.length;

        return true;
      }

      return false;
    }



    AGT_forceinline bool _try_resolve_delayed_frees_from_full_chunk(rcpool_chunk_t chunk) noexcept {
      delayed_free_list list;
      if ((list.bits = atomicExchange(chunk->delayedFreeList.bits, 0))) {
        // There are delayed free operations that have to be processed
        chunk->freeSlotList.nextSlot = list.next;
        chunk->freeSlotList.length = list.length;
        return true;
      }

      return false;
    }

    AGT_noinline    bool _resolve_all_delayed_frees(rcpool& pool) noexcept {
      bool            freed         = false;
      rcpool_chunk_t* stackBase     = pool.stackBase;
      rcpool_chunk_t* oldFullHead   = pool.stackFullHead;
      rcpool_chunk_t* stackPos      = oldFullHead - 1;
      rcpool_chunk_t* swapPos       = stackPos;

      for (; stackBase <= stackPos; --stackPos) {
        if (_try_resolve_delayed_frees_from_full_chunk(*stackPos)) {
          _swap(*stackPos, *swapPos);
          --swapPos;
          freed = true;
        }
      }

      pool.stackFullHead = swapPos + 1;

      stackPos = oldFullHead;
      swapPos  = pool.stackEmptyTail - 1;

      for (; stackPos <= swapPos; ++stackPos) {
        if (_try_resolve_delayed_frees(*stackPos)) {
          freed = true;
          if (_chunk_is_empty(*stackPos)) {
            _swap(*stackPos, *swapPos);
            --swapPos;
          }
        }
      }

      pool.stackEmptyTail = swapPos + 1;

      return freed;
    }




    template <thread_safety SafetyModel>
    AGT_forceinline rcpool_chunk_t _get_alloc_chunk(rcpool& pool) noexcept {
      if constexpr (!test(SafetyModel, thread_owner_safe)) {
        if (!pool.allocChunk) {
          if (_stack_is_full(pool)) {
            if constexpr (test(SafetyModel, thread_user_safe)) {
              if (!_resolve_all_delayed_frees(pool))
                _push_new_chunk_to_stack(pool);
            }
            else {
              _push_new_chunk_to_stack(pool);
            }
          }
          pool.allocChunk = _top_of_stack(pool);
        }
        return pool.allocChunk;
      }
      else {

      }
    }


    // bool is true if chunk was empty prior to the allocation
    template <bool AtomicOp>
    AGT_forceinline std::pair<rc_object*, agt_u16_t> _alloc_from_chunk(rcpool_chunk_t chunk) noexcept {
      if constexpr (AtomicOp) {
        agt_u16_t oldLength;
        rc_object* allocObj;
        slotListAtomicUpdate(chunk->freeSlotList,
                             [&](slot_list old) mutable {
                               oldLength = old.length;
                               allocObj = &_get_slot(chunk, old.nextSlot);
                               return slot_list{
                                   .nextSlot = allocObj->nextFreeSlot,
                                   .length   = static_cast<agt_u16_t>(old.length - 1)
                               };
                             });
        return { allocObj, oldLength };
      }
      else {
        AGT_invariant( !_chunk_is_full(chunk) );
        auto& obj = _get_slot(chunk, chunk->freeSlotList.nextSlot);
        chunk->freeSlotList.nextSlot = obj.nextFreeSlot;
        return { &obj, chunk->freeSlotList.length-- };
      }
    }



    template <thread_safety SafetyModel>
    AGT_forceinline void _adjust_stack_post_alloc(rcpool& pool, rcpool_chunk_t chunk, bool oldLength) noexcept {

    }


    template <thread_safety SafetyModel>
    AGT_forceinline void _adjust_stack_post_free(rcpool& pool, rcpool_chunk_t chunk, agt_u16_t oldAllocCount) noexcept {
      bool shrunkStack = false;

      if (oldAllocCount == 1) {
        // If the block at the top of the stack is empty AND is distinct from this block, pop the empty block from the stack
        // and destroy it, and move this block to the top of the stack.

        auto emptyChunkPos = --pool.stackEmptyTail;
        _swap(chunk, *emptyChunkPos);

        auto emptyChunk = _first_empty_chunk();
        auto topChunk = _top_of_stack(pool);

        if (chunk != topChunk) {
          if (_chunk_is_empty(topChunk)) {
            _pop_chunk_from_stack(pool);
            shrunkStack = true;
          }
          _swap(chunk, _top_of_stack(pool));
        }
      }
      else if (oldAllocCount == chunk->totalSlotCount) {


        _swap(chunk, *(pool.stackFullHead - 1));
        --pool.stackFullHead;
      }

      return shrunkStack;
    }



  }


  template <thread_safety SafetyModel>
  AGT_forceinline rc_object* impl::_alloc_from_rcpool(rcpool& pool) noexcept {
    auto allocChunk      = _get_alloc_chunk<SafetyModel>(pool);
    auto [obj, oldLength] = _alloc_from_chunk<test(SafetyModel, thread_owner_safe)>(allocChunk);
    _adjust_stack_post_alloc<SafetyModel>(pool, allocChunk, oldLength);
  }

  template <thread_safety SafetyModel>
  AGT_forceinline void       impl::_return_to_chunk(rcpool_chunk_t chunk, rc_object& obj) noexcept {

    agt_u16_t oldAllocCount;
    if constexpr (!test(SafetyModel, thread_owner_safe)) {
      if constexpr (test(SafetyModel, thread_user_safe)) {
        if (chunk->threadId != _thread_id()) {
          _delayed_free(chunk->delayedFreeList, obj);
          return;
        }
      }
      obj.nextFreeSlot = chunk->freeSlotList.nextSlot;
      chunk->freeSlotList.nextSlot = obj.thisSlot;
      oldAllocCount = chunk->freeSlotList.length++;
    }
    else {
      slotListAtomicUpdate(chunk->freeSlotList,
                           [&](slot_list old) mutable {
                             obj.nextFreeSlot = old.nextSlot;
                             oldAllocCount = old.length;
                             return slot_list {
                                 .nextSlot = obj.thisSlot,
                                 .length = ++old.length
                             };
                           });
    }


    _adjust_stack_post_free<SafetyModel>(chunk->ownerPool, chunk);
  }


  template <bool AtomicOp>
  AGT_forceinline void       impl::_retain_chunk(rcpool_chunk_t chunk, agt_u32_t count) noexcept {
    if constexpr (AtomicOp)
      atomicExchangeAdd(chunk->chunkWeakRefCount, count);
    else
      chunk->chunkWeakRefCount += count;
  }

  template <bool AtomicOp>
  AGT_forceinline void       impl::_release_chunk(rcpool_chunk_t chunk, agt_u32_t count) noexcept {
    bool shouldFreeChunk;
    if constexpr (AtomicOp) {
      shouldFreeChunk = atomicExchangeAdd(chunk->chunkWeakRefCount, -count) == count;
    }
    else {
      shouldFreeChunk = (chunk->chunkWeakRefCount -= count) == 0;
    }

    if (shouldFreeChunk)
      ctxFreePages(chunk->ctx, chunk, chunk->chunkSize);
  }

}

#endif//AGATE_CORE_TRANSIENT_POOL_HPP
