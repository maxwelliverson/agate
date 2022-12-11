//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_POOL_HPP
#define JEMSYS_AGATE2_POOL_HPP

#include <agate_old.h>

namespace Agt {
  
  template <typename T>
  inline T* allocArray(agt_size_t arraySize, agt_size_t alignment) noexcept {
#if defined(_WIN32)
    return static_cast<T*>( _aligned_malloc(arraySize * sizeof(T), std::max(alignof(T), alignment)) );
#else
    return static_cast<T*>( std::aligned_alloc(arraySize * sizeof(T), std::max(alignof(T), alignment)) );
#endif
  }
  template <typename T>
  inline void freeArray(T* array, agt_size_t arraySize, agt_size_t alignment) noexcept {
#if defined(_WIN32)
    _aligned_free(array);
#else
    free(array);
#endif
  }
  template <typename T>
  inline T* reallocArray(T* array, agt_size_t newArraySize, agt_size_t oldArraySize, agt_size_t alignment) noexcept {
#if defined(_WIN32)
    return static_cast<T*>(_aligned_realloc(array, newArraySize * sizeof(T), std::max(alignof(T), alignment)));
#else
    auto newArray = allocArray<T>(newArraySize, alignment);
    std::memcpy(newArray, array, oldArraySize * sizeof(T));
    freeArray(array, oldArraySize, alignment);
    return newArray;
#endif
  }

  class Pool {

    using Index = size_t;
    using Block = void**;

    struct Slab {
      Index  availableBlocks;
      Block  nextFreeBlock;
      Slab** stackPosition;
    };

    inline constexpr static size_t MinimumBlockSize = sizeof(Slab);
    inline constexpr static size_t InitialStackSize = 4;
    inline constexpr static size_t StackGrowthRate  = 2;
    inline constexpr static size_t StackAlignment   = AGT_CACHE_LINE;




    inline Slab* alloc_slab() const noexcept {
#if defined(_WIN32)
      auto result = static_cast<Slab*>(_aligned_malloc(slabAlignment, slabAlignment));

#else
      auto result = static_cast<Slab*>(std::aligned_alloc(slabAlignment, slabAlignment));
#endif
      std::memset(result, 0, slabAlignment);
      return result;
    }
    inline void free_slab(Slab* Slab) const noexcept {
#if defined(_WIN32)
      _aligned_free(Slab);
#else
      std::free(Slab);
#endif
    }

    inline Block lookupBlock(Slab* s, size_t blockIndex) const noexcept {
      return reinterpret_cast<Block>(reinterpret_cast<char*>(s) + (blockIndex * blockSize));
    }
    inline Slab*  lookupSlab(void* block) const noexcept {
      return reinterpret_cast<Slab*>(reinterpret_cast<uintptr_t>(block) & slabAlignmentMask);
    }

    inline size_t indexOfBlock(Slab* s, Block block) const noexcept {
      return ((std::byte*)block - (std::byte*)s) / blockSize;
    }



    inline bool isEmpty(const Slab* s) const noexcept {
      return s->availableBlocks == blocksPerSlab;
    }
    inline static bool isFull(const Slab* s) noexcept {
      return s->availableBlocks == 0;
    }


    inline void assertSlabIsValid(Slab* s) const noexcept {
      if ( s->availableBlocks == 0 )
        return;
      size_t blockCount = 1;
      std::vector<bool> blocks;
      blocks.resize(blocksPerSlab + 1);
      Block currentBlock = s->nextFreeBlock;
      while (blockCount < s->availableBlocks) {
        size_t index = indexOfBlock(s, currentBlock);
        assert( !blocks[index] );
        blocks[index] = true;
        ++blockCount;
        currentBlock = (Block)*currentBlock;
      }
    }


    inline void makeNewSlab() noexcept {

      if ( slabStackHead == slabStackTop ) [[unlikely]] {
        Slab** oldStackBase = slabStackBase;
        size_t oldStackSize = slabStackTop - oldStackBase;
        size_t newStackSize = oldStackSize * StackGrowthRate;
        Slab** newStackBase = reallocArray(slabStackBase, newStackSize, oldStackSize, StackAlignment);
        if ( slabStackBase != newStackBase ) {
          slabStackBase = newStackBase;
          slabStackTop  = newStackBase + newStackSize;
          slabStackHead = newStackBase + oldStackSize;
          fullStackOnePastTop = newStackBase + oldStackSize;

          for ( Slab** entry = newStackBase; entry != slabStackHead; ++entry )
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
        Block block = lookupBlock(*s, i);
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

    inline void pruneSlabs(Slab* parentSlab) noexcept {
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


    inline static void swapSlabs(Slab* a, Slab* b) noexcept {
      std::swap(*a->stackPosition, *b->stackPosition);
      std::swap(a->stackPosition, b->stackPosition);
    }

    inline void removeStackEntry(Slab* s) noexcept {
      const auto lastPosition = --slabStackHead;
      if ( s->stackPosition != lastPosition ) {
        (*lastPosition)->stackPosition = s->stackPosition;
        std::swap(*s->stackPosition, *lastPosition);
      }
      free_slab(*lastPosition);
    }


    size_t blockSize;
    size_t blocksPerSlab;
    Slab**  slabStackBase;
    Slab**  slabStackHead;
    Slab**  slabStackTop;
    Slab**  fullStackOnePastTop;
    Slab*   allocSlab;
    Slab*   freeSlab;
    size_t slabAlignment;
    size_t slabAlignmentMask;

  public:
    Pool(size_t blockSize, size_t blocksPerSlab) noexcept
        : blockSize(std::max(blockSize, MinimumBlockSize)),
          blocksPerSlab(blocksPerSlab - 1),
          slabStackBase(allocArray<Slab*>(InitialStackSize, StackAlignment)),
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

    ~Pool() noexcept {
      while ( slabStackHead != slabStackBase ) {
        free_slab(*slabStackHead);
        --slabStackHead;
      }

      size_t stackSize = slabStackTop - slabStackBase;
      freeArray(slabStackBase, stackSize, StackAlignment);
    }

    void* alloc_block() noexcept {

      findAllocSlab();

      Block block = allocSlab->nextFreeBlock;
      allocSlab->nextFreeBlock = static_cast<Block>(*block);
      --allocSlab->availableBlocks;

      if (freeSlab == allocSlab)
        freeSlab = nullptr;

      return block;

    }
    void  free_block(void* block_) noexcept {

      auto* parentSlab = lookupSlab(block_);
      assert( ((uintptr_t)block_ - (uintptr_t)parentSlab) % blockSize == 0);
      auto  block = static_cast<Block>(block_);

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

  template <size_t BlockSize, size_t BlocksPerSlab>
  class FixedPool {

    using Index = size_t;
    using Block = void**;

    struct Slab {
      Index  availableBlocks;
      Block  nextFreeBlock;
      Slab** stackPosition;
    };

    inline constexpr static size_t InitialStackSize  = 4;
    inline constexpr static size_t StackGrowthRate   = 2;
    inline constexpr static size_t StackAlignment    = AGT_CACHE_LINE;
    inline constexpr static size_t SlabSize          = std::bit_ceil(BlockSize * BlocksPerSlab);
    inline constexpr static size_t SlabAlignment     = SlabSize;
    inline constexpr static size_t SlabAlignmentMask = ~(SlabAlignment - 1);




    inline Slab* new_slab() const noexcept {
#if defined(_WIN32)
      auto result = static_cast<Slab*>(_aligned_malloc(SlabSize, SlabAlignment));

#else
      auto result = static_cast<Slab*>(std::aligned_alloc(SlabSize, SlabAlignment));
#endif
      std::memset(result, 0, SlabSize);
      return result;
    }
    inline void delete_slab(Slab* Slab) const noexcept {
#if defined(_WIN32)
      _aligned_free(Slab);
#else
      std::free(Slab);
#endif
    }

    inline Block lookupBlock(Slab* s, size_t blockIndex) const noexcept {
      return reinterpret_cast<Block>(reinterpret_cast<char*>(s) + (blockIndex * BlockSize));
    }
    inline Slab* lookupSlab(void* block) const noexcept {
      return reinterpret_cast<Slab*>(reinterpret_cast<uintptr_t>(block) & SlabAlignmentMask);
    }

    inline size_t indexOfBlock(Slab* s, Block block) const noexcept {
      return ((std::byte*)block - (std::byte*)s) / BlockSize;
    }



    inline bool isEmpty(const Slab* s) const noexcept {
      return s->availableBlocks == BlocksPerSlab;
    }
    inline static bool isFull(const Slab* s) noexcept {
      return s->availableBlocks == 0;
    }


    inline void assertSlabIsValid(Slab* s) const noexcept {
      if ( s->availableBlocks == 0 )
        return;
      size_t blockCount = 1;
      std::vector<bool> blocks;
      blocks.resize(BlocksPerSlab + 1);
      Block currentBlock = s->nextFreeBlock;
      while (blockCount < s->availableBlocks) {
        size_t index = indexOfBlock(s, currentBlock);
        assert( !blocks[index] );
        blocks[index] = true;
        ++blockCount;
        currentBlock = (Block)*currentBlock;
      }
    }


    inline void makeNewSlab() noexcept {

      if ( slabStackHead == slabStackTop ) [[unlikely]] {
        Slab** oldStackBase = slabStackBase;
        size_t oldStackSize = slabStackTop - oldStackBase;
        size_t newStackSize = oldStackSize * StackGrowthRate;
        Slab** newStackBase = reallocArray(slabStackBase, newStackSize, oldStackSize, StackAlignment);
        if ( slabStackBase != newStackBase ) {
          slabStackBase = newStackBase;
          slabStackTop  = newStackBase + newStackSize;
          slabStackHead = newStackBase + oldStackSize;
          fullStackOnePastTop = newStackBase + oldStackSize;

          for ( Slab** entry = newStackBase; entry != slabStackHead; ++entry )
            (*entry)->stackPosition = entry;
        }
      }

      const auto s = slabStackHead;



      *s = new_slab();
      (*s)->availableBlocks = static_cast<agt_u32_t>(BlocksPerSlab);
      (*s)->nextFreeBlock   = lookupBlock(*s, 1);
      (*s)->stackPosition = s;

      allocSlab = *s;

      uint32_t i = 1;
      while (  i < BlocksPerSlab ) {
        Block block = lookupBlock(*s, i);
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

    inline void pruneSlabs(Slab* parentSlab) noexcept {
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


    inline static void swapSlabs(Slab* a, Slab* b) noexcept {
      std::swap(*a->stackPosition, *b->stackPosition);
      std::swap(a->stackPosition, b->stackPosition);
    }

    inline void removeStackEntry(Slab* s) noexcept {
      const auto lastPosition = --slabStackHead;
      if ( s->stackPosition != lastPosition ) {
        (*lastPosition)->stackPosition = s->stackPosition;
        std::swap(*s->stackPosition, *lastPosition);
      }
      free_slab(*lastPosition);
    }

    Slab**  slabStackBase;
    Slab**  slabStackHead;
    Slab**  slabStackTop;
    Slab**  fullStackOnePastTop;
    Slab*   allocSlab;
    Slab*   freeSlab;
    size_t totalAllocations;

  public:
    FixedPool() noexcept
        : slabStackBase(allocArray<Slab*>(InitialStackSize, StackAlignment)),
          slabStackHead(slabStackBase),
          slabStackTop(slabStackBase + InitialStackSize),
          fullStackOnePastTop(slabStackHead),
          allocSlab(nullptr),
          freeSlab(nullptr),
          totalAllocations(0)
    {
      std::memset(slabStackBase, 0, sizeof(void*) * InitialStackSize);
      makeNewSlab();
    }

    ~FixedPool() noexcept {
      while ( slabStackHead != slabStackBase ) {
        free_slab(*slabStackHead);
        --slabStackHead;
      }

      size_t stackSize = slabStackTop - slabStackBase;
      freeArray(slabStackBase, stackSize, StackAlignment);
    }

    AGT_nodiscard void* alloc() noexcept {

      findAllocSlab();

      Block block = allocSlab->nextFreeBlock;
      allocSlab->nextFreeBlock = static_cast<Block>(*block);
      --allocSlab->availableBlocks;
      ++totalAllocations;

      if (freeSlab == allocSlab)
        freeSlab = nullptr;

      return block;

    }
    void  free(void* block_) noexcept {

      auto* parentSlab = lookupSlab(block_);
      assert( ((uintptr_t)block_ - (uintptr_t)parentSlab) % BlockSize == 0);
      auto  block = static_cast<Block>(block_);

      *block = parentSlab->nextFreeBlock;
      parentSlab->nextFreeBlock = block;
      ++parentSlab->availableBlocks;
      --totalAllocations;

      pruneSlabs(parentSlab);

      allocSlab = parentSlab;
    }

    AGT_forceinline size_t getAllocationCount() const noexcept {
      return totalAllocations;
    }

    AGT_nodiscard size_t blockSize() const noexcept {
      return BlockSize;
    }
    AGT_nodiscard size_t blockAlignment() const noexcept {
      return ((~BlockSize) + 1) & BlockSize;
    }
  };


  template <typename T, size_t BlocksPerSlab = 255>
  class ObjectPool {
  public:

    ObjectPool() noexcept : fixedPool() {}

    template <typename ...Args>
    AGT_forceinline T*   create(Args&& ... args) noexcept {
      return new(fixedPool.alloc()) T(std::forward<Args>(args)...);
    }

    AGT_forceinline void destroy(T* pObject) noexcept {
      if (pObject != nullptr) {
        void* memory = pObject;
        pObject->~T();
        fixedPool.free(memory);
      }
    }

    AGT_forceinline size_t activeObjectCount() const noexcept {
      return fixedPool.getAllocationCount();
    }


  private:
    FixedPool<sizeof(T), BlocksPerSlab> fixedPool;
  };

}

#endif//JEMSYS_AGATE2_POOL_HPP
