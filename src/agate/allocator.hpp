//
// Created by maxwe on 2021-09-21.
//

#ifndef JEMSYS_INTERNAL_ALLOCATOR_HPP
#define JEMSYS_INTERNAL_ALLOCATOR_HPP

#include "agate.h"

#include <utility>
#include <memory>
#include <new>
#include <vector>
#include <concepts>

// Considering scrapping this file, and doing away with templated allocator support, as it has not been used yet, and likely won't be.

namespace agt {


  class default_allocator{
  public:

    default_allocator() = default;
    default_allocator(const default_allocator&) = default;
    default_allocator(default_allocator&&) noexcept = default;

    void* allocate(size_t size, size_t align = alignof(std::max_align_t)) noexcept {
      return ::operator new(size, /*std::align_val_t{align},*/ std::nothrow);
    }
    void  deallocate(void* addr, size_t size, size_t align = alignof(std::max_align_t)) noexcept {
      ::operator delete(addr, size/*, std::align_val_t{align}*/);
    }
  };

  template <typename T>
  inline T*   alloc_array(agt_size_t arraySize, agt_size_t alignment) noexcept {
#if defined(_WIN32)
    return static_cast<T*>( _aligned_malloc(arraySize * sizeof(T), std::max(alignof(T), alignment)) );
#else
    return static_cast<T*>( std::aligned_alloc(arraySize * sizeof(T), std::max(alignof(T), alignment)) );
#endif
  }
  template <typename T>
  inline void free_array(T* array, agt_size_t arraySize, agt_size_t alignment) noexcept {
#if defined(_WIN32)
    _aligned_free(array);
#else
    free(array);
#endif
  }
  template <typename T>
  inline T*   realloc_array(T* array, agt_size_t newArraySize, agt_size_t oldArraySize, agt_size_t alignment) noexcept {
#if defined(_WIN32)
    return static_cast<T*>(_aligned_realloc(array, newArraySize * sizeof(T), std::max(alignof(T), alignment)));
#else
    auto newArray = alloc_array<T>(newArraySize, alignment);
    std::memcpy(newArray, array, oldArraySize * sizeof(T));
    free_array(array, oldArraySize, alignment);
    return newArray;
#endif
  }



  inline void* alloc_aligned(size_t size, size_t alignment) noexcept {
#if defined(_WIN32)
    return ::_aligned_malloc(size, alignment);
#else
    return std::aligned_alloc(size, alignment);
#endif
  }

  inline void  free_aligned(void* memory, size_t size, size_t alignment) noexcept {
#if defined(_WIN32)
    ::_aligned_free(memory);
#else
    free(memory);
#endif
  }

  inline void* realloc_aligned(void* memory, size_t newSize, size_t oldSize, size_t alignment) noexcept {
#if defined(_WIN32)
    return ::_aligned_realloc(memory, newSize, alignment);
#else
    auto newMemory = alloc_aligned(newSize, alignment);
    std::memcpy(newMemory, memory, (std::min)(newSize, oldSize));
    free_aligned(memory, oldSize, alignment);
    return newMemory;
#endif
  }





  class fixed_size_pool{

    using index_t = size_t;
    using block_t = void**;

    struct slab {
      index_t availableBlocks;
      block_t nextFreeBlock;
      slab**  stackPosition;
    };

    inline constexpr static size_t MinimumBlockSize = sizeof(slab);
    inline constexpr static size_t InitialStackSize = 4;
    inline constexpr static size_t StackGrowthRate  = 2;
    inline constexpr static size_t StackAlignment   = AGT_CACHE_LINE;




    inline slab* alloc_slab() const noexcept {
#if defined(_WIN32)
      auto result = static_cast<slab*>(_aligned_malloc(slabAlignment, slabAlignment));

#else
      auto result = static_cast<slab*>(std::aligned_alloc(slabAlignment, slabAlignment));
#endif
      std::memset(result, 0, slabAlignment);
      return result;
    }
    inline void free_slab(slab* slab) const noexcept {
#if defined(_WIN32)
      _aligned_free(slab);
#else
      std::free(slab);
#endif
    }

    inline block_t lookupBlock(slab* s, size_t blockIndex) const noexcept {
      return reinterpret_cast<block_t>(reinterpret_cast<char*>(s) + (blockIndex * blockSize));
    }
    inline slab*  lookupSlab(void* block) const noexcept {
      return reinterpret_cast<slab*>(reinterpret_cast<uintptr_t>(block) & slabAlignmentMask);
    }

    inline agt_size_t indexOfBlock(slab* s, block_t block) const noexcept {
      return ((std::byte*)block - (std::byte*)s) / blockSize;
    }



    inline bool isEmpty(const slab* s) const noexcept {
      return s->availableBlocks == blocksPerSlab;
    }
    inline static bool isFull(const slab* s) noexcept {
      return s->availableBlocks == 0;
    }


    inline void assertSlabIsValid(slab* s) const noexcept {
      if ( s->availableBlocks == 0 )
        return;
      agt_size_t blockCount = 1;
      std::vector<bool> blocks;
      blocks.resize(blocksPerSlab + 1);
      block_t currentBlock = s->nextFreeBlock;
      while (blockCount < s->availableBlocks) {
        agt_size_t index = indexOfBlock(s, currentBlock);
        assert( !blocks[index] );
        blocks[index] = true;
        ++blockCount;
        currentBlock = (block_t)*currentBlock;
      }
    }


    inline void makeNewSlab() noexcept {

      if ( slabStackHead == slabStackTop ) [[unlikely]] {
        slab** oldStackBase = slabStackBase;
        size_t oldStackSize = slabStackTop - oldStackBase;
        size_t newStackSize = oldStackSize * StackGrowthRate;
        slab** newStackBase = realloc_array(slabStackBase, newStackSize, oldStackSize, StackAlignment);
        if ( slabStackBase != newStackBase ) {
          slabStackBase = newStackBase;
          slabStackTop  = newStackBase + newStackSize;
          slabStackHead = newStackBase + oldStackSize;
          fullStackOnePastTop = newStackBase + oldStackSize;

          for ( slab** entry = newStackBase; entry != slabStackHead; ++entry )
            (*entry)->stackPosition = entry;
        }
      }

      const auto s = slabStackHead;



      *s = alloc_slab();
      (*s)->availableBlocks = static_cast<agt_u32_t>(blocksPerSlab);
      (*s)->nextFreeBlock   = lookupBlock(*s, 1);
      (*s)->stackPosition = s;

      allocSlab = *s;

      uint32_t i = 1;
      while (  i < blocksPerSlab ) {
        block_t block = lookupBlock(*s, i);
        *block = lookupBlock(*s, ++i);
      }

      assertSlabIsValid(*s);

      ++slabStackHead;
    }

    inline void findAllocSlab() noexcept {
      if ( isFull(allocSlab) ) {
        if (fullStackOnePastTop == slabStackHead ) {
          makeNewSlab();
        }
        else {
          if ( allocSlab != *fullStackOnePastTop)
            swapSlabs(allocSlab, *fullStackOnePastTop);
          allocSlab = *++fullStackOnePastTop;
        }
      }
    }

    inline void pruneSlabs(slab* parentSlab) noexcept {
      if ( isEmpty(parentSlab) ) {
        if ( freeSlab && isEmpty(freeSlab) ) [[unlikely]] {
          removeStackEntry(freeSlab);
        }
        freeSlab = parentSlab;
      }
      else if (parentSlab->stackPosition < fullStackOnePastTop) [[unlikely]] {
        --fullStackOnePastTop;
        if ( parentSlab->stackPosition != fullStackOnePastTop)
          swapSlabs(parentSlab, *fullStackOnePastTop);
      }
    }


    inline static void swapSlabs(slab* a, slab* b) noexcept {
      std::swap(*a->stackPosition, *b->stackPosition);
      std::swap(a->stackPosition, b->stackPosition);
    }

    inline void removeStackEntry(slab* s) noexcept {
      const auto lastPosition = --slabStackHead;
      if ( s->stackPosition != lastPosition ) {
        (*lastPosition)->stackPosition = s->stackPosition;
        std::swap(*s->stackPosition, *lastPosition);
      }
      free_slab(*lastPosition);
    }


    size_t blockSize;
    size_t blocksPerSlab;
    slab** slabStackBase;
    slab** slabStackHead;
    slab** slabStackTop;
    slab** fullStackOnePastTop;
    slab*  allocSlab;
    slab*  freeSlab;
    size_t slabAlignment;
    size_t slabAlignmentMask;

  public:
    fixed_size_pool(size_t blockSize, size_t blocksPerSlab) noexcept
        : blockSize(std::max(blockSize, MinimumBlockSize)),
          blocksPerSlab(blocksPerSlab - 1),
          slabStackBase(alloc_array<slab*>(InitialStackSize, StackAlignment)),
          slabStackHead(slabStackBase),
          slabStackTop(slabStackBase + InitialStackSize),
          fullStackOnePastTop(slabStackHead),
          allocSlab(),
          freeSlab(),
          slabAlignment(std::bit_ceil(this->blockSize * blocksPerSlab)),
          slabAlignmentMask(~(slabAlignment - 1)) {
      std::memset(slabStackBase, 0, sizeof(void*) * InitialStackSize);
      makeNewSlab();
    }

    ~fixed_size_pool() noexcept {
      while ( slabStackHead != slabStackBase ) {
        free_slab(*slabStackHead);
        --slabStackHead;
      }

      size_t stackSize = slabStackTop - slabStackBase;
      free_array(slabStackBase, stackSize, StackAlignment);
    }

    void* alloc_block() noexcept {

      findAllocSlab();

      block_t block = allocSlab->nextFreeBlock;
      allocSlab->nextFreeBlock = static_cast<block_t>(*block);
      --allocSlab->availableBlocks;

      if (freeSlab == allocSlab)
        freeSlab = nullptr;

      return block;

    }
    void  free_block(void* block_) noexcept {

      auto* parentSlab = lookupSlab(block_);
      assert( ((uintptr_t)block_ - (uintptr_t)parentSlab) % blockSize == 0);
      auto  block = static_cast<block_t>(block_);

      *block = parentSlab->nextFreeBlock;
      parentSlab->nextFreeBlock = block;
      ++parentSlab->availableBlocks;

      pruneSlabs(parentSlab);

      allocSlab = parentSlab;
    }

    AGT_nodiscard size_t block_size() const noexcept {
      return blockSize;
    }
    AGT_nodiscard size_t block_alignment() const noexcept {
      return ((~blockSize) + 1) & blockSize;
    }
  };

  template <typename Fn>
  class pool_allocator : private Fn {
  public:
    template <typename ...Args>
    pool_allocator(Args&& ...args) noexcept
        : Fn(std::forward<Args>(args)...) { }

    void* allocate(size_t size) noexcept {
      fixed_size_pool* pool = (*this)(size);
      if ( !pool ) [[unlikely]]
        return nullptr;
      return pool->alloc_block();
    }
    void* allocate(size_t size, size_t align) noexcept {
      if constexpr ( std::is_invocable_v<Fn, size_t, size_t> ) {
        fixed_size_pool* pool = (*this)(size, align);
        if ( !pool ) [[unlikely]]
          return nullptr;
        return pool->alloc_block();
      }
      else {
        fixed_size_pool* pool = (*this)(size);
        if ( !pool || pool->block_alignment() < align ) [[unlikely]]
          return nullptr;
        return pool->alloc_block();
      }

    }
    void  deallocate(void* mem, size_t size) noexcept {
      fixed_size_pool* pool = (*this)(size);
      assert(pool != nullptr);
      pool->free_block(mem);
    }
    void  deallocate(void* mem, size_t size, size_t align) noexcept {
      fixed_size_pool* pool;
      if constexpr ( std::is_invocable_v<Fn, size_t, size_t> )
        pool = (*this)(size, align);
      else
        pool = (*this)(size);
      assert(pool != nullptr);
      pool->free_block(mem);
    }
  };


}

#endif//JEMSYS_INTERNAL_ALLOCATOR_HPP
