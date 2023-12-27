//
// Created by maxwe on 2023-01-12.
//

#ifndef AGATE_POOL_H
#define AGATE_POOL_H

#include <agate/core.h>

AGT_begin_c_namespace

/* ============[ Fixed Size Memory Pool ]============ */


AGT_core_api agt_status_t        AGT_stdcall agt_new_pool(agt_ctx_t ctx, agt_pool_t* pPool, agt_size_t fixedSize, agt_pool_flags_t flags) AGT_noexcept;
AGT_core_api agt_status_t        AGT_stdcall agt_reset_pool(agt_pool_t pool) AGT_noexcept;
AGT_core_api void                AGT_stdcall agt_destroy_pool(agt_pool_t pool) AGT_noexcept;

AGT_core_api void*               AGT_stdcall agt_pool_alloc(agt_pool_t pool) AGT_noexcept;
AGT_core_api void                AGT_stdcall agt_pool_free(agt_pool_t pool, void* allocation) AGT_noexcept;





/* ============[ Reference Counted Memory Pool ]============ */

/*
 * Reference Counted Memory Pool API Guide
 *
 * agt_rcpool_t is a memory pool similar to agt_pool_t,
 * but where allocations are flexibly reference counted.
 *
 * The ability to reset the memory pool en-mass is lost in exchange
 * for reference counting with both strong and weak reference semantics.
 *
 * A weak reference consists of a pair of opaque values;
 *    - agt_weak_ref_t: an opaque reference token; refers to a reference
 *                      counted allocation that may or may not be valid.
 *    - agt_epoch_t:    an opaque epoch value; acts as a verifier for
 *                      the token.
 *
 *
 * agt_rc_alloc(pool, initialCount):
 *      Creates a new allocation from pool with an
 *      initial reference count of $initialCount.
 *
 * agt_rc_retain($alloc, $count):
 *      Increases the reference count of $alloc by $count
 *
 * agt_rc_release($alloc, $count):
 *      Decreases the reference count of $alloc by $count,
 *      releasing the allocation back to the pool from
 *      which it was allocated if the reference count has
 *      been reduced to zero.
 *
 * agt_rc_recycle($alloc, $releaseCount, $initialCount):
 *      Releases $releaseCounts references, and returns a
 *      new allocation from the same pool if the count was
 *      reduced to zero. This is an optimization primarily
 *      intended for the case where it is expected that
 *      the reference count will be reduced to zero, in
 *      which case the allocation may be reused without
 *      any interaction with the underlying pool. In
 *      either case, the reference count of the returned
 *      allocation is $initialCount.
 *
 * agt_weak_ref_take($alloc, $pEpoch, $count):
 *      Returns $count weak references referring to $alloc.
 *      This does not increase the reference count at all,
 *      and the returned weak reference must be
 *      successfully reacquired with
 *      agt_acquire_from_weak_ref to be accessed. $pEpoch
 *      must not be null, as the weak references' epoch
 *      is returned by writing to the memory pointed to
 *      by $pEpoch.
 *      NOTE: Despite "returning" $count weak references,
 *      only a single token/epoch pair is actually
 *      returned. In effect, each weak reference shares
 *      the same values for the token and epoch.
 *
 * agt_weak_ref_retain($ref, $epoch, $count):
 *      Acquires $count more weak references to the same
 *      allocation referred to by $ref and $epoch. If
 *      AGT_NULL_WEAK_REF is returned, the underlying
 *      allocation has already been invalidated, and the
 *      weak reference is dropped (otherwise it'd be
 *      necessary to call agt_weak_ref_drop immediately
 *      after anyways). As such, the return value should
 *      always be checked.
 *
 * agt_weak_ref_drop($ref, $epoch, $count):
 *      Drops $count weak references to the allocation
 *      referred to by $ref and $epoch. Take note that
 *      other API calls may cause a weak reference to be
 *      dropped, so only call when the owner of a weak
 *      reference no longer needs to reacquire it, or when
 *      multiple weak references are to be dropped at once.
 *
 * agt_acquire_from_weak_ref($ref, $epoch, $count, $flags):
 *      Try to reacquire a strong reference to a ref
 *      counted allocation referred to by $ref and $epoch.
 *      If successful, a pointer to the allocation is
 *      returned, and its reference count is increased by
 *      $count. Also if successful, one weak reference is
 *      dropped unless AGT_WEAK_REF_RETAIN_IF_ACQUIRED is
 *      specified in $flags. If unsuccessful, NULL is
 *      returned, and one weak reference is dropped.
 *      NOTE: While the reference count on success is
 *      incremented by $count, only one weak reference is
 *      dropped in either case.
 *
 *
 * */




AGT_core_api agt_status_t        AGT_stdcall agt_new_rcpool(agt_ctx_t ctx, agt_rcpool_t* pPool, agt_size_t fixedSize, agt_pool_flags_t flags) AGT_noexcept;
AGT_core_api void                AGT_stdcall agt_destroy_rcpool(agt_rcpool_t pool) AGT_noexcept;

/**
 * Acquire ownership over a new rc allocation from the specified pool.
 * */
AGT_core_api void*               AGT_stdcall agt_rc_alloc(agt_rcpool_t pool, agt_u32_t initialRefCount) AGT_noexcept;
/**
 * Release ownership of the specified allocation, and acquire another from the pool from which it was allocated
 *
 * Semantically equivalent to calling
 *
 * In many cases, this is much more efficient than calling
 * agt_rc_release(allocation);
 * result = agt_rc_alloc(pool);
 * */
AGT_core_api void*               AGT_stdcall agt_rc_recycle(void* allocation, agt_u32_t releasedCount, agt_u32_t acquiredCount) AGT_noexcept;
AGT_core_api void*               AGT_stdcall agt_rc_retain(void* allocation, agt_u32_t count) AGT_noexcept;
AGT_core_api void                AGT_stdcall agt_rc_release(void* allocation, agt_u32_t count) AGT_noexcept;


AGT_core_api agt_weak_ref_t      AGT_stdcall agt_weak_ref_take(void* rcObj, agt_epoch_t* pEpoch, agt_u32_t count) AGT_noexcept;
AGT_core_api agt_weak_ref_t      AGT_stdcall agt_weak_ref_retain(agt_weak_ref_t ref, agt_epoch_t epoch, agt_u32_t count) AGT_noexcept;
AGT_core_api void                AGT_stdcall agt_weak_ref_drop(agt_weak_ref_t token, agt_u32_t count) AGT_noexcept;
AGT_core_api void*               AGT_stdcall agt_acquire_from_weak_ref(agt_weak_ref_t token, agt_epoch_t epoch, agt_weak_ref_flags_t flags) AGT_noexcept;




AGT_end_c_namespace

#endif//AGATE_POOL_H
