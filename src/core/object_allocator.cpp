//
// Created by maxwe on 2022-12-05.
//

#include "agate/atomic.hpp"
#include "core/object.hpp"
#include "object_pool.hpp"
#include "thread_state.hpp"

#include <cstring>

#include <immintrin.h>

enum {
  AGT_CHUNK_IS_DUAL  = 0x1,
  AGT_CHUNK_IS_SMALL = 0x2,
  AGT_CHUNK_IS_BIG   = 0x4,
};

struct AGT_cache_aligned agt::alloc_impl::object_pool_chunk {
  agt_u32_t               flags;
  agt_u16_t               nextFreeSlot;
  agt_u16_t               availableSlotCount;
  agt_u16_t               totalSlotCount;
  agt_u32_t               retainedRefCount;
  agt_u32_t               delayedFreeList;
  object_pool*            ownerPool;
  object_chunk_t*         stackPos;
  object_chunk_allocator* allocator;
  size_t                  chunkSize;
  uintptr_t               threadId;
};

static_assert(sizeof(agt::alloc_impl::object_pool_chunk) == 64);

namespace {

  AGT_forceinline uintptr_t thread_identifier() noexcept {
    return _readgsbase_u64();
  }

  using namespace agt;
  using namespace agt::alloc_impl;

  struct object_slot {
    agt_u16_t nextSlot;
    agt_u16_t slotNumber;
  };


  inline constexpr size_t ObjectSlotUnit = 8;


  inline object_slot*   _get_slot(object_chunk_t chunk, agt_u16_t slot) noexcept {
    return reinterpret_cast<object_slot*>(reinterpret_cast<char*>(chunk) + (slot * ObjectSlotUnit));
  }

  inline object_slot*   _get_slot(object* obj) noexcept {
    return reinterpret_cast<object_slot*>(static_cast<void*>(obj));
  }

  inline object_chunk_t _get_chunk(object_slot* slot) noexcept {
    return reinterpret_cast<object_chunk_t>(reinterpret_cast<char*>(slot) - (ObjectSlotUnit * slot->slotNumber));
  }

  inline object_chunk_t _get_chunk(object* obj) noexcept {
    return reinterpret_cast<object_chunk_t>(reinterpret_cast<char*>(obj) - (ObjectSlotUnit * obj->poolChunkOffset));
  }

  inline void           _resize_stack(object_pool* pool, size_t newCapacity) noexcept {
    auto newBase = (object_chunk_t*)std::realloc(pool->stackBase, newCapacity * sizeof(void*));

    // pool->stackTop must always be updated, but the other fields only need to be updated in the case
    // that realloc moved the location of the stack in memory.
    pool->stackTop = newBase + newCapacity;

    if (newBase != pool->stackBase) {
      pool->stackFullHead  = newBase + (pool->stackFullHead  - pool->stackBase);
      pool->stackHead      = newBase + (pool->stackHead      - pool->stackBase);
      pool->stackBase      = newBase;

      // Reassign the stackPosition field for every active chunk as the stack has been moved in memory.
      for (auto pChunk = newBase; pChunk < pool->stackHead; ++pChunk)
        (*pChunk)->stackPos = pChunk;
    }
  }

  inline void           _push_chunk_to_stack(object_chunk_t chunk, object_pool* pool) noexcept {
    // Grow the stack by a factor of 2 if the stack size is equal to the stack capacity
    if (pool->stackHead == pool->stackTop) [[unlikely]]
      _resize_stack(pool, (pool->stackTop - pool->stackBase) * 2);

    *pool->stackHead = chunk;
    chunk->stackPos = pool->stackHead;
    ++pool->stackHead;
  }

  void _init_chunk_and_push_to_stack(object_chunk_t          chunk,
                                     object_pool*            pool,
                                     object_chunk_allocator* allocator,
                                     agt_u16_t               initialSlot,
                                     agt_u16_t               maxSlotIndex) noexcept {
    const size_t    slotSize         = pool->slotSize;
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
    chunk->ownerPool             = pool;
    chunk->chunkSize             = allocator->chunkSize;
    // chunk->slotSize              = slotSize;
    chunk->threadId              = thread_identifier();

    _push_chunk_to_stack(chunk, pool);
  }


  void _push_new_chunk_to_stack(object_chunk_allocator* allocator) noexcept {
    auto mem = ctxAllocPages(allocator->context, allocator->chunkSize);

    if (allocator->flags & AGT_CHUNK_ALLOCATOR_IS_DUAL) {
      auto alloc = static_cast<dual_object_chunk_allocator*>(allocator);
      auto smallChunk = new(mem) object_pool_chunk;
      auto bigChunk   = new(static_cast<char*>(mem) + sizeof(object_pool_chunk)) object_pool_chunk;
      smallChunk->flags = AGT_CHUNK_IS_DUAL | AGT_CHUNK_IS_SMALL;
      bigChunk->flags   = AGT_CHUNK_IS_DUAL | AGT_CHUNK_IS_BIG;
      const agt_u16_t bigInitialSlot    = sizeof(object_pool_chunk) / ObjectSlotUnit;
      const agt_u16_t smallInitialSlot  = bigInitialSlot * 2;
      const agt_u16_t smallMaxSlotIndex = allocator->chunkSize / ObjectSlotUnit;
      const agt_u16_t bigMaxSlotIndex   = smallMaxSlotIndex - bigInitialSlot;
      // TODO: Refactor the following so that only a single iteration is done rather than two
      //       separate ones. This should improve cache performance by a decent bit.
      _init_chunk_and_push_to_stack(smallChunk, alloc->smallPool, allocator, smallInitialSlot, smallMaxSlotIndex);
      _init_chunk_and_push_to_stack(bigChunk,   alloc->largePool, allocator, bigInitialSlot,   bigMaxSlotIndex);
    }
    else {
      auto alloc = static_cast<solo_object_chunk_allocator*>(allocator);
      auto chunk = new(mem) object_pool_chunk;
      chunk->flags = 0;
      const agt_u16_t initialSlot  = sizeof(object_pool_chunk) / ObjectSlotUnit;
      const agt_u16_t maxSlotIndex = allocator->chunkSize / ObjectSlotUnit;
      _init_chunk_and_push_to_stack(chunk, alloc->pool, allocator, initialSlot, maxSlotIndex);
    }
  }

  void           _release_chunk(object_chunk_t chunk) noexcept {
    if (atomicDecrement(chunk->retainedRefCount) == 0) {
      ctxFreePages(get_ctx(), chunk, chunk->chunkSize);
    }
  }

  inline bool    _chunk_is_empty(object_chunk_t chunk) noexcept {
    return chunk->availableSlotCount == chunk->totalSlotCount;
  }

  inline bool    _chunk_is_full(object_chunk_t chunk) noexcept {
    return chunk->availableSlotCount == 0;
  }

  inline bool    _stack_is_full(object_pool* pool) noexcept {
    return pool->stackFullHead == pool->stackHead;
  }

  inline void    _swap(object_chunk_t chunkA, object_chunk_t chunkB) noexcept {
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

  inline object_chunk_t _top_of_stack(object_pool* pool) noexcept {
    return *(pool->stackHead - 1);
  }

  inline void _pop_chunk_from_stack(object_pool* pool) noexcept {

    _release_chunk(_top_of_stack(pool));
    --pool->stackHead;

    const ptrdiff_t stackCapacity = pool->stackTop - pool->stackBase;

    // Shrink the stack capacity by half if stack size is 1/4th of the current capacity.
    if (pool->stackHead - pool->stackBase == stackCapacity / 4) [[unlikely]]
      _resize_stack(pool, stackCapacity / 2);
  }

  inline void _find_alloc_chunk(object_pool* pool) noexcept {

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

    if (_stack_is_full(pool))
      _push_new_chunk_to_stack(pool->chunkAllocator);
    pool->allocChunk = _top_of_stack(pool);
  }

  inline bool _adjust_stack_post_free(object_pool* pool, object_chunk_t chunk) noexcept {

    bool shrunkStack = false;

    // Checks if the chunk had previously been full by checking whether it is in the "full block segment" of the stack.
    if (chunk->stackPos < pool->stackFullHead) {
      _swap(chunk, *(pool->stackFullHead - 1));
      --pool->stackFullHead;
    }
    else if (_chunk_is_empty(chunk)) [[unlikely]] {
      // TODO: Fix this logic, which assumes that a pool being done with a chunk means it's out of use lol
      // If the block at the top of the stack is empty AND is distinct from this block, pop the empty block from the stack
      // and destroy it, and move this block to the top of the stack.
      auto topChunk = _top_of_stack(pool);
      if (chunk != topChunk) {
        if (_chunk_is_empty(topChunk)) {
          _pop_chunk_from_stack(pool);
          shrunkStack = true;
        }
        _swap(chunk, _top_of_stack(pool));
      }

      // No matter what, block is now the only empty block in the pool, and is on top of the stack.
    }

    return shrunkStack;
  }


  bool _trim_chunk(object_pool* pool, object_chunk_t chunk) noexcept {
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



  void  _append_to_delayed_list(object_pool* pool, object_chunk_t chunk, object_slot* obj) noexcept {
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



  void   _init_pool(object_pool& pool, size_t slotSize, object_chunk_allocator& defaultAllocator) noexcept {

    // const size_t chunkSize = slotSize * slotsPerChunk;
    std::memset(&pool, 0, sizeof(object_pool));


    const size_t initialStackSize = 2;
    auto chunkStack = (object_chunk_t*)::malloc(initialStackSize * sizeof(void*));

    pool.chunkAllocator = &defaultAllocator;
    pool.slotSize       = slotSize;
    pool.stackBase      = chunkStack;
    pool.stackHead      = chunkStack;
    pool.stackTop       = chunkStack + initialStackSize;
    pool.stackFullHead  = chunkStack;
    pool.allocChunk     = nullptr;
  }

  void   _init_chunk_allocator(agt_ctx_t ctx, solo_object_chunk_allocator& allocator, object_pool& pool, size_t slotsPerChunk) noexcept {
    AGT_assert( std::has_single_bit(slotsPerChunk) );

    const size_t slotSize = pool.slotSize;

    AGT_assert( std::has_single_bit(slotSize) );

    const size_t chunkSize = slotSize * slotsPerChunk;
    std::memset(&allocator, 0, sizeof(solo_object_chunk_allocator));

    allocator.pool = &pool;
    allocator.chunkSize = chunkSize;
    allocator.totalSlotsPerChunk = slotsPerChunk;
    allocator.slotsPerChunk = slotsPerChunk - ((sizeof(object_pool_chunk) + slotSize - 1) / slotSize);
    allocator.context = ctx;
    allocator.flags = 0;
    allocator.totalSlotSize = slotSize;
  }

  void   _init_chunk_allocator(agt_ctx_t ctx, dual_object_chunk_allocator& allocator, object_pool& smallPool, object_pool& largePool, size_t slotsPerChunk) noexcept {
    AGT_assert( std::has_single_bit(slotsPerChunk) );

    const size_t slotSize = smallPool.slotSize + largePool.slotSize;

    AGT_assert( std::has_single_bit(slotSize) );

    const size_t chunkSize = slotSize * slotsPerChunk;
    std::memset(&allocator, 0, sizeof(solo_object_chunk_allocator));

    allocator.smallPool = &smallPool;
    allocator.largePool = &largePool;
    allocator.chunkSize = chunkSize;
    allocator.totalSlotsPerChunk = slotsPerChunk;
    allocator.slotsPerChunk = slotsPerChunk - (((2 * sizeof(object_pool_chunk)) + slotSize - 1) / slotSize);
    allocator.context = ctx;
    allocator.flags = AGT_CHUNK_ALLOCATOR_IS_DUAL;
    allocator.totalSlotSize = slotSize;
  }


  void    _destroy_pool(object_pool* pool) noexcept {

    union {
      struct {
        agt_u16_t head;
        agt_u16_t length;
      };
      agt_u32_t value = 0;
    };

    for (auto chunk = pool->stackBase; chunk != pool->stackHead; ++chunk) {
      auto c = *chunk;
      value = atomicExchange(c->delayedFreeList, 0);
      c->availableSlotCount += length;
      atomicStore(c->ownerPool, nullptr);
      // std::atomic_thread_fence(std::memory_order_release);
      _release_chunk(c);
    }
    ::free(pool->stackBase);
    pool->stackBase = nullptr;
    pool->stackHead = nullptr;
    pool->stackFullHead = nullptr;
    pool->stackTop = nullptr;
    pool->allocChunk = nullptr;
    // std::memset(pool, 0, sizeof(object_pool));
  }

  void    _trim_pool(object_pool* pool) noexcept {

    constexpr static size_t InlineBufferSize = 64;

    object_chunk_t  buffer[InlineBufferSize];
    size_t          count;
    object_chunk_t* array;

    // All this extra stuff is so that the stack is copied over to a temporary buffer, given that the
    // trimming process may modify the stack order. The trimming process may also pop elements from the
    // stack, in which case count is reduced such that we do not attempt to trim already freed chunks.

    count = pool->stackHead - pool->stackBase;

    // Use inline buffer if count is small enough (note this should cover the vast majority of cases)
    if (count <= InlineBufferSize) [[likely]]
      array = buffer;
    else
      array = new object_chunk_t[count];

    std::memcpy(array, pool->stackBase, count * sizeof(void*));


    // Try to trim every allocated chunk
    for (size_t i = 0; i < count; ++i) {
      if (_trim_chunk(pool, array[i]))
        --count;
    }


    // Release array if it was dynamically allocated.
    if (array != buffer)
      delete[] array;
  }
}













object* agt::alloc_impl::alloc_obj(object_pool* pool) noexcept {
  if (!pool->allocChunk) [[unlikely]]
    _find_alloc_chunk(pool);

  auto allocChunk = pool->allocChunk;

  // bool wasEmpty = _chunk_is_empty(allocChunk);

  auto slotIndex = allocChunk->nextFreeSlot;
  auto slot = reinterpret_cast<object_slot*>(reinterpret_cast<char*>(allocChunk) + (slotIndex * pool->slotSize));

  AGT_assert( slotIndex == slot->slotNumber );

  allocChunk->nextFreeSlot = slot->nextSlot;
  --allocChunk->availableSlotCount;

  // auto prevAvailSlotCount = allocChunk->availableSlotCount--;

  /*if (prevAvailSlotCount == allocChunk->totalSlotCount) {

    _swap(allocChunk, *pool->stackEmptyTail);
    ++pool->stackEmptyTail;
  }
  else */

  if (_chunk_is_full(pool->allocChunk)) {
    _swap(pool->allocChunk, *pool->stackFullHead);
    ++pool->stackFullHead;
    pool->allocChunk = nullptr;
  }

  return reinterpret_cast<object*>(static_cast<void*>(slot));
}


void    agt::free_obj(object* obj) noexcept {
  auto chunk = _get_chunk(obj);
  auto pool  = chunk->ownerPool;

  auto slot = reinterpret_cast<object_slot*>(static_cast<void*>(obj));

  // atomicIncrement(slot->epoch);

  if (chunk->threadId == thread_identifier()) {
    slot->nextSlot = chunk->nextFreeSlot;
    chunk->nextFreeSlot = slot->slotNumber;
    ++chunk->availableSlotCount;
    _adjust_stack_post_free(pool, chunk);
  }
  else {
    _append_to_delayed_list(pool, chunk, slot);
  }
}

namespace {



}


void         agt::init_object_allocator(agt_ctx_t ctx, object_allocator *allocator) noexcept {
  initializer helper{ctx, allocator};

  for (size_t i = 0; i < ObjectPoolCount; ++i)
    helper.init_pool(allocator, i);
  for (size_t i = 0; i < SoloChunkAllocatorCount; ++i)
    helper.init_solo_chunk_allocator(allocator, i);
  for (size_t i = 0; i < DualChunkAllocatorCount; ++i)
    helper.init_dual_chunk_allocator(allocator, i);
}

agt::object* agt::dyn_alloc(object_allocator* allocator, size_t size) noexcept {
  using namespace alloc_impl;

  static_assert(std::has_single_bit(MinAllocAlignment));

  if (size > MaxAllocSize) [[unlikely]] {
    alloc_impl::reportBadSize(allocator, size);
    return nullptr;
  }

  const size_t alignedSize = align_to<MinAllocAlignment>(size);
  const size_t poolIndex   = PoolIndexLookupTable[ alignedSize / MinAllocAlignment ];

  return alloc_impl::alloc_obj(&allocator->objectPools[poolIndex]);
}


void         agt::destroy_object_allocator(object_allocator* allocator) noexcept {

}

void         agt::trim_object_allocator(object_allocator* allocator, int poolIndex) noexcept {
  if (poolIndex == -1) {
    for () {

    }
  }
  else {

  }
}
