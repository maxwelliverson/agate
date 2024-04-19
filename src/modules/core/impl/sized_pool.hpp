//
// Created by maxwe on 2024-03-14.
//

#ifndef AGATE_CORE_IMPL_SIZED_POOL_HPP
#define AGATE_CORE_IMPL_SIZED_POOL_HPP

#include "config.hpp"

#include "object_defs.hpp"

#include "agate/align.hpp"
#include "agate/atomic.hpp"
#include "agate/thread_id.hpp"
#include "core/error.hpp"
// #include "core/object.hpp"


#include <memory>
#include <concepts>

#if !defined(AGT_POOL_BASIC_UNIT_SIZE)
# define AGT_POOL_BASIC_UNIT_SIZE 8
#endif

namespace agt {

  [[nodiscard]] inline agt_instance_t get_instance(agt_ctx_t ctx) noexcept;

  void* instance_mem_alloc(agt_instance_t instance, size_t size, size_t alignment) noexcept;
  void  instance_mem_free(agt_instance_t instance, void* ptr, size_t size, size_t alignment) noexcept;

  namespace impl {

    enum : agt_u32_t {
      IS_USER_SAFE_FLAG        = static_cast<agt_u32_t>(thread_user_safe),
      IS_OWNER_SAFE_FLAG       = static_cast<agt_u32_t>(thread_owner_safe),
      IS_DUAL_FLAG             = 0x4,
      IS_SMALL_FLAG            = 0x8,
      IS_BIG_FLAG              = 0x10,
      IS_PRIVATELY_OWNED_FLAG  = 0x20,
      IS_SHARED_FLAG           = 0x40,
      IS_REF_COUNTED_FLAG      = 0x80,
      CHUNK_IS_DISCARDED_FLAGS = 0x100,
    };

    inline constexpr static size_t ObjectAlignment    = alignof(void*);
    inline constexpr static size_t RcObjectAlignment  = 8;
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

    struct pool_chunk;
    using pool_chunk_t = pool_chunk*;
    using const_pool_chunk_t = const pool_chunk*;

    struct sized_pool;

    struct AGT_cache_aligned pool_chunk {
      slot_list         freeSlotList;
      agt_u16_t         totalSlotCount;
      agt_u16_t         slotSize;
      agt_u32_t         weakRefCount;
      agt_flags32_t     flags;
      // pool_chunk_type chunkType;
      agt_u16_t         initialSlot;
      agt_u32_t         chunkSize;
      sized_pool*       ownerPool;
      pool_chunk_t*     stackPos;
      agt_ctx_t         ctx;
      delayed_free_list delayedFreeList;
      uintptr_t         threadId;
    };


    AGT_abstract_object(sized_pool) {
      agt_i16_t     pairedPoolOffset;
      agt_u16_t     slotSize;
      agt_u16_t     slotsPerChunk;
      agt_u32_t     lastChunkSize;

      // agt_ctx_t     ctx;

      // agt_u16_t     slotsPerChunk; // totalSlotsPerChunk - slots occupied by chunk header
      // agt_u16_t     totalSlotsPerChunk;

      // size_t        lastChunkSize;
      // size_t                chunkSize;

      pool_chunk_t  allocChunk;
      pool_chunk_t* stackBase;
      pool_chunk_t* stackTop;
      pool_chunk_t* stackHead;
      pool_chunk_t* stackFullHead;
      pool_chunk_t* stackEmptyTail;
    };


    AGT_forceinline static object*          _aligned(object* obj) noexcept {
      return std::assume_aligned<ObjectAlignment>(obj);
    }
    AGT_forceinline static const object*    _aligned(const object* obj) noexcept {
      return std::assume_aligned<ObjectAlignment>(obj);
    }
    AGT_forceinline static object&          _aligned(object& obj) noexcept {
      return *_aligned(&obj);
    }
    AGT_forceinline static const object&    _aligned(const object& obj) noexcept {
      return *_aligned(&obj);
    }
    AGT_forceinline static rc_object*       _aligned(rc_object* obj) noexcept {
      return std::assume_aligned<RcObjectAlignment>(obj);
    }
    AGT_forceinline static const rc_object* _aligned(const rc_object* obj) noexcept {
      return std::assume_aligned<RcObjectAlignment>(obj);
    }
    AGT_forceinline static rc_object&       _aligned(rc_object& obj) noexcept {
      return *_aligned(&obj);
    }
    AGT_forceinline static const rc_object& _aligned(const rc_object& obj) noexcept {
      return *_aligned(&obj);
    }

    AGT_forceinline static pool_chunk_t     _aligned(pool_chunk_t chunk) noexcept {
      return std::assume_aligned<PoolChunkAlignment>(chunk);
    }

    AGT_forceinline static object&        _get_object(pool_slot& slot) noexcept {
      return *_aligned(reinterpret_cast<object*>(static_cast<void*>(&slot)));
    }

    AGT_forceinline static pool_slot&     _get_slot(pool_chunk_t chunk, agt_u16_t slot) noexcept {
      return *reinterpret_cast<pool_slot*>(reinterpret_cast<char*>(_aligned(chunk)) + (slot * PoolBasicUnitSize));
    }

    AGT_forceinline static pool_slot&     _get_slot(object& obj) noexcept {
      return *reinterpret_cast<pool_slot*>(static_cast<void*>(_aligned(&obj)));
    }

    AGT_forceinline static pool_chunk_t   _get_chunk(pool_slot& slot) noexcept {
      return _aligned(reinterpret_cast<pool_chunk_t>(reinterpret_cast<char*>(&slot) - (PoolBasicUnitSize * slot.self)));
    }

    AGT_forceinline static pool_chunk_t   _get_chunk(object& obj) noexcept {
      return _get_chunk(_get_slot(obj));
    }



    AGT_forceinline static pool_chunk_t  _get_partner_chunk(const_pool_chunk_t chunk) noexcept {
      switch(chunk->flags & (IS_BIG_FLAG | IS_SMALL_FLAG)) {
        case 0:
          return nullptr;
        case IS_SMALL_FLAG:
          return const_cast<pool_chunk_t>(reinterpret_cast<const_pool_chunk_t>(reinterpret_cast<const char*>(chunk) + sizeof(pool_chunk)));
        case IS_BIG_FLAG:
          return const_cast<pool_chunk_t>(reinterpret_cast<const_pool_chunk_t>(reinterpret_cast<const char*>(chunk) - sizeof(pool_chunk)));
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
        return reinterpret_cast<pool_chunk_t>(reinterpret_cast<char*>(chunk) - sizeof(pool_chunk));
      return chunk;
    }



    AGT_forceinline void                 _retain_chunk(pool_chunk_t chunk) noexcept {
      atomic_relaxed_increment(_get_first_chunk_header(chunk)->weakRefCount);
    }

    AGT_forceinline void                 _release_chunk(pool_chunk_t chunk) noexcept {
      const auto smallChunk = _get_first_chunk_header(chunk);
      if ( atomic_decrement(smallChunk->weakRefCount) == 0 )
        instance_mem_free(get_instance(smallChunk->ctx), smallChunk, smallChunk->chunkSize, PoolChunkAlignment);
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

    AGT_forceinline static bool          _stack_is_full(const sized_pool& pool) noexcept {
      return pool.stackFullHead == pool.stackHead;
    }

    AGT_forceinline static pool_chunk_t  _top_of_stack(const sized_pool& pool) noexcept {
      return *(pool.stackHead - 1);
    }

    AGT_noinline void _resize_stack(sized_pool& pool, size_t newCapacity) noexcept;

    AGT_forceinline static size_t        _get_pool_slot_size(const sized_pool& pool) noexcept {
      return pool.slotSize;
    }

    AGT_forceinline static void          _push_chunk_to_stack(sized_pool& pool, pool_chunk_t chunk) noexcept {
      if (pool.stackHead == pool.stackTop) [[unlikely]]
        _resize_stack(pool, (pool.stackTop - pool.stackBase) * 2);

      *pool.stackHead = chunk;
      chunk->stackPos = pool.stackHead;
      ++pool.stackHead;
    }

    AGT_noinline void   _pop_stack(sized_pool& pool) noexcept;

    AGT_noinline    pool_chunk_t         _get_free_chunk_from_partner(sized_pool& pool) noexcept;

    AGT_forceinline /*inline*/ static pool_chunk_t _try_get_partner_chunk(sized_pool& pool) noexcept {
      if (pool.pairedPoolOffset == 0)
        return nullptr;
      return _get_free_chunk_from_partner(pool);
    }


    AGT_noinline void _push_new_solo_pool_chunk(agt_ctx_t ctx, sized_pool& pool, agt_flags32_t flags) noexcept;

    AGT_noinline void _push_new_dual_pool_chunk(agt_ctx_t ctx, sized_pool& smallPool, sized_pool& largePool, size_t slotsPerChunk, agt_flags32_t flags) noexcept;


    agt_u16_t _push_slot_to_chunk_atomic(pool_chunk_t chunk, pool_slot& slot) noexcept;

    std::optional<std::pair<pool_slot&, agt_u16_t>> _pop_slot_from_chunk_atomic(pool_chunk_t chunk) noexcept;

    AGT_forceinline static void _push_slot_to_chunk(pool_chunk_t chunk, pool_slot& slot) noexcept {

      AGT_invariant( chunk != nullptr );

      auto& list = chunk->freeSlotList;

      slot.next = list.nextSlot;
      list.nextSlot = slot.self;
      ++list.length;
    }

    AGT_forceinline static std::pair<pool_slot&, agt_u16_t> _pop_slot_from_chunk(pool_chunk_t chunk) noexcept {

      AGT_invariant( chunk != nullptr );

      auto& list = chunk->freeSlotList;

      auto& slot = _get_slot(chunk, list.nextSlot);

      AGT_invariant( slot.self == list.nextSlot );

      list.nextSlot = slot.next;

      agt_u16_t newLength = --list.length;

      return { slot, newLength };
    }

    AGT_forceinline static void _deferred_push_to_chunk(pool_chunk_t chunk, pool_slot& slot) noexcept {

      auto& list = chunk->delayedFreeList;

      delayed_free_list old, next;

      old.bits = atomic_relaxed_load(list.bits);

      agt_u16_t thisSlot = slot.self;
      next.next = thisSlot;
      next.padding = 0;

      do {
        AGT_invariant(old.padding == 0);
        next.head = old.head == 0 ? thisSlot : old.head;
        next.length = old.length + 1;
        slot.next = old.next;
      } while(!atomic_cas(list.bits, old.bits, next.bits));
    }

    // returns nullopt if no deferred pushes were processed
    //   if deferred pushes WERE processed, return true if the chunk is now empty
    AGT_noinline std::optional<bool> _try_resolve_deferred_ops(pool_chunk_t chunk) noexcept;

    AGT_noinline bool _resolve_deferred_ops(pool_chunk_t chunk) noexcept;

    AGT_noinline bool _resolve_all_deferred_ops(sized_pool& pool) noexcept;

    AGT_forceinline static void _init_sized_pool(sized_pool& pool, size_t slotSize, size_t slotsPerChunk) noexcept {
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

    AGT_noinline void _release_top_of_stack(sized_pool& pool) noexcept;
  }

  AGT_object(ctx_sized_pool, extends(impl::sized_pool)) {  };

  AGT_object(user_sized_pool, extends(impl::sized_pool)) {
    agt_ctx_t ctx;
    // Something else maybe??
  };


  [[nodiscard]] agt_ctx_t    get_ctx() noexcept;


  namespace impl {
    AGT_noinline void _push_new_chunk_to_stack(agt_ctx_t ctx, sized_pool& pool) noexcept;

    AGT_forceinline /*inline*/ static pool_chunk_t _get_alloc_chunk(agt_ctx_t ctx, sized_pool& pool) noexcept {
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
        if (_stack_is_full(pool)) [[unlikely]] {
          if (!_resolve_all_deferred_ops(pool)) {
            if (auto freeChunk = _try_get_partner_chunk(pool))
              _push_chunk_to_stack(pool, freeChunk);
            else
              _push_new_chunk_to_stack(ctx, pool);
          }
        }
        pool.allocChunk = _top_of_stack(pool);
      }

      return pool.allocChunk;
    }

    AGT_forceinline /*inline*/ static void         _adjust_stack_post_alloc(/*agt_ctx_t ctx, */sized_pool& pool, pool_chunk_t allocChunk, agt_u16_t remainingCount) noexcept {
      if ( remainingCount == 0 ) {
        _swap(allocChunk, *pool.stackFullHead);
        ++pool.stackFullHead;
        pool.allocChunk = nullptr;
      }
      else if ( pool.stackEmptyTail <= allocChunk->stackPos ) {
        _swap(allocChunk, *pool.stackEmptyTail);
        ++pool.stackEmptyTail;
      }
    }

    AGT_forceinline /*inline*/ static void         _adjust_stack_post_free(sized_pool& pool, pool_chunk_t chunk) noexcept {
      if (_chunk_is_empty(chunk)) {
        if (pool.stackEmptyTail != pool.stackHead) {
          // In this case, the empty section of the stack was non-empty before this free, and as such,
          // we can release that chunk knowing that it is not the chunk we just freed up.
          _pop_stack(pool);
        }
        --pool.stackEmptyTail;
        _swap(chunk, *pool.stackEmptyTail);
      }
      else if (chunk->stackPos < pool.stackFullHead) {
        --pool.stackFullHead;
        _swap(chunk, *pool.stackFullHead);
      }
      pool.allocChunk = chunk;
    }



    union rc_object_cas_wrapper {
      struct {
        agt_u32_t refCount;
        agt_u32_t epoch;
      };
      agt_u64_t u64;
    };


    AGT_forceinline static agt_u64_t& _get_cas_value(rc_object& obj) noexcept {
      return *std::assume_aligned<8>(reinterpret_cast<agt_u64_t*>(&_aligned(obj).refCount));
    }

    AGT_forceinline static void       _atomic_cas_init(rc_object& obj, rc_object_cas_wrapper& old) noexcept {
      old.u64 = atomic_relaxed_load(_get_cas_value(obj));
    }

    AGT_forceinline static bool       _atomic_cas(rc_object& obj, rc_object_cas_wrapper& old, rc_object_cas_wrapper next) noexcept {
      return atomic_cas(_get_cas_value(obj), old.u64, next.u64);
    }


    // Returns true if obj should be freed
    /*AGT_forceinline inline static bool _release_ref(rc_object& obj) noexcept {

    }*/






    AGT_noinline object*      pool_alloc(agt_ctx_t ctx, sized_pool& pool) noexcept;

    AGT_noinline void         pool_free(object& obj, pool_chunk_t chunk) noexcept;

    AGT_noinline void         destroy_pool(sized_pool& pool) noexcept;

    template <size_t Size>
    AGT_forceinline /*inline*/ static sized_pool&  get_ctx_pool(agt_ctx_t ctx) noexcept;

    AGT_forceinline /*inline*/ static sized_pool&  get_ctx_pool_dyn(agt_ctx_t ctx, size_t size) noexcept;


    template <typename ObjectType>
    AGT_forceinline static ObjectType* init_object(object* obj) noexcept {
      if constexpr (std::derived_from<ObjectType, rc_object>) {
        auto rcObj = static_cast<rc_object*>(obj);
        if constexpr (std::is_trivially_default_constructible_v<ObjectType>) {
          obj->type = AGT_type_id_of(ObjectType);
          rcObj->refCount = 1;
          std::atomic_thread_fence(std::memory_order_release);
          return std::launder(static_cast<ObjectType*>(obj));
        }
        else {
          const auto offset = obj->poolChunkOffset;
          const auto epoch = rcObj->epoch;
          const auto newObj = new (obj) ObjectType;
          newObj->type = AGT_type_id_of(ObjectType);
          newObj->poolChunkOffset = offset; // This is just to ensure that the compiler doesn't modify this field when the object is constructed...
          newObj->epoch = epoch;
          std::atomic_thread_fence(std::memory_order_release);
          return newObj;
        }
      }
      else {
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
    }

    template <typename ObjectType>
    AGT_forceinline inline static ObjectType*  ctx_pool_alloc(agt_ctx_t ctx, sized_pool& pool) noexcept {
      return init_object<ObjectType>(impl::pool_alloc(ctx, pool));
    }


    template <typename ObjectType, typename ...Args>
    AGT_forceinline inline static ObjectType*  recycle_rc_object(ObjectType* obj, sized_pool& pool, Args&& ...args) noexcept {
      impl::rc_object_cas_wrapper old, next;

      impl::_atomic_cas_init(obj, old);

      bool mayRecycleObject;

      pool_chunk_t chunk = _get_chunk(*obj);

      do {
        next.epoch      = old.epoch;
        mayRecycleObject = old.refCount == 1;
        if (mayRecycleObject) {
          next.refCount = 1;
          next.epoch   += 1;
        }
        else {
          next.refCount = old.refCount - 1;
        }
      } while(!impl::_atomic_cas(obj, old, next));

      if (mayRecycleObject) {
        if constexpr (has_adl_destroy<ObjectType, Args...>) {
          destroy(obj, std::forward<Args>(args)...);
        }
        return init_object<ObjectType>(&obj);
      }

      if (chunk->threadId == get_thread_id()) [[likely]]
        return ctx_pool_alloc<ObjectType>(chunk->ctx, pool);

      // Wasn't able to recycle, and the pool from which obj was allocated belongs to a different thread.
      // TODO: Decide whether or not to do different things based on whether or not the pool in question is a ctx pool or a user pool. If it was a ctx pool, there's no reason not to just default to using the current ctx, but if it was a user pool, I don't think there's anything we can really do that would be reasonable...

      if (pool.type == object_type::ctx_sized_pool) {
        auto ctx = get_ctx();
        return ctx_pool_alloc<ObjectType>(ctx, get_ctx_pool<sizeof(ObjectType)>(ctx));
      }

      return nullptr;
    }


  }

  template <typename ObjectType>
  AGT_forceinline ObjectType* pool_alloc(user_sized_pool& pool) noexcept {
    static_assert(std::default_initializable<ObjectType>);

    return impl::ctx_pool_alloc<ObjectType>(pool.ctx, pool);
  }

  template <typename ObjectType>
  AGT_forceinline inline static ObjectType* alloc(agt_ctx_t ctx) noexcept {
    static_assert(std::default_initializable<ObjectType>);

    // auto& pool = impl::get_ctx_pool<sizeof(ObjectType)>(ctx);

    return impl::ctx_pool_alloc<ObjectType>(ctx, impl::get_ctx_pool<sizeof(ObjectType)>(ctx));
  }

  template <typename ObjectType>
  AGT_forceinline inline static ObjectType* alloc_dyn(agt_ctx_t ctx, size_t size) noexcept {
    static_assert(std::default_initializable<ObjectType>);

    auto& pool = impl::get_ctx_pool<sizeof(ObjectType)>(ctx);

    return impl::ctx_pool_alloc<ObjectType>(ctx, impl::get_ctx_pool_dyn(ctx, size));
  }

  template <typename ObjectType, typename ...Args>
  AGT_forceinline void        release(ObjectType* obj, Args&& ...args) noexcept {
    // if objecttype is ref counted, release a ref, and then free if ref count is 0.
    // otherwise, simply free

    bool objectShouldBeFreed = true;

    if constexpr (std::derived_from<ObjectType, rc_object>) {
      // Use a CAS loop to ensure that when the reference count is decreased to zero,
      // the the epoch is incremented in the same atomic operation as the reference count decrease.

      impl::rc_object_cas_wrapper old, next;

      _atomic_cas_init(*obj, old);

      do {
        next.refCount = old.refCount - 1;
        next.epoch    = old.epoch;
        objectShouldBeFreed = next.refCount == 0;
        if (objectShouldBeFreed)
          next.epoch += 1;
      } while(!_atomic_cas(*obj, old, next));
    }

    if (objectShouldBeFreed) {
      if constexpr (impl::has_adl_destroy<ObjectType, Args...>) {
        destroy(obj, std::forward<Args>(args)...);
      }
      impl::pool_free(*obj, impl::_get_chunk(*obj));
    }

  }

  template <typename ObjectType, typename ...Args>
  AGT_forceinline ObjectType* recycle(ObjectType* obj, Args&& ...args) noexcept {
    static_assert(std::derived_from<ObjectType, rc_object>);

    auto& pool = *impl::_get_chunk(*obj)->ownerPool;

    return impl::recycle_rc_object(obj, pool, std::forward<Args>(args)...);
  }


  AGT_forceinline void        retain(rc_object* obj) noexcept {
    atomic_relaxed_increment(obj->refCount);
  }



  AGT_forceinline agt_epoch_t take_weak_ref(rc_object* obj) noexcept {
    impl::_retain_chunk(impl::_get_chunk(*obj));
    std::atomic_thread_fence(std::memory_order_acquire);
    return obj->epoch;
  }


  AGT_forceinline void        drop_weak_ref(rc_object* obj) noexcept {
    impl::_release_chunk(impl::_get_chunk(*obj));
  }

  AGT_forceinline bool        retain_weak_ref(rc_object* obj, agt_epoch_t key) noexcept {
    std::atomic_thread_fence(std::memory_order_acquire);
    const bool isValidRef = obj->epoch == key;

    if (isValidRef)
      impl::_retain_chunk(impl::_get_chunk(*obj));

    return isValidRef;
  }

  AGT_forceinline rc_object*  actualize_weak_ref(rc_object* obj, agt_epoch_t key) noexcept {
    impl::rc_object_cas_wrapper old, next;

    next.epoch = key;

    impl::_atomic_cas_init(*obj, old);

    do {
      if (old.refCount == UINT32_MAX) [[unlikely]] // Should pretty much never be taken, so this is very reliably predicted and the check causes effectively no slowdown
        return nullptr;
      if (old.epoch != key)
        return nullptr;
      next.refCount = old.refCount + 1;
    } while(!impl::_atomic_cas(*obj, old, next));

    return obj;
  }

}

#endif //AGATE_CORE_IMPL_SIZED_POOL_HPP
