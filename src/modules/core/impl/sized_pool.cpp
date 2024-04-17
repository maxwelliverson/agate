//
// Created by maxwe on 2024-03-14.
//

#include "sized_pool.hpp"


void agt::impl::_push_new_solo_pool_chunk(agt_ctx_t ctx, sized_pool &pool, agt_flags32_t flags) noexcept {
  constexpr static size_t ChunkAlignment  = AGT_CACHE_LINE;
  constexpr static size_t ChunkHeaderSize = sizeof(pool_chunk);

  const size_t slotsPerChunk = pool.slotsPerChunk;

  const size_t slotSize = _get_pool_slot_size(pool);

  const size_t totalChunkSize = align_to<ChunkAlignment>(slotSize * slotsPerChunk);

  void* mem = instance_mem_alloc(get_instance(ctx), totalChunkSize, ChunkAlignment);


  auto chunk = new (mem) pool_chunk;

  AGT_invariant( (slotSize % PoolBasicUnitSize) == 0 );

  const size_t    initialSlotOffset = (flags & IS_SMALL_FLAG) ? 2 * ChunkHeaderSize : ChunkHeaderSize;
  const agt_u16_t initialSlotIndex  = static_cast<agt_u16_t>(initialSlotOffset / PoolBasicUnitSize);
  const agt_u16_t maxSlotIndex      = static_cast<agt_u16_t>(totalChunkSize / PoolBasicUnitSize);
  const agt_u16_t trueSlotCount     = static_cast<agt_u16_t>(slotsPerChunk - ((initialSlotOffset + slotSize - 1) / slotSize));
  const agt_u16_t slotIndexStride   = static_cast<agt_u16_t>(slotSize / PoolBasicUnitSize);


  auto slotPtr = reinterpret_cast<char*>(&_get_slot(chunk, initialSlotIndex));

  AGT_invariant( ((char*)mem) + initialSlotOffset == slotPtr );

  for (agt_u16_t i = initialSlotIndex; i < maxSlotIndex; slotPtr += slotSize) {
    auto slot = reinterpret_cast<pool_slot*>(slotPtr);
    slot->self = i;
    i += slotIndexStride;
    slot->next = i;
  }

  chunk->freeSlotList.nextSlot = initialSlotIndex;
  chunk->freeSlotList.length   = trueSlotCount;
  chunk->totalSlotCount        = trueSlotCount;
  chunk->slotSize              = static_cast<agt_u16_t>(slotSize);
  chunk->weakRefCount          = 1;
  chunk->flags                 = flags;
  chunk->initialSlot           = initialSlotIndex;

  chunk->chunkSize             = static_cast<agt_u32_t>(totalChunkSize);
  chunk->ownerPool             = std::addressof(pool);
  chunk->ctx                   = ctx;

  chunk->delayedFreeList.bits = 0;
  chunk->threadId = get_thread_id();
  std::atomic_thread_fence(std::memory_order_release);

  _push_chunk_to_stack(pool, chunk);
}

void agt::impl::_push_new_dual_pool_chunk(agt_ctx_t ctx, sized_pool &smallPool, sized_pool &largePool, size_t slotsPerChunk, agt_flags32_t flags) noexcept {
  constexpr static size_t ChunkAlignment  = AGT_CACHE_LINE;
  constexpr static size_t ChunkHeaderSize = sizeof(pool_chunk);
  constexpr static size_t SlotTableOffset = 2 * ChunkHeaderSize;

  const size_t smallSize = _get_pool_slot_size(smallPool);
  const size_t largeSize = _get_pool_slot_size(largePool);

  const size_t slotStride = smallSize + largeSize;
  const size_t totalChunkSize = align_to<ChunkAlignment>(slotStride * slotsPerChunk);

  void* mem = instance_mem_alloc(get_instance(ctx), totalChunkSize, ChunkAlignment);


  auto smallChunk = new (mem) pool_chunk;
  auto largeChunk = new (smallChunk + 1) pool_chunk;

  AGT_invariant( (slotStride % PoolBasicUnitSize) == 0 );


  const size_t slotTableSize = totalChunkSize - SlotTableOffset;

  const agt_u16_t trueSmallSlotCount = (slotTableSize + largeSize) / slotStride;
  const agt_u16_t trueLargeSlotCount = slotTableSize / slotStride;
  const agt_u16_t smallInitialSlotIndex = static_cast<agt_u16_t>(SlotTableOffset / PoolBasicUnitSize);
  const agt_u16_t largeInitialSlotIndex = static_cast<agt_u16_t>((ChunkHeaderSize + smallSize) / PoolBasicUnitSize);
  const agt_u16_t maxSmallSlotIndex     = static_cast<agt_u16_t>(totalChunkSize / PoolBasicUnitSize);
  const agt_u16_t maxLargeSlotIndex     = static_cast<agt_u16_t>((totalChunkSize - ChunkHeaderSize) / PoolBasicUnitSize);
  const agt_u16_t slotIndexStride       = static_cast<agt_u16_t>(slotStride / PoolBasicUnitSize);

  auto slotPtr = reinterpret_cast<char*>(mem) + SlotTableOffset;

  agt_u16_t smallSlotIndex = smallInitialSlotIndex;
  agt_u16_t largeSlotIndex = largeInitialSlotIndex;

  do {
    if (smallSlotIndex >= maxSmallSlotIndex)
      break;
    auto smallSlot = reinterpret_cast<pool_slot*>(slotPtr);
    smallSlot->self = smallSlotIndex;
    smallSlotIndex += slotIndexStride;
    smallSlot->next = smallSlotIndex;
    slotPtr += smallSize;

    if (largeSlotIndex >= maxLargeSlotIndex)
      break;
    auto largeSlot = reinterpret_cast<pool_slot*>(slotPtr);
    largeSlot->self = largeSlotIndex;
    largeSlotIndex += slotIndexStride;
    largeSlot->next = largeSlotIndex;
    slotPtr += largeSize;

  } while(true);

  smallChunk->freeSlotList.nextSlot = smallInitialSlotIndex;
  smallChunk->freeSlotList.length   = trueSmallSlotCount;
  smallChunk->totalSlotCount        = trueSmallSlotCount;
  smallChunk->slotSize              = static_cast<agt_u16_t>(smallSize);
  smallChunk->flags                 = (flags & ~IS_BIG_FLAG) | IS_SMALL_FLAG;
  // smallChunk->chunkType             = traits::chunk_type_enum;
  smallChunk->initialSlot           = smallInitialSlotIndex;
  smallChunk->weakRefCount          = 1; // Only one, as the ref count is only decremented when both pools discard a given chunk.
  smallChunk->chunkSize             = static_cast<agt_u32_t>(totalChunkSize);
  smallChunk->ownerPool             = std::addressof(smallPool);
  smallChunk->ctx                   = ctx;

  largeChunk->freeSlotList.nextSlot = largeInitialSlotIndex;
  largeChunk->freeSlotList.length   = trueLargeSlotCount;
  largeChunk->totalSlotCount        = trueLargeSlotCount;
  largeChunk->slotSize              = static_cast<agt_u16_t>(largeSize);
  largeChunk->flags                 = (flags & ~IS_SMALL_FLAG) | IS_BIG_FLAG;
  // largeChunk->chunkType             = traits::chunk_type_enum;
  largeChunk->initialSlot           = largeInitialSlotIndex;
  largeChunk->weakRefCount          = 0;
  largeChunk->chunkSize             = static_cast<agt_u32_t>(totalChunkSize);
  largeChunk->ownerPool             = std::addressof(largePool);
  largeChunk->ctx                   = ctx;

  uintptr_t tId = get_thread_id();
  smallChunk->delayedFreeList.bits = 0;
  smallChunk->threadId = tId;
  largeChunk->delayedFreeList.bits = 0;
  largeChunk->threadId = tId;
  std::atomic_thread_fence(std::memory_order_release);

  _push_chunk_to_stack(smallPool, smallChunk);
  _push_chunk_to_stack(largePool, largeChunk);
}



void agt::impl::_resize_stack(sized_pool &pool, size_t newCapacity) noexcept {
  auto newBase = (pool_chunk_t*)std::realloc(pool.stackBase, newCapacity * sizeof(void*));
  auto oldBase = pool.stackBase;
  // pool->stackTop must always be updated, but the other fields only need to be updated in the case
  // that realloc moved the location of the stack in memory.
  pool.stackTop = newBase + newCapacity;

  if (newBase != oldBase) {
    pool.stackBase      = newBase;
    pool.stackFullHead  = newBase + (pool.stackFullHead  - oldBase);
    pool.stackEmptyTail = newBase + (pool.stackEmptyTail - oldBase);
    pool.stackHead      = newBase + (pool.stackHead      - oldBase);

    // Reassign the stackPosition field for every active chunk as the stack has been moved in memory.
    for (auto pChunk = newBase; pChunk < pool.stackHead; ++pChunk)
      (*pChunk)->stackPos = pChunk;
  }
}


std::optional<std::pair<impl::pool_slot &, agt_u16_t>> agt::impl::_pop_slot_from_chunk_atomic(pool_chunk_t chunk) noexcept {
  AGT_invariant( chunk != nullptr );

  auto& list = chunk->freeSlotList;

  slot_list old, next;

  old.bits = atomic_relaxed_load(list.bits);
  pool_slot* slot;

  do {
    if (old.length == 0)
      return std::nullopt;
    slot = &_get_slot(chunk, old.nextSlot);

    AGT_invariant( slot->self == old.nextSlot ); // Should ALWAYS be true, even if slot is already allocated...

    next.length = old.length - 1;
    next.nextSlot = slot->next;
  } while(!atomic_cas(list.bits, old.bits, next.bits));

  return std::pair<pool_slot&, agt_u16_t>{ *slot, next.length };
}


agt_u16_t agt::impl::_push_slot_to_chunk_atomic(pool_chunk_t chunk, pool_slot& slot) noexcept {
  AGT_invariant( chunk != nullptr );

  auto& list = chunk->freeSlotList;

  slot_list old, next;

  old.bits = atomic_relaxed_load(list.bits);
  next.nextSlot = slot.self;

  do {
    next.length = old.length + 1;
    slot.next   = old.nextSlot;
  } while(!atomic_cas(list.bits, old.bits, next.bits));

  return next.length;
}


std::optional<bool> agt::impl::_try_resolve_deferred_ops(pool_chunk_t chunk) noexcept  {
  AGT_invariant( _chunk_is_full(chunk) );
  AGT_invariant( (chunk->flags & (IS_USER_SAFE_FLAG | IS_OWNER_SAFE_FLAG)) == IS_USER_SAFE_FLAG );


  auto& list = chunk->delayedFreeList;

  delayed_free_list old;

  if ((old.bits = atomic_exchange(list.bits, 0)) != 0) {
    chunk->freeSlotList.nextSlot = old.next;
    chunk->freeSlotList.length   = old.length;

    return old.length == chunk->totalSlotCount;
  }

  return std::nullopt;
}


bool agt::impl::_resolve_deferred_ops(pool_chunk_t chunk) noexcept {
  AGT_invariant( !_chunk_is_full(chunk) );
  AGT_invariant( (chunk->flags & (IS_USER_SAFE_FLAG | IS_OWNER_SAFE_FLAG)) == IS_USER_SAFE_FLAG );

  delayed_free_list old;

  if ((old.bits = atomic_exchange(chunk->delayedFreeList.bits, 0))) {
    _get_slot(chunk, old.head).next = chunk->freeSlotList.nextSlot;
    chunk->freeSlotList.nextSlot = old.next;
    chunk->freeSlotList.length  += old.length;

    return _chunk_is_empty(chunk);
  }

  return false;
}


bool agt::impl::_resolve_all_deferred_ops(sized_pool &pool) noexcept {

  AGT_invariant( _stack_is_full( pool ) );

  pool_chunk_t* stackBase    = pool.stackBase;
  pool_chunk_t* oldFullHead  = pool.stackHead;
  pool_chunk_t* stackPos     = oldFullHead - 1;
  pool_chunk_t* fullSwapPos  = oldFullHead;
  pool_chunk_t* emptySwapPos = pool.stackEmptyTail;

  // Some black magic shit
  for (; stackBase <= stackPos; --stackPos) {
    auto chunk = *stackPos;
    if (auto result = _try_resolve_deferred_ops(chunk)) {
      --fullSwapPos;
      pool_chunk_t fullSwapChunk = *fullSwapPos;
      if (result.value()) /* if chunk is now empty */ {
        --emptySwapPos;
        pool_chunk_t emptySwapChunk = *emptySwapPos;
        if (chunk == fullSwapChunk)
          _swap(chunk, emptySwapChunk);
        else if (emptySwapChunk == fullSwapChunk)
          _unique_swap(chunk, emptySwapChunk);
        else
          _unique_cycle_swap(chunk, emptySwapChunk, fullSwapChunk);
      }
      else /* if chunk isn't empty, but does have some free slots */
        _swap(chunk, fullSwapChunk);
    }
  }

  pool.stackFullHead  = fullSwapPos;
  pool.stackEmptyTail = emptySwapPos;

  return fullSwapPos != oldFullHead;
}


namespace {
  AGT_forceinline agt::impl::sized_pool& get_partner_pool(agt::impl::sized_pool& pool) noexcept {
    return *reinterpret_cast<agt::impl::sized_pool*>(reinterpret_cast<char*>(&pool) + pool.pairedPoolOffset);
  }

  AGT_forceinline size_t stack_size(agt::impl::sized_pool& pool) noexcept {
    return pool.stackHead - pool.stackBase;
  }
}


void impl::_pop_stack(sized_pool& pool) noexcept  {
  auto freedChunk = *--pool.stackHead;

  const ptrdiff_t stackCapacity = pool.stackTop - pool.stackBase;

  if (pool.pairedPoolOffset == 0) {
    _release_chunk(freedChunk);
  }
  else {
    freedChunk->flags |= CHUNK_IS_DISCARDED_FLAGS;
    if (_get_partner_chunk(freedChunk)->flags & CHUNK_IS_DISCARDED_FLAGS)
      _release_chunk(freedChunk);
  }

  // Shrink the stack capacity by half if stack size is 1/4th of the current capacity.
  if (pool.stackHead - pool.stackBase == stackCapacity / 4) [[unlikely]]
    _resize_stack(pool, stackCapacity / 2);
}


impl::pool_chunk_t impl::_get_free_chunk_from_partner(sized_pool &pool) noexcept {
  auto& partnerPool = get_partner_pool(pool);
  if (stack_size(pool) < stack_size(partnerPool)) {
    // Iterate through the partner stack from top to bottom, as empty chunks/nearly empty chunks should be more likely to have an empty partner.
    for (auto stackPos = partnerPool.stackHead; partnerPool.stackBase < stackPos;) {
      --stackPos;
      auto partnerChunk = *stackPos;
      auto chunk = _get_partner_chunk(partnerChunk);
      if (chunk->flags & CHUNK_IS_DISCARDED_FLAGS) {
        AGT_assert( _chunk_is_empty(chunk) );
        chunk->flags & ~CHUNK_IS_DISCARDED_FLAGS; // unset discarded flag
        return chunk;
      }
    }
  }

  return nullptr;
}


void    agt::impl::_push_new_chunk_to_stack(agt_ctx_t ctx, sized_pool& pool) noexcept {
  if (ctx == nullptr)
    ctx = get_ctx();
  if (pool.pairedPoolOffset == 0) {
    const auto flags = pool.type == object_type::user_object_pool ? IS_PRIVATELY_OWNED_FLAG : 0;
    _push_new_solo_pool_chunk(ctx, pool, flags);
  }
  else {
    auto& partnerPool = get_partner_pool(pool);
    if (pool.slotSize < partnerPool.slotSize)
      _push_new_dual_pool_chunk(ctx, pool, partnerPool, pool.slotsPerChunk, IS_DUAL_FLAG);
    else
      _push_new_dual_pool_chunk(ctx, partnerPool, pool, pool.slotsPerChunk, IS_DUAL_FLAG);
  }
}


object* agt::impl::pool_alloc(agt_ctx_t ctx, sized_pool &pool) noexcept  {
  pool_chunk_t allocChunk = _get_alloc_chunk(ctx, pool);

  auto&& [ slot, remainingCount ] = _pop_slot_from_chunk(allocChunk);

  _adjust_stack_post_alloc(/*ctx, */pool, allocChunk, remainingCount);

  return &_get_object(slot);
}

void    agt::impl::pool_free(object &obj, pool_chunk_t chunk) noexcept  {
  auto&& slot = _get_slot(obj);

  if (chunk->threadId != get_thread_id()) {
    _deferred_push_to_chunk(chunk, slot);
    return;
  }

  _push_slot_to_chunk(chunk, slot);

  _adjust_stack_post_free(*chunk->ownerPool, chunk);
}

void    agt::impl::destroy_pool(sized_pool& pool) noexcept {
  bool shouldReleaseAllChunks = true;

  if (pool.pairedPoolOffset != 0) {
    auto& partnerPool = get_partner_pool(pool);
    if (partnerPool.slotSize < pool.slotSize) {
      // pool is the large pool, only release the chunks that the partner pool has already discarded.
      shouldReleaseAllChunks = false;
      for (auto pChunk = pool.stackHead; pool.stackBase < pChunk;) {
        auto chunk = *--pChunk;
        if (_get_partner_chunk(chunk)->flags & CHUNK_IS_DISCARDED_FLAGS)
          _release_chunk(chunk);
      }
    }
  }


  if (shouldReleaseAllChunks) {
    for (auto pChunk = pool.stackHead; pool.stackBase < pChunk;)
      _release_chunk(*--pChunk);
  }

  std::free(pool.stackBase);
}
