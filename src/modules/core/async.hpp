//
// Created by maxwe on 2021-11-28.
//

#ifndef JEMSYS_AGATE2_ASYNC_HPP
#define JEMSYS_AGATE2_ASYNC_HPP

#include "config.hpp"
#include "agate/cast.hpp"
#include "agate/list.hpp"
#include "core/object.hpp"
#include "core/exec.hpp"
#include "core/task_queue.hpp"

#include <span>
#include <utility>

namespace agt {

  AGT_BITFLAG_ENUM(async_flags, agt_u32_t) {
    eUnbound       = 0x0,
    eEdgeTriggered = AGT_ASYNC_EDGE_TRIGGERED,
    // eUninitialized = AGT_ASYNC_IS_UNINITIALIZED,
    eMayReplace    = AGT_ASYNC_MAY_REPLACE,
    // eIsValid       = AGT_ASYNC_IS_VALID,
    // eCacheStatus   = AGT_ASYNC_CACHE_STATUS,
    eBound         = 0x800,
    eHasOwnershipOfData = 0x1000,
    eReady         = 0x2000,
    eWaiting       = 0x4000,
    eMemoryIsOwned = 0x8000, // If set, then the async object was allocated by the ctx allocator, otherwise it was allocated on the stack.
    eIsComplete    = 0x10000,
    eRetainOwnershipOnSuccess = 0x20000,
};


  enum {
    // AGT_ASYNC_DATA_ALLOCATED_CALLBACK_DATA = 0x1
    ASYNC_DATA_HAS_TASK_QUEUE_REF = 0x1,
    ASYNC_DATA_HAS_SIGNAL_TASK_QUEUE = 0x2,
  };


  inline constexpr async_flags eAsyncNoFlags       = { };
  inline constexpr async_flags eAsyncUnbound       = async_flags::eUnbound;
  inline constexpr async_flags eAsyncBound         = async_flags::eBound;
  inline constexpr async_flags eAsyncReady         = async_flags::eBound | async_flags::eReady;
  inline constexpr async_flags eAsyncWaiting       = async_flags::eBound | async_flags::eWaiting;
  inline constexpr async_flags eAsyncMemoryIsOwned = async_flags::eMemoryIsOwned;
  inline constexpr async_flags eAsyncHasOwnershipOfData = async_flags::eHasOwnershipOfData;
  inline constexpr async_flags eAsyncIsComplete    = async_flags::eIsComplete;
  inline constexpr async_flags eAsyncRetainsDataOnSuccess = async_flags::eRetainOwnershipOnSuccess;



  AGT_object(async, extends(object), aligned(16)) {
    agt_u32_t                structSize;
    agt_ctx_t                ctx;
    async_flags              flags;
    agt_status_t             status;
    async_data_t             data;
    resolve_async_callback_t resolveCallback;
    void*                    resolveCallbackData;
  };

  /*struct async {
    agt_ctx_t    ctx;
    agt_u32_t    structSize;
    async_flags  flags;
    agt_u64_t    resultValue;
    agt_status_t status;
    async_key_t  dataKey;
    async_data_t data;

  };*/


  namespace impl {
    union async_rc {
      struct {
        agt_u16_t epoch;
        agt_u16_t ownerRefCount;
        agt_u32_t weakRefCount;
      };
      agt_u64_t bits = 0;
    };

    inline static void _init(async_rc& rc, agt_u32_t weakRefs) noexcept {
      rc.epoch = 0;
      rc.ownerRefCount = 1;
      rc.weakRefCount = weakRefs + 1;
      std::atomic_thread_fence(std::memory_order_release);
    }

    /**
      * Try to acquire sole ownership over the async data object.
      *
      * If returns false, then the async data was still in use by someone else,
      * and the reference has been dropped. A new async data should be created instead.
      *
      * If returns true, then the caller is now guaranteed to be the sole owner,
      * the epoch has been advanced, and the attachedCount added to the weak reference count.
      */
    inline static bool _try_recycle(async_rc& rc, agt_u32_t weakRefs, bool hasOwnership) noexcept {
      // const auto epoch = static_cast<agt_u16_t>(key);
      async_rc next, prev;

      const auto expectedOwnerCount = static_cast<agt_u16_t>(hasOwnership);

      bool success;

      prev.bits = atomic_relaxed_load(rc.bits); // It is more efficient to have a second atomic operation guaranteed to execute, but have the conditional easily predicted than it is having only the CAS-loop as is.

      do {
        // assert(prev.epoch == epoch); // ownership is held, so the epoch should never advance.
        // assert( prev.ownerRefCount != 0 ); // likewise, ownerRefCount should always be at least 1.
        success = prev.ownerRefCount == expectedOwnerCount;
        AGT_invariant( prev.weakRefCount > 0 );
        if (success) {
          next.epoch         = prev.epoch + 1;
          next.ownerRefCount = 1;
          next.weakRefCount  = prev.weakRefCount + weakRefs; // don't need to add the 1, cause the caller already holds a weak ref.

        }
        else {
          next.epoch         = prev.epoch;
          next.ownerRefCount = prev.ownerRefCount - expectedOwnerCount;
          next.weakRefCount  = prev.weakRefCount - 1;
        }
      }
      while (!atomic_cas(rc.bits, prev.bits, next.bits));

      return success;
    }

    /// Turn a weak reference into an owner reference. Returns true on success.
    /// On failure, the weak reference is dropped.
    inline static bool _try_acquire(async_rc& rc, async_key_t key, bool& shouldFreeAsyncData) noexcept {
      const auto epoch = static_cast<agt_u16_t>(key);
      async_rc next, prev;

      bool success;

      prev.bits = atomic_relaxed_load(rc.bits); // It is more efficient to have a second atomic operation guaranteed to execute, but have the conditional easily predicted than it is having only the CAS-loop as is.

      do {
        success = prev.epoch == epoch && prev.ownerRefCount > 0; // if there are no owners, it is also failure.
        next.epoch = prev.epoch;
        AGT_invariant( prev.weakRefCount > 0 );
        if (success) {
          next.ownerRefCount = prev.ownerRefCount + 1;
          next.weakRefCount = prev.weakRefCount;
        }
        else {
          next.ownerRefCount = prev.ownerRefCount;
          next.weakRefCount = prev.weakRefCount - 1;
          shouldFreeAsyncData = next.weakRefCount == 0;
        }
      } while(!atomic_cas(rc.bits, prev.bits, next.bits));

      return success;
    }


    inline static void _retain(async_rc& rc) noexcept {
      atomic_exchange_add(rc.bits, 0x0000000100010000ULL);
      // atomic_relaxed_increment(rc.ownerRefCount);
    }

    /// Releases ownership while retaining a weak reference.
    /// Should be used by clients who might signal the async data multiple times, or for clearing an async object.
    ///
    inline static bool _release_ownership(async_rc& rc) noexcept {
      /*async_rc prev, next;
      prev.bits = atomic_relaxed_load(rc.bits);

      do {
        AGT_invariant( prev.weakRefCount > 0 );

        if (prev.weakRefCount <= 1) // If weakrefcount is 1, then this is the only weak ref remaining, and we should simply drop the whole thing. We don't have to worry about updating the value atomically, cause there are no other references to it.
          return false;

        next.epoch         = prev.epoch;
        next.ownerRefCount = prev.ownerRefCount - 1;
        next.weakRefCount  = prev.weakRefCount;
      } while(!atomic_cas(rc.bits, prev.bits, next.bits));

      return true;*/
      async_rc prev;
      prev.bits = atomic_exchange_add(rc.bits, -0x10000);
      AGT_invariant( prev.weakRefCount > 0 );
      return prev.weakRefCount == 1;
    }

    /// Returns true if the async data object should be freed.
    inline static bool _drop_ownership(async_rc& rc) noexcept {
      async_rc prev;
      prev.bits = atomic_exchange_add(rc.bits,  -0x100010000ULL);
      AGT_invariant( prev.weakRefCount > 0 );
      return prev.weakRefCount == 1;
      /*async_rc prev, next;

      prev.bits = atomic_relaxed_load(rc.bits);

      do {
        next.epoch = prev.epoch;
        next.ownerRefCount = prev.ownerRefCount - 1;
        next.weakRefCount = prev.weakRefCount - 1;
      } while(!atomic_cas(rc.bits, prev.bits, next.bits));

      return next.weakRefCount == 0;*/
    }

    /// Returns true if the async data object should be freed.
    inline static bool _drop_nonowner(async_rc& rc) noexcept {
      return atomic_decrement(rc.weakRefCount) == 0;
    }


    inline static uint32_t _count_weak_refs(async_rc& rc) noexcept {
      async_rc val;
      val.bits = atomic_relaxed_load(rc.bits);
      return val.weakRefCount - val.ownerRefCount;
    }
  }

  struct local_event_executor;
  struct local_event_eagent;



  /*struct blocked_queue {
    agt::block_kind  blockKind;
    agt_u32_t        blockedMsgQueueSize;
    agt_message_t    blockedMsgQueueHead;
    agt_message_t*   blockedMsgQueueTail;
    agt_u64_t        blockTimeoutDeadline;
    union {
      agt_async_t* const * blockedOps; ///< Used if blocked on any/many
      agt_async_t*         blockedOp;  ///< Used if blocked on a single op
    };
    agt_u32_t        blockedManyCount;
    agt_u32_t        blockedManyFinishedCount;
    agt_u32_t*       asyncOpIndices;
    size_t*          anyIndexPtr;
    // std::jmp_buf     contextStorage;
  };*/


  inline constexpr static size_t LocalAsyncUserDataSize = 32;

  // figure out some way to align to 64 bytes maybe?
  AGT_abstract_object(async_data) {
    // TODO: Add async_rc impl
    agt_u32_t      scData; // holds flags for local_async_data, for shared_async_data, this is the callback table index.
    impl::async_rc rc;
  };

  AGT_object(local_async_data, extends(async_data), aligned(64)) {
    locked_task_queue blockedQueue;
    agt_u8_t          userData[LocalAsyncUserDataSize];
  };

  AGT_object(shared_async_data, extends(async_data)) {
    agt_instance_id_t sourceInstance; //
  };

  AGT_object(imported_async_data) {
    shared_async_data* ptr;
    agt_handle_t       sharedDataHandle;
  };


  template <>
  struct handle_casting<local_async_data, async_data_t> {
    AGT_forceinline static bool        isa(async_data_t handle) noexcept {
      return (static_cast<uintptr_t>(handle) & 0x1) == 0;
    }
    AGT_forceinline static local_async_data* cast(async_data_t handle) noexcept {
      return reinterpret_cast<local_async_data*>(static_cast<uintptr_t>(handle));
    }
  };
  template <>
  struct handle_casting<imported_async_data, async_data_t> {
    AGT_forceinline static bool        isa(async_data_t handle) noexcept {
      return (static_cast<uintptr_t>(handle) & 0x3) == 0x3;
    }
    AGT_forceinline static imported_async_data* cast(async_data_t handle) noexcept {
      return reinterpret_cast<imported_async_data*>(static_cast<uintptr_t>(handle) & ~0x3ULL);
    }
  };



  AGT_forceinline static async_data_t to_handle(agt::local_async_data* data) noexcept {
    return static_cast<async_data_t>(reinterpret_cast<uintptr_t>(data));
  }


  inline imported_async_data* get_imported_async_data() noexcept {
    return nullptr;// TODO: Implement
  }


  AGT_forceinline static bool async_data_is_attached(async_data_t asyncData) noexcept {
    return asyncData != async_data_t{};
  }


  AGT_forceinline static async_key_t get_local_async_key(const local_async_data* data) noexcept {
    return static_cast<async_key_t>(data->rc.epoch);
  }


  local_async_data* alloc_local_async_data(agt_ctx_t ctx, agt_u32_t weakRefs) noexcept;

  local_async_data* recycle_local_async_data(agt_ctx_t ctx, async_data_t data, agt_u32_t weakRefs, bool hasOwnership) noexcept;

  void              drop_local_async_data(async_data_t data, bool hasOwnership) noexcept;



  // Same as the other overload, but also returns true in refCountIsZero if the
  // ref count was dropped to zero on failure. This is needed to support indirect messages
  local_async_data* try_acquire_local_async(async_data_t asyncData, async_key_t key, bool& refCountIsZero) noexcept;


  /**
   *
   * @param asyncData
   * @param key
   * @return the address of the bound signal, whose type depends on the bound operation.
   *         The caller should generally be aware of said type, but need be, it can be ascertained by
   *         the value of the returned object's type field. If asyncData is no longer valid, null is
   *         returned and the ref to asyncData is dropped.
   */
  inline static local_async_data* try_acquire_local_async(async_data_t asyncData, async_key_t key) noexcept {
    bool refCountIsZero;
    return try_acquire_local_async(asyncData, key, refCountIsZero);
  }


  /**
   *
   * @param asyncData
   * @param retain If true, a weak reference is retained.
   * @returns returns true if asyncData is no longer valid and should be discarded (NOTE: this may still occur even if retain is true)
   */
  bool              release_local_async(async_data_t asyncData, bool retain = false) noexcept;

  AGT_forceinline static bool release_local_async(local_async_data* asyncData, bool retain = false) noexcept {
    return release_local_async(to_handle(asyncData), retain);
  }


  // FUNCTION RETURNS TRUE IF ASYNC DATA IS STILL VALID
  template <typename Fn> requires std::is_invocable_r_v<bool, Fn, local_async_data*>
  inline static bool try_notify_local_async(async_data_t asyncData, async_key_t key, Fn&& func) noexcept {
    bool discarded = true;
    if (auto data = try_acquire_local_async(asyncData, key)) {
      bool shouldRetain = std::invoke_r<bool>(func, data);
      discarded = release_local_async(data, shouldRetain);
    }
    return !discarded;
  }

  template <typename Fn> requires std::is_invocable_r_v<bool, Fn, agt_ctx_t, local_async_data*>
  inline static bool try_notify_local_async(agt_ctx_t ctx, async_data_t asyncData, async_key_t key, Fn&& func) noexcept {
    bool discarded = true;
    if (auto data = try_acquire_local_async(asyncData, key)) {
      bool shouldRetain = std::invoke_r<bool>(func, ctx, data);
      discarded = release_local_async(data, shouldRetain);
    }
    return !discarded;
  }


  AGT_forceinline static void wake_all_local_async(agt_ctx_t ctx, async_data_t asyncData) noexcept {
    const auto localData = unsafe_handle_cast<local_async_data>(asyncData);
    AGT_resolve_ctx(ctx);
    wake_all(ctx, localData->blockedQueue);
  }

  AGT_forceinline static void wake_one_local_async(agt_ctx_t ctx, async_data_t asyncData) noexcept {
    const auto localData = unsafe_handle_cast<local_async_data>(asyncData);
    wake_one(ctx, localData->blockedQueue);
  }

  AGT_forceinline static void wake_all_local_async(agt_ctx_t ctx, async_data_t asyncData, task_queue_predicate_t predicate) noexcept {
    const auto localData = unsafe_handle_cast<local_async_data>(asyncData);
    wake_all(ctx, localData->blockedQueue, predicate, &localData->userData[0]);
  }

  AGT_forceinline static void wake_one_local_async(agt_ctx_t ctx, async_data_t asyncData, task_queue_predicate_t predicate) noexcept {
    const auto localData = unsafe_handle_cast<local_async_data>(asyncData);
    wake_one(ctx, localData->blockedQueue, predicate, &localData->userData[0]);
  }

  agt_status_t local_async_wait(async& async, agt_u64_t* pResult) noexcept;

  agt_status_t local_async_wait_for(async& async, agt_u64_t* pResult, agt_timeout_t timeout) noexcept;


  /*
  AGT_forceinline static agt_status_t wait_for_local_async(agt_ctx_t ctx, agt_u64_t* pResult, async_data_t asyncData, agt_timestamp_t deadline, resolve_async_callback_t resolveCallback, void* callbackData) noexcept {
    return block_executor(ctx->executor, ctx->boundAgent, asyncData, deadline, pResult, resolveCallback, callbackData);
  }

  AGT_forceinline static agt_status_t wait_for_local_async(const async& async, agt_u64_t* pResult) noexcept {
    const auto ctx = async.ctx;
    return block_executor(ctx->executor, ctx->boundAgent, async.data, 0, pResult, async.resolveCallback, async.resolveCallbackData);
  }

  AGT_forceinline static agt_status_t wait_for_local_async_until(const async& async, agt_u64_t* pResult, agt_timestamp_t deadline) noexcept {
    const auto ctx = async.ctx;
    return block_executor(ctx->executor, ctx->boundAgent, async.data, deadline, pResult, async.resolveCallback, async.resolveCallbackData);
  }*/

  /*AGT_forceinline static agt_status_t wait_for_local_async(async& async, agt_u64_t* pResult, ) noexcept {

  }*/


  local_async_data* local_async_bind(async& async, agt_u32_t weakRefs) noexcept;

  void              local_async_clear(async& async) noexcept;

  void              local_async_destroy(async& async) noexcept;

  agt_status_t      local_async_status(async& async, agt_u64_t* pResult) noexcept;

  void AGT_stdcall  destroy_async_local(agt_async_t async) noexcept;


  AGT_forceinline void*        get_local_async_user_data(agt::local_async_data* data) noexcept {
    return &data->userData[0];
  }



  void         async_copy_to(const async& fromAsync, async& toAsync) noexcept;
  void         async_clear(async& async) noexcept;
  void         async_destroy(async& async, bool wipeMemory = true) noexcept;







  std::pair<async_data_t, async_key_t> async_attach_shared(async& async, agt_u32_t expectedCount, agt_u32_t attachedCount) noexcept;

  std::pair<async_data_t, async_key_t> async_attach_local(async& async, agt_u32_t expectedCount, agt_u32_t attachedCount) noexcept;


  // Functionally equivalent to calling "asyncWait" with a timeout of 0, but slightly optimized
  // TODO: Benchmark to see whether or not there is actually any speedup from this, or if the gains are outweighed by the optimized instruction caching
  agt_status_t async_status(async& async) noexcept;

  bool         async_is_complete(async& async) noexcept;

  void         init_async(agt_ctx_t ctx, agt_async_t& async, agt_async_flags_t flags) noexcept;

  inline static async* init_inline_async_local(agt_ctx_t ctx, agt_inline_async_t& async, agt_async_flags_t flags) noexcept {
    auto a = reinterpret_cast<agt::async*>(&async);
    a->ctx = ctx;
    a->flags = async_flags{ flags };
    a->structSize = static_cast<agt_u32_t>(sizeof(agt_inline_async_t));
    a->data = {};
    a->status = AGT_SUCCESS;
    a->resolveCallback = nullptr;
    a->resolveCallbackData = nullptr;
    return a;
  }



  struct local_agent_message_async_data {
    agt_bool_t   isComplete;
    agt_status_t status;
    agt_u64_t    responseValue;
  };

  /*struct local_indirect_agent_message_async_data {
    agt_u32_t
    agt_bool_t   isComplete;
    agt_status_t status;
    agt_u64_t    responseValue;
  };*/



}

#endif//JEMSYS_AGATE2_ASYNC_HPP
