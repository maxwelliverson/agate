//
// Created by maxwe on 2022-05-28.
//

#include "message_pool.hpp"

#include "message.hpp"
#include "modules/core/instance.hpp"

#include <bit>
#include <new>

struct agt::message_pool_block {
  union {
    agt_message_t nextAvailableSlot;
    uintptr_t     nextAvailableSlotOffset;
  };
  size_t        availableSlotCount;
  size_t        totalSlots;
  size_t        slotSize;
  agt::message_pool_block** stackPosition;
  union {
    message_pool_t pool;
    uintptr_t      poolOffset;
  };
  shared_handle poolSharedHandle; ///< Local pools ignore this value entirely
};

namespace {
  inline agt_message_t lockfree_block_alloc(agt::message_pool_block* block) noexcept {
    auto message = block->nextAvailableSlot;
    block->nextAvailableSlot = message->next;
    --block->availableSlotCount;
    return message;
  }
  inline void          lockfree_block_free(agt::message_pool_block* block, agt_message_t msg) noexcept {
    msg->next = block->nextAvailableSlot;
    block->nextAvailableSlot = msg;
    ++block->availableSlotCount;
  }


  agt::message_pool_block* create_local_block(agt_ctx_t ctx, agt::message_pool_t pool, size_t slotSize, size_t slotCount) noexcept {
    const size_t blockSize = slotSize * slotCount;
    const size_t blockAlignment = std::bit_ceil(blockSize);
    void* mem = agt::instance_mem_alloc(ctx->instance, blockSize, blockAlignment);
    const auto block = new (mem) agt::message_pool_block;
    block->nextAvailableSlot  = (agt_message_t)(((char*)mem) + slotSize);
    block->availableSlotCount = slotCount - 1;
    block->totalSlots         = slotCount - 1;
    block->slotSize           = slotSize;

    const auto lastMessage = (agt_message_t)(((char*)mem) + (slotSize * slotCount));
    auto message = block->nextAvailableSlot;
    auto nextMessage = (agt_message_t)(((char*)message) + slotSize);

    while (message != lastMessage) {
      message->next = nextMessage;
      // message->pool = pool;
      message = nextMessage;
      nextMessage = (agt_message_t)(((char*)nextMessage) + slotSize);
    }

    return block;
  }

  void destroy_block(agt_ctx_t ctx, agt::message_pool_block* block) noexcept {
    // Could there be legitimate cases where a block is destroyed with messages still in use?
    // assert(block->availableSlotCount == block->totalSlots);
    const size_t blockSize = block->slotSize * (block->totalSlots + 1);
    const size_t blockAlignment = std::bit_ceil(blockSize);
    agt::instance_mem_free(ctx->instance, block, blockSize, blockAlignment);
    // agt::ctxLocalFree(ctx, block, blockSize, blockAlignment);
  }


  inline bool block_is_empty(agt::message_pool_block* block) noexcept {
    return block->availableSlotCount == block->totalSlots;
  }

  inline bool block_is_full(agt::message_pool_block* block) noexcept {
    return block->availableSlotCount == 0;
  }

  void swap(agt::message_pool_block* blockA, agt::message_pool_block* blockB) noexcept {
    if (blockA != blockB) {
      auto locA = blockA->stackPosition;
      auto locB = blockB->stackPosition;
      *locA = blockB;
      blockB->stackPosition = locA;
      *locB = blockA;
      blockA->stackPosition = locB;
    }
  }

  inline agt::message_pool_block* top_of_stack(agt::private_sized_message_pool * pool) noexcept {
    return *(pool->blockStackHead - 1);
  }


  void resize_stack(agt::private_sized_message_pool * pool, size_t newCapacity) noexcept {
    auto newBase = (agt::message_pool_block**)std::realloc(pool->blockStackBase, newCapacity * sizeof(void*));

    // pool->blockStackTop must always be updated, but the other fields only need to be updated in the case
    // that realloc moved the location of the stack in memory.
    pool->blockStackTop = newBase + newCapacity;
    if (newBase != pool->blockStackBase) {
      pool->blockStackFullHead = newBase + (pool->blockStackFullHead - pool->blockStackBase);
      pool->blockStackHead     = newBase + (pool->blockStackHead - pool->blockStackBase);
      pool->blockStackBase     = newBase;

      // Reassign the stackPosition field for every active block as the stack has been moved in memory.
      for (; newBase < pool->blockStackHead; ++newBase)
        (*newBase)->stackPosition = newBase;
    }
  }

  void push_block_to_stack(agt_ctx_t ctx, agt::private_sized_message_pool * pool) noexcept {
    // Grow the stack by a factor of 2 if the stack size is equal to the stack capacity
    if (pool->blockStackHead == pool->blockStackTop) [[unlikely]]
      resize_stack(pool, (pool->blockStackTop - pool->blockStackBase) * 2);

    auto newBlock = create_local_block(ctx, pool, pool->messageSize + sizeof(agt_message_st), pool->slotCount);
    *pool->blockStackHead = newBlock;
    newBlock->stackPosition = pool->blockStackHead;
    ++pool->blockStackHead;
  }

  void pop_block_from_stack(agt_ctx_t ctx, agt::private_sized_message_pool * pool) noexcept {
    destroy_block(ctx, top_of_stack(pool));
    --pool->blockStackHead;

    const ptrdiff_t stackCapacity = pool->blockStackTop - pool->blockStackBase;

    // Shrink the stack capacity by half if stack size is 1/4th of the current capacity.
    if (pool->blockStackHead - pool->blockStackBase == stackCapacity / 4) [[unlikely]]
      resize_stack(pool, stackCapacity / 2);
  }



  void find_alloc_block(agt_ctx_t ctx, agt::private_sized_message_pool * pool) noexcept {

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

    if (pool->blockStackFullHead == pool->blockStackHead)
      push_block_to_stack(ctx, pool);


    pool->allocBlock = top_of_stack(pool);
  }


  void init_pool(agt_ctx_t ctx, agt::private_sized_message_pool * pool) noexcept {
    // Use malloc rather than the ctxLocalAlloc API to take advantage of realloc optimizations
    // Initial stack capacity is 2. Many pools will never need more than that.

    constexpr static size_t DefaultInitialCapacity = 2;

    auto stack = (agt::message_pool_block**)std::malloc(DefaultInitialCapacity * sizeof(void*));
    pool->blockStackBase     = stack;
    pool->blockStackHead     = stack;
    pool->blockStackFullHead = stack;
    pool->blockStackTop      = stack + DefaultInitialCapacity;

    push_block_to_stack(ctx, pool);

    pool->allocBlock = *stack;
  }


  inline agt::message_pool_block* get_block(agt::private_sized_message_pool * pool, agt_message_t message) noexcept {
    return reinterpret_cast<agt::message_pool_block*>(pool->blockMask & reinterpret_cast<uintptr_t>(message));
  }




  void destroy_pool(agt_ctx_t ctx, agt::private_sized_message_pool* pool) noexcept {
    if (pool->blockStackBase) {
      while (pool->blockStackBase < pool->blockStackHead)
        pop_block_from_stack(ctx, pool);
    }
    std::free(pool->blockStackBase);
    instance_mem_free(get_instance(ctx), pool, sizeof(agt::private_sized_message_pool), alignof(agt::private_sized_message_pool));
  }
}


agt_status_t agt::createInstance(local_spmc_message_pool& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

agt_status_t agt::createInstance(private_sized_message_pool& poolRef,       agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

/*
agt_status_t agt::createInstance(private_sized_message_pool *&poolRef, agt_ctx_t ctx, size_t messageSize, size_t countPerBlock) noexcept {
  auto mem = instance_mem_alloc(get_instance(ctx), sizeof(private_sized_message_pool), alignof(private_sized_message_pool));
  if (!mem) [[unlikely]]
    return AGT_ERROR_BAD_ALLOC;

  auto pool = new (mem) private_sized_message_pool;

  pool->refCount = 1;
  pool->messageSize = messageSize;
  pool->slotCount = countPerBlock;
  pool->blockStackBase = nullptr;
  pool->blockStackHead = nullptr;
  pool->blockStackFullHead = nullptr;
  pool->blockStackTop = nullptr;
  pool->allocBlock = nullptr;


  const auto slotSize = messageSize + sizeof(agt_message_st);
  const auto blockSize = slotSize * countPerBlock;

  pool->blockMask  = ~(std::bit_ceil(blockSize) - 1);

  poolRef = pool;
  return AGT_SUCCESS;
}



void agt::release_message_pool(agt_ctx_t ctx, private_sized_message_pool *pool) noexcept {
  if (!--pool->refCount)
    destroy_pool(ctx, pool);
}

agt_message_t agt::acquire_message(agt_ctx_t ctx, private_sized_message_pool * pool) noexcept {
  if (!pool->allocBlock) [[unlikely]] {
    if (!pool->blockStackBase) [[unlikely]]
      init_pool(ctx, pool);
    else
      find_alloc_block(ctx, pool);
  }

  auto message = lockfree_block_alloc(pool->allocBlock);

  if (block_is_full(pool->allocBlock)) {
    swap(pool->allocBlock, *pool->blockStackFullHead);
    ++pool->blockStackFullHead;
    pool->allocBlock = nullptr;
  }

  return message;
}

void agt::release_message(agt_ctx_t ctx, private_sized_message_pool * pool, agt_message_t message) noexcept {
  auto block = get_block(pool, message);

  lockfree_block_free(block, message);


  // Checks if the block had previously been full by checking whether it is in the "full block segment" of the stack.
  if (block->stackPosition < pool->blockStackFullHead) {
    swap(block, *(pool->blockStackFullHead - 1));
    --pool->blockStackFullHead;
  }
  else if (block_is_empty(block)) [[unlikely]] {
    // If the block at the top of the stack is empty AND is distinct from this block, pop the empty block from the stack
    // and destroy it, and move this block to the top of the stack.
    auto topBlock = top_of_stack(pool);
    if (block != topBlock) {
      if (block_is_empty(topBlock))
        pop_block_from_stack(ctx, pool);
      swap(block, top_of_stack(pool));
    }

    // No matter what, block is now the only empty block in the pool, and is on top of the stack.
  }
}





using proxy_pool_t = agt::message_pool_header*;
using proxy_sized_pool_t = agt::sized_message_pool_header*;


enum message_pool_kind {
  private_message_pool,
  private_sized_message_pool,
  local_mpsc_message_pool,
  local_mpsc_sized_message_pool,
  local_spsc_message_pool,
  local_spsc_sized_message_pool,
  local_spmc_message_pool,
  local_spmc_sized_message_pool,
  local_mpmc_message_pool,
  local_mpmc_sized_message_pool,
  shared_mpsc_message_pool,
  shared_mpsc_sized_message_pool,
  shared_spsc_message_pool,
  shared_spsc_sized_message_pool,
  shared_spmc_message_pool,
  shared_spmc_sized_message_pool,
  shared_mpmc_message_pool,
  shared_mpmc_sized_message_pool
};


void          agt::retain_message_pool(message_pool_t pool) noexcept {
  ++((proxy_pool_t)pool)->refCount;
}

agt_message_t agt::acquire_message(agt_ctx_t ctx, message_pool_t pool_, size_t messageSize) noexcept {
  const auto pool = (proxy_pool_t)pool_;

  // If the pool is sized, make sure the requested messageSize fits.
  if (pool->poolType & 0x1) {
    if (((proxy_sized_pool_t)pool)->messageSize < messageSize)
      return nullptr;
  }


  switch ((message_pool_kind)pool->poolType) {
    case ::private_message_pool:
      break;
    case ::private_sized_message_pool:
      return acquire_message(ctx, (private_sized_message_pool*)pool);
    case ::local_mpsc_message_pool:
      break;
    case ::local_mpsc_sized_message_pool:
      break;
    case ::local_spsc_message_pool:
      break;
    case ::local_spsc_sized_message_pool:
      break;
    case ::local_spmc_message_pool:
      break;
    case ::local_spmc_sized_message_pool:
      break;
    case ::local_mpmc_message_pool:
      break;
    case ::local_mpmc_sized_message_pool:
      break;
    case ::shared_mpsc_message_pool:
      break;
    case ::shared_mpsc_sized_message_pool:
      break;
    case ::shared_spsc_message_pool:
      break;
    case ::shared_spsc_sized_message_pool:
      break;
    case ::shared_spmc_message_pool:
      break;
    case ::shared_spmc_sized_message_pool:
      break;
    case ::shared_mpmc_message_pool:
      break;
    case ::shared_mpmc_sized_message_pool:
      break;
  }
}

agt_message_t agt::acquire_sized_message(agt_ctx_t ctx, message_pool_t pool_) noexcept {
  const auto pool = (proxy_pool_t)pool_;
  switch ((message_pool_kind)pool->poolType) {
    case ::private_sized_message_pool:
      return acquire_message(ctx, (private_sized_message_pool*)pool);
    case ::local_mpsc_sized_message_pool:
      break;
    case ::local_spsc_sized_message_pool:
      break;
    case ::local_spmc_sized_message_pool:
      break;
    case ::local_mpmc_sized_message_pool:
      break;
    case ::shared_mpsc_sized_message_pool:
      break;
    case ::shared_spsc_sized_message_pool:
      break;
    case ::shared_spmc_sized_message_pool:
      break;
    case ::shared_mpmc_sized_message_pool:
      break;
    default:
      return nullptr;
  }
}

agt_status_t  agt::release_message(agt_ctx_t ctx, agt_message_t message) noexcept {
  if ((message->flags & (message_flags::ownedByChannel | message_flags::isShared)) != message_flags{})
    return AGT_ERROR_NOT_YET_IMPLEMENTED;

  release_message(ctx, message->pool, message);

  return AGT_SUCCESS;
}

void          agt::release_message(agt_ctx_t ctx, message_pool_t pool_, agt_message_t message) noexcept {
  const auto pool = (proxy_pool_t)pool_;
  switch ((message_pool_kind)pool->poolType) {
    case ::private_message_pool:
      break;
    case ::private_sized_message_pool:
      release_message(ctx, (private_sized_message_pool*)pool, message);
      break;
    case ::local_mpsc_message_pool:
      break;
    case ::local_mpsc_sized_message_pool:
      break;
    case ::local_spsc_message_pool:
      break;
    case ::local_spsc_sized_message_pool:
      break;
    case ::local_spmc_message_pool:
      break;
    case ::local_spmc_sized_message_pool:
      break;
    case ::local_mpmc_message_pool:
      break;
    case ::local_mpmc_sized_message_pool:
      break;
    case ::shared_mpsc_message_pool:
      break;
    case ::shared_mpsc_sized_message_pool:
      break;
    case ::shared_spsc_message_pool:
      break;
    case ::shared_spsc_sized_message_pool:
      break;
    case ::shared_spmc_message_pool:
      break;
    case ::shared_spmc_sized_message_pool:
      break;
    case ::shared_mpmc_message_pool:
      break;
    case ::shared_mpmc_sized_message_pool:
      break;
  }
}

void          agt::release_message_pool(agt_ctx_t ctx, message_pool_t pool_) noexcept {
  const auto pool = (proxy_pool_t)pool_;
  if (!--pool->refCount) {
    switch ((message_pool_kind)pool->poolType) {
      case ::private_message_pool:
        break;
      case ::private_sized_message_pool:
        destroy_pool(ctx, (private_sized_message_pool*)pool);
        break;
      case ::local_mpsc_message_pool:
        break;
      case ::local_mpsc_sized_message_pool:
        break;
      case ::local_spsc_message_pool:
        break;
      case ::local_spsc_sized_message_pool:
        break;
      case ::local_spmc_message_pool:
        break;
      case ::local_spmc_sized_message_pool:
        break;
      case ::local_mpmc_message_pool:
        break;
      case ::local_mpmc_sized_message_pool:
        break;
      case ::shared_mpsc_message_pool:
        break;
      case ::shared_mpsc_sized_message_pool:
        break;
      case ::shared_spsc_message_pool:
        break;
      case ::shared_spsc_sized_message_pool:
        break;
      case ::shared_spmc_message_pool:
        break;
      case ::shared_spmc_sized_message_pool:
        break;
      case ::shared_mpmc_message_pool:
        break;
      case ::shared_mpmc_sized_message_pool:
        break;
    }
  }
}*/