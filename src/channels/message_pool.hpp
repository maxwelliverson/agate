//
// Created by maxwe on 2022-05-27.
//

#ifndef AGATE_MESSAGE_POOL_HPP
#define AGATE_MESSAGE_POOL_HPP

#include "fwd.hpp"


#include <cstdint>

namespace agt {


  // Assume message_pool_t has infinite capacity. Acquiring a message should never fail due to the pool
  // having "run out of messages".
  // This might (should) change in the future.


  /// Simply increments the reference count of pool
  void          retain_message_pool(message_pool_t pool) noexcept;

  /// Will return null if pool is a fixed size pool, and the fixed size is less than the requested messageSize.
  agt_message_t acquire_message(agt_ctx_t ctx, message_pool_t pool, size_t messageSize) noexcept;

  /// Will return null if pool is not a fixed size pool
  agt_message_t acquire_sized_message(agt_ctx_t ctx, message_pool_t pool) noexcept;

  /// Will fail if message was acquired from a fixed capacity channel (ie. not a message_pool_t)
  agt_status_t  release_message(agt_ctx_t ctx, agt_message_t message) noexcept;

  /// Will implode if message was not acquired from pool
  void          release_message(agt_ctx_t ctx, message_pool_t pool, agt_message_t message) noexcept;

  /// Decrements the reference count of pool, and if this was the last reference, destroys pool.
  void          release_message_pool(agt_ctx_t ctx, message_pool_t pool) noexcept;




  // struct message_pool_block;


  struct message_pool_header {
    uint32_t         poolType : 6;
    uint32_t         refCount : 26; ///< Maximum possible references: ~67 million
  };

  struct sized_message_pool_header : message_pool_header {
    uint32_t         slotCount;
    size_t           messageSize;
  };

  struct private_sized_message_pool : sized_message_pool_header {
    uintptr_t        blockMask;
    message_block_t  allocBlock;
    message_block_t* blockStackBase;
    message_block_t* blockStackTop;
    message_block_t* blockStackHead;
    message_block_t* blockStackFullHead;
  };

  struct private_message_pool : message_pool_header {
    uint32_t                     tableSize;
    uint32_t                     tombstoneCount;
    uint32_t                     entryCount;
    uint32_t                     blockSize;
    uint32_t                     messageSizeAlignFactor;
    private_sized_message_pool** sizedPoolTable;
  };





  struct local_mpsc_sized_message_pool : message_pool_header {};

  struct local_mpmc_sized_message_pool : message_pool_header {};

  struct local_spsc_sized_message_pool : message_pool_header {};

  struct local_spmc_sized_message_pool : message_pool_header {};




  struct local_mpsc_message_pool : message_pool_header {};

  struct local_mpmc_message_pool : message_pool_header {};

  struct local_spsc_message_pool : message_pool_header {};

  struct local_spmc_message_pool : message_pool_header {};



  struct shared_mpsc_sized_message_pool : message_pool_header {};

  struct shared_mpmc_sized_message_pool : message_pool_header {};

  struct shared_spsc_sized_message_pool : message_pool_header {};

  struct shared_spmc_sized_message_pool : message_pool_header {};




  struct shared_mpsc_message_pool : message_pool_header {};

  struct shared_mpmc_message_pool : message_pool_header {};

  struct shared_spsc_message_pool : message_pool_header {};

  struct shared_spmc_message_pool : message_pool_header {};




  agt_status_t createInstance(private_sized_message_pool*& poolRef,    agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(private_message_pool*& poolRef,          agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_mpsc_sized_message_pool*& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_mpsc_message_pool*& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_spsc_sized_message_pool*& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_spsc_message_pool*& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_mpmc_sized_message_pool*& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_mpmc_message_pool*& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_spmc_sized_message_pool*& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_spmc_message_pool*& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;

  agt_status_t createInstance(shared_mpsc_sized_message_pool*& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_mpsc_message_pool*& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_spsc_sized_message_pool*& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_spsc_message_pool*& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_mpmc_sized_message_pool*& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_mpmc_message_pool*& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_spmc_sized_message_pool*& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_spmc_message_pool*& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;

  agt_message_t acquire_message(agt_ctx_t ctx, private_sized_message_pool* pool) noexcept;
  void          release_message(agt_ctx_t ctx, private_sized_message_pool* pool, agt_message_t message) noexcept;
  void          release_message_pool(agt_ctx_t ctx, private_sized_message_pool* pool) noexcept;



  static_assert(sizeof(private_sized_message_pool) == AGT_CACHE_LINE);
  static_assert(sizeof(private_message_pool) == (AGT_CACHE_LINE / 2));
}

#endif//AGATE_MESSAGE_POOL_HPP
