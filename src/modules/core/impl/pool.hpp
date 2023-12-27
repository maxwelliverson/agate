//
// Created by maxwe on 2023-01-03.
//

#ifndef AGATE_CORE_IMPL_POOL_HPP
#define AGATE_CORE_IMPL_POOL_HPP


#include "config.hpp"

#include "agate/align.hpp"
#include "agate/thread_id.hpp"
#include "core/error.hpp"
#include "core/object.hpp"


#include <memory>


#if !defined(AGT_POOL_BASIC_UNIT_SIZE)
#define AGT_POOL_BASIC_UNIT_SIZE 8
#endif


namespace agt {

  [[nodiscard]] inline agt_instance_t get_instance(agt_ctx_t ctx) noexcept;

  void* instance_mem_alloc(agt_instance_t instance, size_t size, size_t alignment) noexcept;
  void  instance_mem_free(agt_instance_t instance, void* ptr, size_t size, size_t alignment) noexcept;


  namespace impl {

    template <typename PoolType>
    struct pool_traits;


    enum : agt_u32_t {
      IS_USER_SAFE_FLAG       = static_cast<agt_u32_t>(thread_user_safe),
      IS_OWNER_SAFE_FLAG      = static_cast<agt_u32_t>(thread_owner_safe),
      IS_DUAL_FLAG            = 0x4,
      IS_SMALL_FLAG           = 0x8,
      IS_BIG_FLAG             = 0x10,
      IS_PRIVATELY_OWNED_FLAG = 0x20,
      IS_SHARED_FLAG          = 0x40,
      IS_REF_COUNTED_FLAG     = 0x80
    };

    inline constexpr static size_t ObjectAlignment    = alignof(void*);
    inline constexpr static size_t PoolChunkAlignment = AGT_CACHE_LINE;
    inline constexpr static size_t PoolBasicUnitSize  = AGT_POOL_BASIC_UNIT_SIZE;
    inline constexpr static size_t PoolChunkMaxSize   = PoolBasicUnitSize * (0x1 << 16);


    struct AGT_alignas(PoolBasicUnitSize) pool_slot {
      agt_u16_t next;
      agt_u16_t self;
    };

    union AGT_alignas(4) slot_list {
      struct {
        agt_u16_t nextSlot;
        agt_u16_t length;
      };
      agt_u32_t bits;
    };

    union AGT_alignas(8) delayed_free_list {
      struct {
        agt_u16_t head;
        agt_u16_t next;
        agt_u16_t length;
        agt_u16_t padding;
      };
      agt_u64_t   bits = 0;
    };

    static_assert(sizeof(slot_list) == sizeof(agt_u32_t));
    static_assert(sizeof(delayed_free_list) == sizeof(agt_u64_t));

    struct pool_chunk_header;
    using pool_chunk_t = pool_chunk_header*;
    using const_pool_chunk_t = const pool_chunk_header*;

    enum pool_chunk_type : agt_u16_t {
      private_pool_chunk,
      ctx_pool_chunk,
      user_pool_chunk,
      locked_pool_chunk,
      shared_pool_chunk
    };

    struct /*AGT_cache_aligned*/ pool_chunk_header {
      slot_list       freeSlotList;
      agt_u16_t       totalSlotCount;
      agt_u16_t       slotSize;
      agt_flags32_t   flags;
      pool_chunk_type chunkType;
      agt_u16_t       initialSlot;
      agt_u32_t       weakRefCount;
      agt_u32_t       chunkSize;
      void*           ownerPool;
      pool_chunk_t*   stackPos;
      agt_instance_t  instance;
    };
    struct private_pool_chunk_header : pool_chunk_header {};
    struct spsc_pool_chunk_header    : pool_chunk_header {};
    struct mpsc_pool_chunk_header    : pool_chunk_header {
      delayed_free_list delayedFreeList;
      uintptr_t         threadId;
    };
    struct mpmc_pool_chunk_header    : pool_chunk_header {
      // Some kind of lock?
    };


    struct basic_pool {
      // agt_u32_t     flags;
      pool_chunk_t  allocChunk;
      pool_chunk_t* stackBase;
      pool_chunk_t* stackTop;
      pool_chunk_t* stackHead;
      pool_chunk_t* stackFullHead;
      pool_chunk_t* stackEmptyTail;
      agt_u32_t     slotSize;
      agt_u32_t     slotsPerChunk;
    };


    AGT_forceinline static object*        _aligned(object* obj) noexcept {
      return std::assume_aligned<ObjectAlignment>(obj);
    }
    AGT_forceinline static const object*  _aligned(const object* obj) noexcept {
      return std::assume_aligned<ObjectAlignment>(obj);
    }
    AGT_forceinline static object&        _aligned(object& obj) noexcept {
      return *_aligned(&obj);
    }
    AGT_forceinline static const object&  _aligned(const object& obj) noexcept {
      return *_aligned(&obj);
    }

    AGT_forceinline static pool_chunk_t   _aligned(pool_chunk_t chunk) noexcept {
      return std::assume_aligned<PoolChunkAlignment>(chunk);
    }

    AGT_forceinline static object&       _get_object(pool_slot& slot) noexcept {
      return *_aligned(reinterpret_cast<object*>(static_cast<void*>(&slot)));
    }

    AGT_forceinline pool_slot&           _get_slot(pool_chunk_t chunk, agt_u16_t slot) noexcept {
      return *reinterpret_cast<pool_slot*>(reinterpret_cast<char*>(_aligned(chunk)) + (slot * PoolBasicUnitSize));
    }

    AGT_forceinline pool_slot&           _get_slot(object& obj) noexcept {
      return *reinterpret_cast<pool_slot*>(static_cast<void*>(_aligned(&obj)));
    }

    AGT_forceinline pool_chunk_t         _get_chunk(pool_slot& slot) noexcept {
      return _aligned(reinterpret_cast<pool_chunk_t>(reinterpret_cast<char*>(&slot) - (PoolBasicUnitSize * slot.self)));
    }

    AGT_forceinline pool_chunk_t         _get_chunk(object& obj) noexcept {
      return _get_chunk(_get_slot(obj));
    }




    AGT_forceinline static pool_chunk_t  _get_partner_chunk(const_pool_chunk_t chunk) noexcept {
      switch(chunk->flags & (IS_BIG_FLAG | IS_SMALL_FLAG)) {
        case 0:
          return nullptr;
        case IS_SMALL_FLAG:
          return const_cast<pool_chunk_t>(reinterpret_cast<const_pool_chunk_t>(reinterpret_cast<const char*>(chunk) + sizeof(mpsc_pool_chunk_header)));
        case IS_BIG_FLAG:
          return const_cast<pool_chunk_t>(reinterpret_cast<const_pool_chunk_t>(reinterpret_cast<const char*>(chunk) - sizeof(mpsc_pool_chunk_header)));
        AGT_no_default;
      }
    }

    AGT_forceinline static bool          _chunk_is_empty(const_pool_chunk_t chunk) noexcept {
      return chunk->freeSlotList.length == chunk->totalSlotCount;
    }

    AGT_forceinline static bool          _chunk_is_truly_empty(const_pool_chunk_t chunk) noexcept {
      AGT_invariant( _chunk_is_empty(chunk) );
      if (auto partnerChunk = _get_partner_chunk(chunk))
        return _chunk_is_empty(partnerChunk);
      return true;
    }

    AGT_forceinline static bool          _chunk_is_full(const_pool_chunk_t chunk) noexcept {
      return chunk->freeSlotList.length == 0;
      // return chunk->availableSlotCount == 0;
    }

    AGT_forceinline pool_chunk_t         _get_first_chunk_header(pool_chunk_t chunk) noexcept {
      if ( chunk->flags & IS_BIG_FLAG ) [[unlikely]]
        return reinterpret_cast<pool_chunk_t>(reinterpret_cast<char*>(chunk) - sizeof(mpsc_pool_chunk_header));
      else
        return chunk;
    }

    AGT_forceinline void                 _retain_chunk(pool_chunk_t chunk) noexcept {
      atomicRelaxedIncrement(_get_first_chunk_header(chunk)->weakRefCount);
    }

    AGT_forceinline void                 _release_chunk(pool_chunk_t chunk) noexcept {
      const auto smallChunk = _get_first_chunk_header(chunk);
      if ( atomicDecrement(smallChunk->weakRefCount) == 0)
        instance_mem_free(smallChunk->instance, smallChunk, smallChunk->chunkSize, PoolChunkAlignment);
    }


    AGT_forceinline static void          _unique_swap(pool_chunk_t chunkA, pool_chunk_t chunkB) noexcept {
      auto locA = chunkA->stackPos;
      auto locB = chunkB->stackPos;
      *locA = chunkB;
      chunkB->stackPos = locA;
      *locB = chunkA;
      chunkA->stackPos = locB;
    }

    AGT_forceinline static void          _unique_cycle_swap(pool_chunk_t chunkA, pool_chunk_t chunkB, pool_chunk_t chunkC) noexcept {
      auto posC = chunkC->stackPos;

      *chunkA->stackPos = chunkC;
      *chunkB->stackPos = chunkA;
      *posC = chunkB;
      chunkC->stackPos = chunkA->stackPos;
      chunkA->stackPos = chunkB->stackPos;
      chunkB->stackPos = posC;
    }

    inline static void                   _swap(pool_chunk_t chunkA, pool_chunk_t chunkB) noexcept {
      AGT_assert( chunkA->ownerPool == chunkB->ownerPool );
      if (chunkA != chunkB)
        _unique_swap(chunkA, chunkB);
    }

    inline static void                   _cycle_swap(pool_chunk_t chunkA, pool_chunk_t chunkB, pool_chunk_t chunkC) noexcept {
      if (chunkB == chunkC) {
        _swap(chunkA, chunkB);
        return;
      }
      else if (chunkA == chunkC) {
        _unique_swap(chunkA, chunkB);
        return;
      }
      else if (chunkA == chunkB) {
        _unique_swap(chunkC, chunkB);
        return;
      }

      _unique_cycle_swap(chunkA, chunkB, chunkC);
    }

    AGT_forceinline static bool          _stack_is_full(const basic_pool& pool) noexcept {
      return pool.stackFullHead == pool.stackHead;
    }

    AGT_forceinline static pool_chunk_t  _top_of_stack(const basic_pool& pool) noexcept {
      return *(pool.stackHead - 1);
    }

    AGT_noinline inline static void      _resize_stack(basic_pool& pool, size_t newCapacity) noexcept {
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

    AGT_forceinline inline static size_t _get_pool_slot_size(const basic_pool& pool) noexcept {
      return pool.slotSize;
    }

    AGT_forceinline inline static void   _push_chunk_to_stack(basic_pool& pool, pool_chunk_t chunk) noexcept {
      if (pool.stackHead == pool.stackTop) [[unlikely]]
        _resize_stack(pool, (pool.stackTop - pool.stackBase) * 2);

      *pool.stackHead = chunk;
      chunk->stackPos = pool.stackHead;
      ++pool.stackHead;
    }

    AGT_noinline inline void             _pop_stack(basic_pool& pool) noexcept {
      --pool.stackHead;

      const ptrdiff_t stackCapacity = pool.stackTop - pool.stackBase;

      // Shrink the stack capacity by half if stack size is 1/4th of the current capacity.
      if (pool.stackHead - pool.stackBase == stackCapacity / 4) [[unlikely]]
        _resize_stack(pool, stackCapacity / 2);
    }

    template <typename PoolType>
    AGT_noinline inline void _push_new_solo_pool_chunk(agt_instance_t instance, PoolType& pool, size_t slotsPerChunk, agt_flags32_t flags) noexcept {
      using traits = pool_traits<PoolType>;
      using chunk_type = typename pool_traits<PoolType>::chunk_type;

      constexpr static size_t ChunkAlignment  = AGT_CACHE_LINE;
      constexpr static size_t ChunkHeaderSize = sizeof(chunk_type);

      const size_t slotSize = _get_pool_slot_size(pool);

      const size_t totalChunkSize = align_to<ChunkAlignment>(slotSize * slotsPerChunk);

      void* mem = instance_mem_alloc(instance, totalChunkSize, ChunkAlignment);


      auto chunk = new (mem) chunk_type;
      pool_chunk_header* chunkBase = chunk;

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

      chunkBase->freeSlotList.nextSlot = initialSlotIndex;
      chunkBase->freeSlotList.length   = trueSlotCount;
      chunkBase->totalSlotCount        = trueSlotCount;
      chunkBase->slotSize              = static_cast<agt_u16_t>(slotSize);
      chunkBase->flags                 = flags;
      chunkBase->chunkType             = traits::chunk_type_enum;
      chunkBase->initialSlot           = initialSlotIndex;
      chunkBase->weakRefCount          = 1;
      chunkBase->chunkSize             = static_cast<agt_u32_t>(totalChunkSize);
      chunkBase->ownerPool             = std::addressof(pool);
      chunkBase->instance              = instance;


      if constexpr (std::same_as<mpsc_pool_chunk_header, chunk_type>) {
        chunk->delayedFreeList.bits = 0;
        chunk->threadId = get_thread_id();
        std::atomic_thread_fence(std::memory_order_release);
      }
      else if constexpr (std::same_as<mpmc_pool_chunk_header, chunk_type>) {
        // TODO: Init lock
      }

      _push_chunk_to_stack(pool, chunk);
    }

    template <typename PoolType>
    AGT_noinline inline void _push_new_dual_pool_chunk(agt_instance_t instance, PoolType& smallPool, PoolType& largePool, size_t slotsPerChunk, agt_flags32_t flags) noexcept {
      using traits = pool_traits<PoolType>;
      using chunk_type = typename traits::chunk_type;

      constexpr static size_t ChunkAlignment  = AGT_CACHE_LINE;
      constexpr static size_t ChunkHeaderSize = sizeof(chunk_type);
      constexpr static size_t SlotTableOffset = 2 * ChunkHeaderSize;

      const size_t smallSize = _get_pool_slot_size(smallPool);
      const size_t largeSize = _get_pool_slot_size(largePool);

      const size_t slotStride = smallSize + largeSize;
      const size_t totalChunkSize = align_to<ChunkAlignment>(slotStride * slotsPerChunk);

      void* mem = instance_mem_alloc(instance, totalChunkSize, ChunkAlignment);


      auto smallChunk = new (mem) chunk_type;
      auto largeChunk = new (smallChunk + 1) chunk_type;
      pool_chunk_header* smallChunkBase = smallChunk;
      pool_chunk_header* largeChunkBase = largeChunk;

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

      smallChunkBase->freeSlotList.nextSlot = smallInitialSlotIndex;
      smallChunkBase->freeSlotList.length   = trueSmallSlotCount;
      smallChunkBase->totalSlotCount        = trueSmallSlotCount;
      smallChunkBase->slotSize              = static_cast<agt_u16_t>(smallSize);
      smallChunkBase->flags                 = (flags & ~IS_BIG_FLAG) | IS_SMALL_FLAG;
      smallChunkBase->chunkType             = traits::chunk_type_enum;
      smallChunkBase->initialSlot           = smallInitialSlotIndex;
      smallChunkBase->weakRefCount          = 2;
      smallChunkBase->chunkSize             = static_cast<agt_u32_t>(totalChunkSize);
      smallChunkBase->ownerPool             = std::addressof(smallPool);
      smallChunkBase->instance              = instance;

      largeChunkBase->freeSlotList.nextSlot = largeInitialSlotIndex;
      largeChunkBase->freeSlotList.length   = trueLargeSlotCount;
      largeChunkBase->totalSlotCount        = trueLargeSlotCount;
      largeChunkBase->slotSize              = static_cast<agt_u16_t>(largeSize);
      largeChunkBase->flags                 = (flags & ~IS_SMALL_FLAG) | IS_BIG_FLAG;
      largeChunkBase->chunkType             = traits::chunk_type_enum;
      largeChunkBase->initialSlot           = largeInitialSlotIndex;
      largeChunkBase->weakRefCount          = 0;
      largeChunkBase->chunkSize             = static_cast<agt_u32_t>(totalChunkSize);
      largeChunkBase->ownerPool             = std::addressof(largePool);
      largeChunkBase->instance              = instance;

      if constexpr (std::same_as<mpsc_pool_chunk_header, chunk_type>) {
        uintptr_t tId = get_thread_id();
        smallChunk->delayedFreeList.bits = 0;
        smallChunk->threadId = tId;
        largeChunk->delayedFreeList.bits = 0;
        largeChunk->threadId = tId;
        std::atomic_thread_fence(std::memory_order_release);
      }
      else if constexpr (std::same_as<mpmc_pool_chunk_header, chunk_type>) {
        // TODO: Init lock
      }

      _push_chunk_to_stack(smallPool, smallChunk);
      _push_chunk_to_stack(largePool, largeChunk);
    }

    template <bool AtomicOp>
    AGT_forceinline static agt_u16_t _push_slot_to_chunk(pool_chunk_t chunk, pool_slot& slot) noexcept {

      AGT_invariant( chunk != nullptr );

      auto& list = chunk->freeSlotList;

      if constexpr (AtomicOp) {
        union {
          struct {
            agt_u16_t nextSlot;
            agt_u16_t length;
          };
        agt_u32_t bits;
        } old, next;


        old.bits = atomicRelaxedLoad(list.bits);
        next.nextSlot = slot.self;

        do {
          next.length = old.length + 1;
          slot.next   = old.nextSlot;
        } while(!atomicCompareExchangeWeak(list.bits, old.bits, next.bits));

        return next.length;
      }
      else {
        slot.next = list.nextSlot;
        list.nextSlot = slot.self;
        return ++list.length;
      }
    }

    template <bool AtomicOp>
    AGT_forceinline static std::optional<std::pair<pool_slot&, agt_u16_t>> _pop_slot_from_chunk(pool_chunk_t chunk) noexcept {

      AGT_invariant( chunk != nullptr );

      auto& list = chunk->freeSlotList;

      if constexpr (AtomicOp) {
        union {
          struct {
            agt_u16_t nextSlot;
            agt_u16_t length;
          };
          agt_u32_t bits;
        } old, next;

        old.bits = atomicRelaxedLoad(list.bits);
        pool_slot* slot;

        do {
          if (old.length == 0)
            return std::nullopt;
          slot = &_get_slot(chunk, old.nextSlot);

          AGT_invariant( slot->self == old.nextSlot ); // Should ALWAYS be true, even if slot is already allocated...

          next.length = old.length - 1;
          next.nextSlot = slot->next;
        } while(!atomicCompareExchangeWeak(list.bits, old.bits, next.bits));

        return std::pair{ *slot, next.length };
      }
      else {
        auto& slot = _get_slot(chunk, list.nextSlot);

        AGT_invariant( slot.self == list.nextSlot );

        list.nextSlot = slot.next;

        agt_u16_t newLength = --list.length;

        return std::pair<pool_slot&, agt_u16_t>{ slot, newLength };
      }
    }

    AGT_forceinline static void _deferred_push_to_chunk(pool_chunk_t chunk, pool_slot& slot) noexcept {

      auto& list = static_cast<mpsc_pool_chunk_header*>(chunk)->delayedFreeList;

      delayed_free_list old, next;

      old.bits = atomicRelaxedLoad(list.bits);

      agt_u16_t thisSlot = slot.self;
      next.next = thisSlot;
      next.padding = 0;

      do {
        AGT_invariant(old.padding == 0);
        next.head = old.head == 0 ? thisSlot : old.head;
        next.length = old.length + 1;
        slot.next = old.next;
      } while(!atomicCompareExchangeWeak(list.bits, old.bits, next.bits));
    }

    // returns nullopt if no deferred pushes were processed
    //   if deferred pushes WERE processed, return true if the chunk is now empty
    AGT_forceinline static std::optional<bool> _resolve_deferred_ops_on_full(pool_chunk_t chunk) noexcept {

      AGT_invariant( _chunk_is_full(chunk) );
      AGT_invariant( (chunk->flags & (IS_USER_SAFE_FLAG | IS_OWNER_SAFE_FLAG)) == IS_USER_SAFE_FLAG );


      auto& list = static_cast<mpsc_pool_chunk_header*>(chunk)->delayedFreeList;

      delayed_free_list old;

      if ((old.bits = atomicExchange(list.bits, 0))) {
        chunk->freeSlotList.nextSlot = old.next;
        chunk->freeSlotList.length   = old.length;

        return old.length == chunk->totalSlotCount;
      }

      return std::nullopt;
    }

    // returns true if the chunk is empty after processing deferred pushes
    AGT_forceinline static bool _resolve_deferred_ops(pool_chunk_t chunk) noexcept {
      AGT_invariant( !_chunk_is_full(chunk) );
      AGT_invariant( (chunk->flags & (IS_USER_SAFE_FLAG | IS_OWNER_SAFE_FLAG)) == IS_USER_SAFE_FLAG );


      auto& list = static_cast<mpsc_pool_chunk_header*>(chunk)->delayedFreeList;

      delayed_free_list old;

      if ((old.bits = atomicExchange(list.bits, 0))) {
        _get_slot(chunk, old.head).next = chunk->freeSlotList.nextSlot;
        chunk->freeSlotList.nextSlot = old.next;
        chunk->freeSlotList.length   += old.length;

        return _chunk_is_empty(chunk);
      }

      return false;
    }

    AGT_noinline    static bool _resolve_all_deferred_ops_on_full(basic_pool& pool) noexcept {

      AGT_invariant( _stack_is_full( pool ) );

      pool_chunk_t* stackBase    = pool.stackBase;
      pool_chunk_t* oldFullHead  = pool.stackHead;
      pool_chunk_t* stackPos     = oldFullHead - 1;
      pool_chunk_t* fullSwapPos  = oldFullHead;
      pool_chunk_t* emptySwapPos = pool.stackEmptyTail;

      for (; stackBase <= stackPos; --stackPos) {
        auto chunk = *stackPos;
        if (auto result = _resolve_deferred_ops_on_full(chunk)) {
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

      return fullSwapPos == oldFullHead;
    }


    AGT_forceinline static void _init_basic_pool(basic_pool& pool, size_t slotSize, size_t slotsPerChunk) noexcept {
      const size_t initialStackSize = 2;
      auto chunkStack = (pool_chunk_t*)::malloc(initialStackSize * sizeof(void*));

      pool.allocChunk     = nullptr;
      pool.stackBase      = chunkStack;
      pool.stackHead      = chunkStack;
      pool.stackTop       = chunkStack + initialStackSize;
      pool.stackFullHead  = chunkStack;
      pool.stackEmptyTail = chunkStack;
      pool.slotSize       = slotSize;
      pool.slotsPerChunk  = slotsPerChunk;
    }



    /*AGT_noinline    static bool _resolve_all_deferred_ops(basic_pool& pool) noexcept {
      bool            freed       = false;
      pool_chunk_t* stackBase     = pool.stackBase;
      pool_chunk_t* oldFullHead   = pool.stackFullHead;
      pool_chunk_t* stackPos      = oldFullHead - 1;
      // pool_chunk_t* swapPos       = stackPos;
      // pool_chunk_t* emptySwapPos  = pool.stackEmptyTail;

      for (; stackBase <= stackPos; --stackPos) {
        auto chunk = *stackPos;
        if (auto result = _resolve_deferred_ops_on_full(chunk)) {
          if (result.value())
            _cycle_swap(chunk, *--pool.stackEmptyTail, *--pool.stackFullHead);
          else
            _swap(chunk, *--pool.stackFullHead);
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
    }*/
  }

}

#endif//AGATE_CORE_IMPL_POOL_HPP
