//
// Created by maxwe on 2022-12-05.
//

#include "object_pool.hpp"
#include "core/objects.hpp"
#include "support/atomic.hpp"
#include "thread_state.hpp"

#include <cstring>


enum {
  AGT_CHUNK_IS_DUAL  = 0x1,
  AGT_CHUNK_IS_SMALL = 0x2,
  AGT_CHUNK_IS_BIG   = 0x4,
};

struct AGT_cache_aligned agt::object_pool_chunk {
  agt_u32_t       flags;
  agt_u16_t       nextFreeSlot;
  agt_u16_t       availableSlotCount;
  agt_u32_t       retainedRefCount;
  agt_u32_t       delayedFreeList;
  void*           ownerPool;
  object_chunk_t* stackPos;
  size_t          chunkSize;
  thread_state*   threadCtx;
};

static_assert(sizeof(agt::object_pool_chunk) == 64);

namespace {

  using namespace agt;

  struct object_slot {
    agt_u16_t nextSlot;
    agt_u16_t slotNumber;
    agt_u32_t flags;
    agt_u32_t refCount;
    agt_u32_t epoch;
  };


  inline constexpr size_t BasicSlotSize = 8;


  inline object_slot*   _get_slot(object_chunk_t chunk, agt_u16_t slot) noexcept {
    return reinterpret_cast<object_slot*>(reinterpret_cast<char*>(chunk) + (slot * BasicSlotSize));
  }

  inline object_slot*   _get_slot(object* obj) noexcept {
    return reinterpret_cast<object_slot*>(static_cast<void*>(obj));
  }

  inline object_chunk_t _get_chunk(object_slot* slot) noexcept {
    return reinterpret_cast<object_chunk_t>(reinterpret_cast<char*>(slot) - (BasicSlotSize * slot->slotNumber));
  }

  inline object_chunk_t _get_chunk(object* obj) noexcept {
    return reinterpret_cast<object_chunk_t>(reinterpret_cast<char*>(obj) - (BasicSlotSize * obj->poolChunkOffset));
  }

  void _init_chunk(object_chunk_t chunk,
                   void* pool,
                   size_t slotSize,
                   agt_u16_t initialSlot,
                   agt_u16_t maxSlotIndex,
                   size_t slotStride) noexcept {
    const agt_u16_t slotIndexStride  = slotStride / BasicSlotSize;

    auto slot = reinterpret_cast<char*>(chunk) + (initialSlot * BasicSlotSize);

    for (agt_u16_t i = initialSlot; i < maxSlotIndex; slot += slotStride) {
      auto currentSlot = reinterpret_cast<object_slot*>(slot);
      // currentSlot->epoch = 0;
      currentSlot->slotNumber = i;
      i += slotIndexStride;
      currentSlot->nextSlot   = i;
    }

    chunk->nextFreeSlot          = initialSlot;
    chunk->availableSlotCount    = pool->slotsPerChunk;
    // chunk->ownerThreadId         = pool->ownerThreadId;
    chunk->delayedFreeList       = 0;
    chunk->retainedRefCount      = 0;
    chunk->ownerPool             = pool;
    chunk->stackPos              = nullptr;
    chunk->chunkSize             = pool->chunkSize;
    chunk->threadCtx             = get_thread_state();
  }

  object_chunk_t _new_chunk(agt_ctx_t ctx, object_pool* pool) noexcept {
    auto mem = ctxAllocPages(ctx, pool->chunkSize);
    auto chunk = new(mem) object_pool_chunk;

    const size_t    slotSize    = pool->slotSize;
    const agt_u16_t initialSlot = ((sizeof(object_pool_chunk) + BasicSlotSize - 1) / BasicSlotSize);
    const agt_u16_t slotOffset  = slotSize / BasicSlotSize;
    const agt_u16_t maxSlotIndex = pool->totalSlotsPerChunk * slotOffset;

    auto slot = static_cast<char*>(mem) + (initialSlot * BasicSlotSize);

    for (agt_u16_t i = initialSlot; i < maxSlotIndex; slot += BasicSlotSize) {
      auto currentSlot = reinterpret_cast<object_slot*>(slot);
      // currentSlot->epoch = 0;
      currentSlot->slotNumber = i;
      i += slotOffset;
      currentSlot->nextSlot   = i;
    }

    chunk->nextFreeSlot          = initialSlot;
    chunk->availableSlotCount    = pool->slotsPerChunk;
    // chunk->ownerThreadId         = pool->ownerThreadId;
    chunk->delayedFreeList       = 0;
    chunk->retainedRefCount      = 0;
    chunk->ownerPool             = pool;
    chunk->stackPos              = nullptr;
    chunk->chunkSize             = pool->chunkSize;
    chunk->threadCtx             = get_thread_state();

    return chunk;
  }

  object_chunk_t _new_chunk(agt_ctx_t ctx, object_dual_pool* pool) noexcept {
    auto mem = ctxAllocPages(ctx, pool->chunkSize);
    auto chunk = new(mem) object_pool_chunk;


    const size_t    slotSize    = pool->slotSize;
    const agt_u16_t initialSlot = ((sizeof(object_pool_chunk) + BasicSlotSize - 1) / BasicSlotSize);
    const agt_u16_t slotOffset  = slotSize / BasicSlotSize;
    const agt_u16_t maxSlotIndex = pool->totalSlotsPerChunk * slotOffset;

    auto slot = static_cast<char*>(mem) + (initialSlot * BasicSlotSize);

    for (agt_u16_t i = initialSlot; i < maxSlotIndex; slot += BasicSlotSize) {
      auto currentSlot = reinterpret_cast<object_slot*>(slot);
      // currentSlot->epoch = 0;
      currentSlot->slotNumber = i;
      i += slotOffset;
      currentSlot->nextSlot   = i;
    }

    chunk->nextFreeSlot          = initialSlot;
    chunk->availableSlotCount    = pool->slotsPerChunk;
    // chunk->ownerThreadId         = pool->ownerThreadId;
    chunk->delayedFreeList       = 0;
    chunk->retainedRefCount      = 0;
    chunk->ownerPool             = pool;
    chunk->stackPos              = nullptr;
    chunk->chunkSize             = pool->chunkSize;
    chunk->threadCtx             = get_thread_state();

    return chunk;
  }

  void           _release_chunk(object_chunk_t chunk) noexcept {
    if (atomicDecrement(chunk->retainedRefCount) == 0) {
      ctxFreePages(get_ctx(), chunk, chunk->chunkSize);
    }
  }

  inline bool    _chunk_is_empty(object_pool* pool, object_chunk_t chunk) noexcept {
    return chunk->availableSlotCount == pool->slotsPerChunk;
  }

  inline bool    _chunk_is_full(object_chunk_t chunk) noexcept {
    return chunk->availableSlotCount == 0;
  }

  inline void    _swap(object_chunk_t chunkA, object_chunk_t chunkB) noexcept {
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
    return *(pool->chunkStackHead - 1);
  }

  inline void    _resize_stack(object_pool* pool, size_t newCapacity) noexcept {
    auto newBase = (object_chunk_t*)std::realloc(pool->chunkStackBase, newCapacity * sizeof(void*));

    // pool->chunkStackTop must always be updated, but the other fields only need to be updated in the case
    // that realloc moved the location of the stack in memory.
    pool->chunkStackTop = newBase + newCapacity;
    if (newBase != pool->chunkStackBase) {
      pool->chunkStackFullHead = newBase + (pool->chunkStackFullHead - pool->chunkStackBase);
      pool->chunkStackHead     = newBase + (pool->chunkStackHead - pool->chunkStackBase);
      pool->chunkStackBase     = newBase;

      // Reassign the stackPosition field for every active block as the stack has been moved in memory.
      for (; newBase < pool->chunkStackHead; ++newBase)
        (*newBase)->stackPos = newBase;
    }
  }

  inline void    _push_chunk_to_stack(object_pool* pool) noexcept {

    auto globalCtx = agt::get_ctx();

    // Grow the stack by a factor of 2 if the stack size is equal to the stack capacity
    if (pool->chunkStackHead == pool->chunkStackTop) [[unlikely]]
      _resize_stack(pool, (pool->chunkStackTop - pool->chunkStackBase) * 2);

    auto newChunk = _new_chunk(globalCtx, pool);
    *pool->chunkStackHead = newChunk;
    newChunk->stackPos = pool->chunkStackHead;
    ++pool->chunkStackHead;
  }

  inline void    _pop_chunk_from_stack(object_pool* pool) noexcept {

    _release_chunk(_top_of_stack(pool));
    --pool->chunkStackHead;

    const ptrdiff_t stackCapacity = pool->chunkStackTop - pool->chunkStackBase;

    // Shrink the stack capacity by half if stack size is 1/4th of the current capacity.
    if (pool->chunkStackHead - pool->chunkStackBase == stackCapacity / 4) [[unlikely]]
      _resize_stack(pool, stackCapacity / 2);
  }

  inline void    _find_alloc_chunk(object_pool* pool) noexcept {

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

    if (pool->chunkStackFullHead == pool->chunkStackHead)
      _push_chunk_to_stack(pool);
    pool->allocChunk = _top_of_stack(pool);
  }

  inline bool    _adjust_stack_post_free(object_pool* pool, object_chunk_t chunk) noexcept {

    bool shrunkStack = false;

    // Checks if the chunk had previously been full by checking whether it is in the "full block segment" of the stack.
    if (chunk->stackPos < pool->chunkStackFullHead) {
      _swap(chunk, *(pool->chunkStackFullHead - 1));
      --pool->chunkStackFullHead;
    }
    else if (_chunk_is_empty(pool, chunk)) [[unlikely]] {
      // If the block at the top of the stack is empty AND is distinct from this block, pop the empty block from the stack
      // and destroy it, and move this block to the top of the stack.
      auto topChunk = _top_of_stack(pool);
      if (chunk != topChunk) {
        if (_chunk_is_empty(pool, topChunk)) {
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
}


void    agt::init_pool(agt_ctx_t ctx, agt_u32_t threadId, object_pool* pool, size_t slotSize, size_t slotsPerChunk) noexcept {
  AGT_assert( std::has_single_bit(slotSize) );
  AGT_assert( std::has_single_bit(slotsPerChunk) );

  const size_t chunkSize = slotSize * slotsPerChunk;
  std::memset(pool, 0, sizeof(object_pool));


  const size_t initialStackSize = 2;
  auto chunkStack = (object_chunk_t*)::malloc(initialStackSize * sizeof(void*));

  pool->context            = ctx;
  pool->chunkSize          = chunkSize;
  pool->ownerThreadId      = threadId;
  pool->totalSlotsPerChunk = static_cast<agt_u16_t>(slotsPerChunk);
  pool->slotsPerChunk      = pool->totalSlotsPerChunk - (slotSize == 16 ? 2 : 1);
  pool->slotSize           = slotSize;
  pool->chunkStackBase     = chunkStack;
  pool->chunkStackHead     = chunkStack;
  pool->chunkStackTop      = chunkStack + initialStackSize;
  pool->chunkStackFullHead = pool->chunkStackHead;
  pool->allocChunk         = nullptr;

}

void    agt::destroy_pool(object_pool* pool) noexcept {

  union {
    struct {
      agt_u16_t head;
      agt_u16_t length;
    };
    agt_u32_t value = 0;
  };

  for (auto chunk = pool->chunkStackBase; chunk != pool->chunkStackHead; ++chunk) {
    auto c = *chunk;
    value = atomicExchange(c->delayedFreeList, 0);
    c->availableSlotCount += length;
    atomicStore(c->ownerPool, nullptr);
    // std::atomic_thread_fence(std::memory_order_release);
    _release_chunk(c);
  }
  ::free(pool->chunkStackBase);
  pool->chunkStackBase = nullptr;
  pool->chunkStackHead = nullptr;
  pool->chunkStackFullHead = nullptr;
  pool->chunkStackTop = nullptr;
  pool->allocChunk = nullptr;
  // std::memset(pool, 0, sizeof(object_pool));
}


void    agt::trim_pool(object_pool* pool) noexcept {

  constexpr static size_t InlineBufferSize = 64;

  object_chunk_t  buffer[InlineBufferSize];
  size_t          count;
  object_chunk_t* array;

  // All this extra stuff is so that the stack is copied over to a temporary buffer, given that the
  // trimming process may modify the stack order. The trimming process may also pop elements from the
  // stack, in which case count is reduced such that we do not attempt to trim already freed chunks.

  count = pool->chunkStackHead - pool->chunkStackBase;

  // Use inline buffer if count is small enough (note this should cover the vast majority of cases)
  if (count <= InlineBufferSize) [[likely]]
    array = buffer;
  else
    array = new object_chunk_t[count];

  std::memcpy(array, pool->chunkStackBase, count * sizeof(void*));


  // Try to trim every allocated chunk
  for (size_t i = 0; i < count; ++i) {
    if (_trim_chunk(pool, array[i]))
      --count;
  }


  // Release array if it was dynamically allocated.
  if (array != buffer)
    delete[] array;
}




object* agt::alloc_obj(object_pool* pool) noexcept {
  if (!pool->allocChunk) [[unlikely]]
    _find_alloc_chunk(pool);

  auto allocChunk = pool->allocChunk;


  auto slotIndex = allocChunk->nextFreeSlot;
  auto slot = reinterpret_cast<object_slot*>(reinterpret_cast<char*>(allocChunk) + (slotIndex * pool->slotSize));

  AGT_assert( slotIndex == slot->slotNumber );

  allocChunk->nextFreeSlot = slot->nextSlot;
  --allocChunk->availableSlotCount;



  if (_chunk_is_full(pool->allocChunk)) {
    _swap(pool->allocChunk, *pool->chunkStackFullHead);
    ++pool->chunkStackFullHead;
    pool->allocChunk = nullptr;
  }

  return reinterpret_cast<object*>(static_cast<void*>(slot));
}


void    agt::free_obj(object* obj, thread_state* state) noexcept {
  auto chunk = _get_chunk(obj);
  auto pool  = chunk->ownerPool;

  auto slot = reinterpret_cast<object_slot*>(static_cast<void*>(obj));

  atomicIncrement(slot->epoch);

  if (chunk->threadCtx == state) {
    slot->nextSlot = chunk->nextFreeSlot;
    chunk->nextFreeSlot = slot->slotNumber;
    ++chunk->availableSlotCount;
    _adjust_stack_post_free(pool, chunk);
  }
  else {
    _append_to_delayed_list(pool, chunk, slot);
  }
}

object* agt::retain_obj(object* obj) noexcept {
  auto chunk = _get_chunk(obj);
  atomicRelaxedIncrement(chunk->retainedRefCount);
  return obj;
}

void    agt::release_obj(object* obj) noexcept {
  _release_chunk(_get_chunk(obj));
}