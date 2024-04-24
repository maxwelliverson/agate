//
// Created by maxwe on 2022-05-27.
//

#ifndef AGATE_CORE_MSG_MESSAGE_POOL_HPP
#define AGATE_CORE_MSG_MESSAGE_POOL_HPP

#include "config.hpp"

#include "core/impl/object_defs.hpp"


#include <cstdint>


AGT_abstract_api_object(agt_msg_pool_st, msg_pool, ref_counted) {

};


namespace agt {

  struct message;


  // Assume message_pool_t has infinite capacity. Acquiring a message should never fail due to the pool
  // having "run out of messages".
  // This might (should) change in the future.


  /// Simply increments the reference count of pool
  void          retain_msg_pool(agt_msg_pool_t pool) noexcept;

  /// Will return null if pool is a fixed size pool, and the fixed size is less than the requested messageSize.
  agt_message_t acquire_msg(agt_msg_pool_t pool, size_t messageSize) noexcept;

  agt_status_t  acquire_msg(agt_msg_pool_t pool, size_t msgSize, agt_message_t& msg) noexcept;

  /// Will return null if pool is not a fixed size pool
  agt_message_t acquire_sized_msg(agt_msg_pool_t pool) noexcept;

  agt_status_t  acquire_sized_msg(agt_msg_pool_t pool, agt_message_t& msg) noexcept;

  /// Will implode if message was not acquired from pool
  void          release_msg(agt_msg_pool_t pool, agt_message_t message) noexcept;

  /// Decrements the reference count of pool, and if this was the last reference, destroys pool.
  void          release_msg_pool(agt_msg_pool_t pool) noexcept;




  // struct message_pool_block;


  AGT_abstract_object(sized_msg_pool, extends(agt_msg_pool_st)) {

  };

  AGT_abstract_object(unsized_msg_pool, extends(agt_msg_pool_st)) {

  };



  AGT_object(private_sized_message_pool, extends(sized_msg_pool), aligned(16)) {
    agt_ctx_t        ctx;
    uint32_t         slotCount;
    uint32_t         messageSize;
    uintptr_t        blockMask;
    message_block_t  allocBlock;
    message_block_t* blockStackBase;
    message_block_t* blockStackTop;
    message_block_t* blockStackHead;
    message_block_t* blockStackFullHead;
  };

  AGT_object(private_message_pool, extends(unsized_msg_pool), aligned(16)) {
    agt_ctx_t                    ctx;
    uint32_t                     tableSize;
    uint32_t                     tombstoneCount;
    uint32_t                     entryCount;
    uint32_t                     blockSize;
    uint32_t                     messageSizeAlignFactor;
    private_sized_message_pool** sizedPoolTable;
  };





  AGT_object(local_mpsc_sized_message_pool, extends(sized_msg_pool)) {
    uint32_t         slotCount;
    size_t           messageSize;
  };

  AGT_object(local_mpmc_sized_message_pool, extends(sized_msg_pool)) {
    uint32_t         slotCount;
    size_t           messageSize;
  };

  AGT_object(local_spsc_sized_message_pool, extends(sized_msg_pool)) {
    uint32_t         slotCount;
    size_t           messageSize;
  };

  AGT_object(local_spmc_sized_message_pool, extends(sized_msg_pool)) {
    uint32_t         slotCount;
    size_t           messageSize;
  };




  AGT_object(local_mpsc_message_pool, extends(unsized_msg_pool)) {};

  AGT_object(local_mpmc_message_pool, extends(unsized_msg_pool)) {};

  AGT_object(local_spsc_message_pool, extends(unsized_msg_pool)) {};

  AGT_object(local_spmc_message_pool, extends(unsized_msg_pool)) {};



  AGT_object(shared_mpsc_sized_message_pool, extends(sized_msg_pool)) {
    uint32_t         slotCount;
    size_t           messageSize;
  };

  AGT_object(shared_mpmc_sized_message_pool, extends(sized_msg_pool)) {
    uint32_t         slotCount;
    size_t           messageSize;
  };

  AGT_object(shared_spsc_sized_message_pool, extends(sized_msg_pool)) {
    uint32_t         slotCount;
    size_t           messageSize;
  };

  AGT_object(shared_spmc_sized_message_pool, extends(sized_msg_pool)) {
    uint32_t         slotCount;
    size_t           messageSize;
  };




  AGT_object(shared_mpsc_message_pool, extends(unsized_msg_pool)) {};

  AGT_object(shared_mpmc_message_pool, extends(unsized_msg_pool)) {};

  AGT_object(shared_spsc_message_pool, extends(unsized_msg_pool)) {};

  AGT_object(shared_spmc_message_pool, extends(unsized_msg_pool)) {};




  agt_status_t createInstance(private_sized_message_pool& poolRef,    agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(private_message_pool& poolRef,          agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_mpsc_sized_message_pool& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_mpsc_message_pool& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_spsc_sized_message_pool& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_spsc_message_pool& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_mpmc_sized_message_pool& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_mpmc_message_pool& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_spmc_sized_message_pool& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(local_spmc_message_pool& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;

  agt_status_t createInstance(shared_mpsc_sized_message_pool& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_mpsc_message_pool& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_spsc_sized_message_pool& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_spsc_message_pool& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_mpmc_sized_message_pool& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_mpmc_message_pool& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_spmc_sized_message_pool& poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;
  agt_status_t createInstance(shared_spmc_message_pool& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept;




  agt_message_t acquire_message(agt_ctx_t ctx, private_sized_message_pool* pool) noexcept;
  void          release_message(agt_ctx_t ctx, private_sized_message_pool* pool, agt_message_t message) noexcept;
  void          release_message_pool(agt_ctx_t ctx, private_sized_message_pool* pool) noexcept;

  agt_message_t acquire_message(agt_ctx_t ctx, local_spmc_message_pool* pool, size_t size) noexcept;
  void          release_message(agt_ctx_t ctx, local_spmc_message_pool* pool, size_t size, agt_message_t message) noexcept;


}

#endif//AGATE_CORE_MSG_MESSAGE_POOL_HPP
