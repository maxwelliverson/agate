//
// Created by maxwe on 2023-01-04.
//

#ifndef AGATE_CORE_IMPL_ALLOCATOR_HPP
#define AGATE_CORE_IMPL_ALLOCATOR_HPP


#include "config.hpp"
#include "../object_pool.hpp"




#include <span>

#include <unordered_map>

namespace agt {
  namespace alloc_impl {
    
    inline constexpr static size_t StaticDefaultBlockSizes[] = {
#include "static_pool_sizes.inl"
    };
    inline constexpr static size_t StaticDefaultBlocksPerChunk[] = {
#include "static_blocks_per_chunk.inl"
    };
    inline constexpr static size_t StaticDefaultPartnerSizes[][2] = {
#include "static_pool_partner_sizes.inl"
    };
    inline constexpr static size_t StaticDefaultSoloSizes[] = {
#include "static_solo_pool_sizes.inl"
    };
    

    enum {
      ALLOCATOR_IS_SINGLE_THREADED_FLAG  = 0x1,
      ALLOCATOR_IS_STATIC_FLAG           = 0x2,
      ALLOCATOR_NOT_DEFAULT_FLAG         = 0x4
    };

    
#if defined(AGT_STATIC_ALLOCATOR_SIZES)
    inline constexpr static size_t PoolStaticBlockSizeCount = std::size(StaticDefaultBlockSizes);
    // inline constexpr static size_t PoolStaticBlockPartnerSizeCount = std::size(StaticDefaultPartnerSizes);
    // inline constexpr static size_t StaticDefaultBlocksPerChunkCount = std::size(StaticDefaultBlocksPerChunk);

    inline constexpr static auto   GetPoolIndexForSize = [](size_t n){
      size_t minValue = std::numeric_limits<size_t>::max();
      size_t minValueIndex = minValue;
      for (size_t i = 0; i < PoolStaticBlockSizeCount; ++i) {
        size_t poolSize = StaticDefaultBlockSizes[i];
        if (n <= poolSize && poolSize < minValue) {
          minValue = poolSize;
          minValueIndex = i;
        }
      }
      return minValueIndex;
    };
    inline constexpr static size_t MaxAllocSize        = std::ranges::max(StaticDefaultBlockSizes);
    // inline constexpr static size_t MinAllocSize        = std::ranges::min(StaticDefaultBlockSizes);
    inline constexpr static size_t MinAllocAlignment   = []{

      // While this could currently be calculated just as easily by checking the difference between
      // the first two sizes, this allows for potentially changing to arbitrary pool sizes in the
      // future to allow for better benchmarking/optimizing.

      size_t alignment = MaxAllocSize / 2;

      for (; alignment > 1; alignment /= 2) {
        if (std::ranges::all_of(StaticDefaultBlockSizes, [alignment](size_t size){ return size % alignment == 0; }))
          return alignment;
      }

      return alignment;

      /*do {
        for ()


        alignment /= 2;
      } while(alignment > 1);

      size_t minAlignment = std::numeric_limits<size_t>::max();
      size_t prevSize = ObjectPoolSizes[0];
      for (size_t i = 1; i < ObjectPoolCount; ++i) {
        size_t currSize = ObjectPoolSizes[i];
        size_t alignment = currSize - prevSize;
        if ((currSize % alignment == 0) && (alignment < minAlignment))
          minAlignment = alignment;
        prevSize = currSize;
      }



      return minAlignment;*/
    }();

    inline constexpr static size_t NoPartnerPool       = static_cast<size_t>(-1);

    inline constexpr static size_t StaticPoolIndexArraySize = (MaxAllocSize / MinAllocAlignment) + 1;



    template <size_t Size, size_t ...Values>
    struct static_pool_indices : static_pool_indices<Size + MinAllocAlignment, Values..., GetPoolIndexForSize(Size)> { };
    template <size_t ...Values>
    struct static_pool_indices<MaxAllocSize + MinAllocAlignment, Values...> {
      // need to specify array size, as indexing into "unbounded" arrays at compile time is a nono
      inline constexpr static size_t array[StaticPoolIndexArraySize] = { Values... };
    };

    inline static constexpr size_t _get_static_pool_index(size_t size) noexcept {
      return static_pool_indices<0>::array[ size / MinAllocAlignment ];
    }

    inline static constexpr bool   _is_solo_pool_size(size_t slotSize) noexcept {
      return std::ranges::any_of(StaticDefaultSoloSizes, [slotSize](size_t size){ return size == slotSize; });
    }

    inline static constexpr agt_u64_t _make_partner_offset(size_t index, const size_t partners[2]) noexcept {
      const size_t size   = StaticDefaultBlockSizes[index];
      agt_flags32_t flags = impl::IS_DUAL_FLAG;
      size_t partnerSize = 0;
      if (partners[0] < partners[1]) {
        if (size == partners[0]) {
          flags |= impl::IS_SMALL_FLAG;
          partnerSize = partners[1];
        } else {
          flags |= impl::IS_BIG_FLAG;
          partnerSize = partners[0];
        }
      }
      else {
        if (size == partners[0]) {
          flags |= impl::IS_BIG_FLAG;
          partnerSize = partners[1];
        } else {
          flags |= impl::IS_SMALL_FLAG;
          partnerSize = partners[0];
        }
      }
      const size_t partnerIndex = _get_static_pool_index(partnerSize);
      return static_cast<agt_u64_t>(flags) | (static_cast<agt_u64_t>(static_cast<agt_i32_t>(partnerIndex) - static_cast<agt_i32_t>(index)) << 32);
    }

    inline constexpr static auto   GetPartnerPoolIndex = [](size_t i) -> agt_u64_t {
      size_t slotSize = StaticDefaultBlockSizes[i];
      if (!_is_solo_pool_size(slotSize)) {
        for (auto&& partnerSizes : StaticDefaultPartnerSizes) {
          if (slotSize == partnerSizes[0] || slotSize == partnerSizes[1])
            return _make_partner_offset(i, partnerSizes);
        }
      }
      return 0;
    };

    template <size_t I, agt_u64_t ...Indices>
    struct static_partner_pool_indices : static_partner_pool_indices<I + 1, Indices..., GetPartnerPoolIndex(I)> { };
    template <agt_u64_t ...Indices>
    struct static_partner_pool_indices<PoolStaticBlockSizeCount, Indices...> {
      inline constexpr static agt_u64_t array[PoolStaticBlockSizeCount] = {
          Indices...
      };
    };

    struct static_ctx_allocator {
      // agt_flags32_t  flags;
      impl::ctx_pool pools[PoolStaticBlockSizeCount];
    };

    AGT_noinline inline static void _init_static_ctx_allocator(static_ctx_allocator& allocator) noexcept {
      for (size_t i = 0; i < PoolStaticBlockSizeCount; ++i) {
        auto& pool = allocator.pools[i];
        impl::_init_basic_pool(pool, StaticDefaultBlockSizes[i], StaticDefaultBlocksPerChunk[i]);
        const agt_u64_t partnerPoolOffset = static_partner_pool_indices<0>::array[i];
        std::memcpy(&pool.flags, &partnerPoolOffset, sizeof partnerPoolOffset);
      }
    }

    AGT_noinline inline static void _destroy_static_ctx_allocator(static_ctx_allocator& allocator) noexcept {
      std::ranges::for_each(allocator.pools, static_cast<void(*)(impl::ctx_pool&)>(impl::_destroy_pool));
    }

    AGT_forceinline inline static size_t    ctx_allocator_lut_required_size(agt_instance_t, const agt_allocator_params_t* params) noexcept {
      return 0;
    }

#if defined(AGT_COMPILE_SINGLE_THREADED)
    using instance_default_allocator_params = static_ctx_allocator*;
    using ctx_allocator        = static_ctx_allocator*;

    using get_default_allocator_params_return_type = instance_default_allocator_params;


    AGT_forceinline inline static impl::ctx_pool* get_ctx_pool_unchecked(ctx_allocator alloc, size_t size) noexcept {
      return &alloc->pools[ size / MinAllocAlignment ];
    }

    AGT_forceinline inline static size_t get_max_alloc_size(ctx_allocator) noexcept { return MaxAllocSize; }

    AGT_forceinline inline static size_t get_max_chunk_size(ctx_allocator) noexcept { return (0x1 << 16) * impl::PoolBasicUnitSize; }

    AGT_forceinline inline static get_default_allocator_params_return_type get_default_allocator_params(agt_instance_t instance) noexcept;

    AGT_forceinline inline static agt_status_t init_default_allocator_params(instance_default_allocator_params& alloc, const agt_allocator_params_t* params) noexcept {
      if ( params )
        return AGT_ERROR_IDK;
      auto allocator = (static_ctx_allocator*)std::malloc(sizeof(static_ctx_allocator));
      std::memset(allocator, 0, sizeof(static_ctx_allocator));
      _init_static_ctx_allocator(*allocator);
      // allocator->flags = ALLOCATOR_IS_SINGLE_THREADED_FLAG | ALLOCATOR_IS_STATIC_FLAG;
      alloc = allocator;
      return AGT_SUCCESS;
    }

    AGT_forceinline static agt_status_t init_ctx_allocator(agt_instance_t instance, ctx_allocator& alloc, const agt_allocator_params_t* params) noexcept {
      if (params != nullptr)
        return AGT_ERROR_IDK;
      alloc = get_default_allocator_params(instance);
    }

    AGT_forceinline static void destroy_ctx_allocator(agt_instance_t, ctx_allocator) noexcept { }

    AGT_noinline inline static void destroy_default_allocator_params(agt_instance_t, instance_default_allocator_params& params) noexcept {
      for (auto&& pool : params->pools)
        impl::_destroy_pool(pool);
      std::free(params);
      params = nullptr;
    }

#else

#define AGT_NO_DEFAULT_ALLOCATOR_PARAMS

#define init_default_allocator_params(...)
#define destroy_default_allocator_params(...)

    using ctx_allocator        = static_ctx_allocator;

    AGT_forceinline static agt_status_t init_ctx_allocator(agt_instance_t, ctx_allocator& alloc, const agt_allocator_params_t* params) noexcept {
      _init_static_ctx_allocator(alloc);
      // alloc.flags = ALLOCATOR_IS_STATIC_FLAG;
      return params == nullptr ? AGT_SUCCESS : AGT_ERROR_IDK;
    }

    AGT_noinline    static void destroy_ctx_allocator(agt_instance_t, ctx_allocator& allocator) noexcept {
      for (auto&& pool : allocator.pools)
        impl::_destroy_pool(pool);
    }


    AGT_forceinline inline static impl::ctx_pool* get_ctx_pool_unchecked(ctx_allocator& alloc, size_t size) noexcept {
      return &alloc.pools[ size / MinAllocAlignment ];
    }

    AGT_forceinline inline static size_t get_max_alloc_size(const ctx_allocator&) noexcept { return MaxAllocSize; }

    AGT_forceinline inline static size_t get_max_chunk_size(const ctx_allocator&) noexcept { return (0x1 << 16) * impl::PoolBasicUnitSize; }

#endif
#else

    struct ctx_allocator {
      agt_flags32_t   flags;
      impl::ctx_pool* pPools;
      agt_u32_t       poolCount;
      agt_u32_t       maxAllocSize;
      agt_u32_t       maxChunkSize;
      agt_u32_t       poolLookupTableSize;
      impl::ctx_pool* poolLookupTable[];
    };


    inline constexpr static size_t CtxAllocatorGranularity = 8;


    inline static size_t _get_index_for_size(size_t size, std::span<const size_t> blockSizes) noexcept {
      size_t minBlockSize = static_cast<size_t>(-1);
      size_t minBlockIndex = minBlockSize;

      for (size_t i = 0; i < blockSizes.size(); ++i) {
        size_t blockSize = blockSizes[i];
        if (size <= blockSize && blockSize < minBlockSize) {
          minBlockSize = blockSize;
          minBlockIndex = i;
        }
      }

      return minBlockIndex;
    }

    inline static void   _make_index_map(std::span<size_t> indexTable, std::span<const size_t> blockSizes) noexcept {
      size_t size = 0;
      for (size_t& index : indexTable) {
        index = _get_index_for_size(size, blockSizes);
        size += CtxAllocatorGranularity;
      }
    }


    inline static size_t _lookup_index(size_t size, std::span<const size_t> indexTable) noexcept {
      return indexTable[size / impl::PoolBasicUnitSize];
    }

    inline static bool   _contains(std::span<const size_t> rng, size_t value) noexcept {
      return std::ranges::any_of(rng, [value](size_t el){ return el == value; });
    }


    inline static agt_status_t _init_allocator_from_params(ctx_allocator& allocator, const agt_allocator_params_t* pParams) noexcept {
      const auto& params = *pParams;

      std::span<const size_t>    blockSizes{ params.blockSizes, params.blockSizeCount };
      std::span<const size_t>    blocksPerChunk{ params.blocksPerChunk, params.blockSizeCount };
      std::span<const size_t[2]> partnerSizes{ params.partneredBlockSizes, params.partneredBlockSizeCount };
      std::span<const size_t>    soloSizes{ params.soloBlockSizes, params.soloBlockSizeCount };

      const auto poolTable = (impl::ctx_pool*)std::calloc(params.blockSizeCount, sizeof(impl::ctx_pool));

      const size_t maxAllocSize     = std::ranges::max(blockSizes);
      const size_t indexTableLength = (maxAllocSize / impl::PoolBasicUnitSize) + 1;

      const std::span<size_t> indexTable{ (size_t*)allocator.poolLookupTable, indexTableLength };

      _make_index_map(indexTable, blockSizes);

      // Bit of a roundabout way of accomplishing this but whatever, it's sound, and this isn't along a performance critical codepath
      for (size_t& index : indexTable) {
        index *= sizeof(void*);
        index += reinterpret_cast<uintptr_t>(allocator.pPools);
      }

      for (size_t i = 0; i < params.blockSizeCount; ++i) {

        const size_t blockSize = blockSizes[i];

        auto& pool = poolTable[i];

        impl::_init_basic_pool(pool, blockSize, blocksPerChunk[i]);

        auto partnerIndex = static_cast<size_t>(-1);
        bool isBig = false;

        if (!_contains(soloSizes, blockSize)) {
          for (auto&& partnerSize : partnerSizes) {
            if (blockSize == partnerSize[0]) {
              partnerIndex = _lookup_index(partnerSize[1], indexTable);
              break;
            }
            else if (blockSize == partnerSize[1]) {
              partnerIndex = _lookup_index(partnerSize[0], indexTable);
              isBig = true;
              break;
            }
          }
        }

        if (partnerIndex != static_cast<size_t>(-1)) {
          pool.partnerPoolOffset = static_cast<agt_i32_t>(partnerIndex) - static_cast<agt_i32_t>(i);
          pool.flags = impl::IS_DUAL_FLAG | (isBig ? impl::IS_BIG_FLAG : impl::IS_SMALL_FLAG);
        }
        else {
          pool.partnerPoolOffset = 0;
          pool.flags = 0;
        }
      }

      allocator.pPools = poolTable;
      allocator.poolLookupTableSize = indexTableLength;
      allocator.poolCount    = blockSizes.size();
      allocator.maxChunkSize = params.maxChunkSize;
      allocator.maxAllocSize = maxAllocSize;

      return AGT_SUCCESS;
    }

    AGT_forceinline inline static size_t _calculate_lut_required_size(const agt_allocator_params_t& params) noexcept {
      size_t maxSize = std::ranges::max(std::span{ params.blockSizes, params.blockSizeCount });
      return ((maxSize / impl::PoolBasicUnitSize) + 1) * sizeof(void*);
    }

    AGT_noinline inline static void      destroy_ctx_allocator(agt_instance_t instance, ctx_allocator& allocator) noexcept {
      std::for_each_n(allocator.pPools, allocator.poolCount, impl::_destroy_pool);
      std::free(allocator.pPools);
    }

    AGT_forceinline inline static impl::ctx_pool* get_ctx_pool_unchecked(ctx_allocator& alloc, size_t size) noexcept {
      return alloc.poolLookupTable[ size / CtxAllocatorGranularity ];
    }

    AGT_forceinline inline static size_t get_max_alloc_size(const ctx_allocator& allocator) noexcept { return allocator.maxAllocSize; }

    AGT_forceinline inline static size_t get_max_chunk_size(const ctx_allocator& allocator) noexcept { return allocator.maxChunkSize; }


#if defined(AGT_COMPILE_SINGLE_THREADED)

    using instance_default_allocator_params = ctx_allocator*;

    using get_default_allocator_params_return_type = instance_default_allocator_params;

    AGT_forceinline inline static get_default_allocator_params_return_type get_default_allocator_params(agt_instance_t instance) noexcept;

    AGT_noinline inline static agt_status_t init_default_allocator_params(instance_default_allocator_params& alloc, const agt_allocator_params_t* pParams) noexcept {
      agt_allocator_params_t defaultParamStruct;
      if (!pParams) {
        defaultParamStruct.blockSizes = StaticDefaultBlockSizes;
        defaultParamStruct.blockSizeCount = std::size(StaticDefaultBlockSizes);
        defaultParamStruct.blocksPerChunk = StaticDefaultBlocksPerChunk;
        defaultParamStruct.partneredBlockSizes = StaticDefaultPartnerSizes;
        defaultParamStruct.partneredBlockSizeCount = std::size(StaticDefaultPartnerSizes);
        defaultParamStruct.soloBlockSizes = StaticDefaultSoloSizes;
        defaultParamStruct.soloBlockSizeCount = std::size(StaticDefaultSoloSizes);
        defaultParamStruct.maxChunkSize = (0x1 << 16) * impl::PoolBasicUnitSize;
        pParams = &defaultParamStruct;
      }

      const auto& params = *pParams;

      size_t lutRequiredSize = _calculate_lut_required_size(params);

      auto allocator = (ctx_allocator*)std::malloc(sizeof(ctx_allocator) + lutRequiredSize);

      std::span<const size_t>    blockSizes{ params.blockSizes, params.blockSizeCount };
      std::span<const size_t>    blocksPerChunk{ params.blocksPerChunk, params.blockSizeCount };
      std::span<const size_t[2]> partnerSizes{ params.partneredBlockSizes, params.partneredBlockSizeCount };
      std::span<const size_t>    soloSizes{ params.soloBlockSizes, params.soloBlockSizeCount };

      const auto poolTable = (impl::ctx_pool*)std::calloc(params.blockSizeCount, sizeof(impl::ctx_pool));

      const size_t maxAllocSize     = std::ranges::max(blockSizes);
      const size_t indexTableLength = (maxAllocSize / impl::PoolBasicUnitSize) + 1;

      const std::span<size_t> indexTable{ (size_t*)allocator->poolLookupTable, indexTableLength };

      _make_index_map(indexTable, blockSizes);

      // Bit of a roundabout way of accomplishing this but whatever, it's sound, and this isn't along a performance critical codepath
      for (size_t& index : indexTable) {
        index *= sizeof(void*);
        index += reinterpret_cast<uintptr_t>(allocator->pPools);
      }

      for (size_t i = 0; i < params.blockSizeCount; ++i) {

        const size_t blockSize = blockSizes[i];

        auto& pool = poolTable[i];

        impl::_init_basic_pool(pool, blockSize, blocksPerChunk[i]);

        auto partnerIndex = static_cast<size_t>(-1);
        bool isBig = false;

        if (!_contains(soloSizes, blockSize)) {
          for (auto&& partnerSize : partnerSizes) {
            if (blockSize == partnerSize[0]) {
              partnerIndex = _lookup_index(partnerSize[1], indexTable);
              break;
            }
            else if (blockSize == partnerSize[1]) {
              partnerIndex = _lookup_index(partnerSize[0], indexTable);
              isBig = true;
              break;
            }
          }
        }

        if (partnerIndex != static_cast<size_t>(-1)) {
          pool.partnerPoolOffset = static_cast<agt_i32_t>(partnerIndex) - static_cast<agt_i32_t>(i);
          pool.flags = impl::IS_DUAL_FLAG | (isBig ? impl::IS_BIG_FLAG : impl::IS_SMALL_FLAG);
        }
        else {
          pool.partnerPoolOffset = 0;
          pool.flags = 0;
        }
      }

      allocator->pPools = poolTable;
      allocator->poolLookupTableSize = indexTableLength;
      allocator->poolCount    = blockSizes.size();
      allocator->maxChunkSize = params.maxChunkSize;
      allocator->maxAllocSize = maxAllocSize;

      alloc = allocator;

      return AGT_SUCCESS;
    }

    AGT_forceinline inline static void _init_default_ctx_allocator(ctx_allocator& allocator, instance_default_allocator_params params) noexcept {
      const size_t totalSize = sizeof(ctx_allocator) + (params->poolLookupTableSize * sizeof(void*));
      std::memcpy(&allocator, params, totalSize);
      allocator.pPools = nullptr; // Does not own the pools, and as such may not free the table. Pools can still be accessed via index table
    }

    AGT_noinline inline static void destroy_default_allocator_params(agt_instance_t instance, instance_default_allocator_params& params) noexcept {
      AGT_assert( params != nullptr );
      destroy_ctx_allocator(instance, *params);
      std::free( params );
      params = nullptr;
    }
#else

    struct instance_default_pool_params {
      agt_u32_t blockSize;       //
      agt_u32_t blocksPerChunk;  //
      agt_u32_t flags;           // Either be 0 or some combination of impl::IS_DUAL_FLAG and either impl::IS_SMALL_FLAG or impl::IS_BIG_FLAG
      agt_i32_t partnerOffset;   // 0 == no partner
    };

    struct instance_default_allocator_params {
      const instance_default_pool_params* pParams;
      const size_t* pPoolIndices;
      agt_u32_t paramCount;
      agt_u32_t poolLookupTableSize;
      agt_u32_t maxAllocSize;
      agt_u32_t maxChunkSize;
    };

    using get_default_allocator_params_return_type = const instance_default_allocator_params&;

    AGT_forceinline inline static get_default_allocator_params_return_type get_default_allocator_params(agt_instance_t instance) noexcept;

    inline static void _init_default_ctx_allocator(ctx_allocator& allocator, const instance_default_allocator_params& params) noexcept {
      const auto poolTable = (impl::ctx_pool*)std::calloc(params.paramCount, sizeof(impl::ctx_pool));

      allocator.maxAllocSize        = params.maxAllocSize;
      allocator.maxChunkSize        = params.maxChunkSize;
      allocator.poolCount           = params.paramCount;
      allocator.poolLookupTableSize = params.poolLookupTableSize;
      allocator.pPools              = poolTable;
      allocator.flags = 0;

      for (size_t i = 0; i < params.paramCount; ++i) {
        auto& pool = poolTable[i];
        auto  poolParams = params.pParams[i];
        impl::_init_basic_pool(pool, poolParams.blockSize, poolParams.blocksPerChunk);
        pool.flags = poolParams.flags;
        pool.partnerPoolOffset = poolParams.partnerOffset;
      }

      for (size_t i = 0; i < params.poolLookupTableSize; ++i)
        allocator.poolLookupTable[i] = poolTable + params.pPoolIndices[i];
    }

    AGT_noinline inline static agt_status_t init_default_allocator_params(instance_default_allocator_params& alloc, const agt_allocator_params_t* params) noexcept {
      std::span<const size_t>    blockSizes;
      std::span<const size_t>    blocksPerChunk;
      std::span<const size_t[2]> partnerSizes;
      std::span<const size_t>    soloSizes;
      size_t                     maxChunkSize;

      if (params) {
        blockSizes     = { params->blockSizes,          params->blockSizeCount };
        blocksPerChunk = { params->blocksPerChunk,      params->blockSizeCount };
        partnerSizes   = { params->partneredBlockSizes, params->partneredBlockSizeCount };
        soloSizes      = { params->soloBlockSizes,      params->soloBlockSizeCount };
        maxChunkSize   = params->maxChunkSize;
      }
      else {
        static_assert(std::size(StaticDefaultBlockSizes) == std::size(StaticDefaultBlocksPerChunk), "core/impl/static_blocks_per_chunk.inl must contain the same number of entries as core/impl/static_pool_sizes.inl");

        blockSizes     = StaticDefaultBlockSizes;
        blocksPerChunk = StaticDefaultBlocksPerChunk;
        partnerSizes   = StaticDefaultPartnerSizes;
        soloSizes      = StaticDefaultSoloSizes;
        maxChunkSize   = impl::PoolBasicUnitSize * (0x1 << 16);
      }

      if (blockSizes.size() != blocksPerChunk.size())
        return AGT_ERROR_INVALID_ATTRIBUTE_VALUE;

      const size_t maxAllocSize   = std::ranges::max(blockSizes);
      const size_t indexTableLength = (maxAllocSize / impl::PoolBasicUnitSize) + 1;

      auto pIndexTable = (size_t*)std::malloc(indexTableLength * sizeof(size_t));

      std::span indexTable{ pIndexTable, indexTableLength };

      _make_index_map(indexTable, blockSizes);

      auto paramCache = (instance_default_pool_params*)std::calloc(blockSizes.size(), sizeof(instance_default_pool_params));

      for (size_t i = 0; i < blockSizes.size(); ++i) {

        auto& cache = paramCache[i];

        const size_t blockSize = blockSizes[i];

        cache.blockSize      = static_cast<agt_u32_t>(blockSize);
        cache.blocksPerChunk = static_cast<agt_u32_t>(blocksPerChunk[i]);

        auto partnerIndex = static_cast<size_t>(-1);
        bool isBig = false;

        if (!_contains(soloSizes, blockSize)) {
          for (auto&& partnerSize : partnerSizes) {
            if (blockSize == partnerSize[0]) {
              partnerIndex = _lookup_index(partnerSize[1], indexTable);
              break;
            }
            else if (blockSize == partnerSize[1]) {
              partnerIndex = _lookup_index(partnerSize[0], indexTable);
              isBig = true;
              break;
            }
          }
        }

        if (partnerIndex != static_cast<size_t>(-1)) {
          cache.flags = impl::IS_DUAL_FLAG | (isBig ? impl::IS_BIG_FLAG : impl::IS_SMALL_FLAG);
          cache.partnerOffset = static_cast<agt_i32_t>(partnerIndex) - static_cast<agt_i32_t>(i);
        }
        else {
          cache.flags = 0;
          cache.partnerOffset = 0;
        }
      }

      alloc.pParams         = paramCache;
      alloc.paramCount      = blockSizes.size();
      alloc.maxAllocSize    = static_cast<agt_u32_t>(maxAllocSize);
      alloc.maxChunkSize    = static_cast<agt_u32_t>(maxChunkSize);
      alloc.poolLookupTableSize = static_cast<agt_u32_t>(indexTableLength);
      alloc.pPoolIndices    = pIndexTable;

      return AGT_SUCCESS;
    }

    AGT_noinline inline static void destroy_default_allocator_params(agt_instance_t instance, instance_default_allocator_params& params) noexcept {
      std::free((void*)params.pParams);
      std::free((void*)params.pPoolIndices);
      std::memset(&params, 0, sizeof(instance_default_allocator_params));
    }

#endif



    template <typename T> requires (!std::is_pointer_v<T>)
    AGT_forceinline inline static T& _deref(T& ref) noexcept {
      return ref;
    }
    template <typename T> requires (!std::is_pointer_v<T>)
    AGT_forceinline inline static const T& _deref(const T& ref) noexcept {
      return ref;
    }
    template <typename T>
    AGT_forceinline inline static T& _deref(T* ptr) noexcept {
      return *ptr;
    }
    template <typename T>
    AGT_forceinline inline static const T& _deref(const T* ptr) noexcept {
      return *ptr;
    }



    AGT_noinline inline static agt_status_t init_ctx_allocator(agt_instance_t instance, ctx_allocator& allocator, const agt_allocator_params_t* pParams) noexcept {
      agt_status_t status = AGT_SUCCESS;
      if (pParams) {
        status = _init_allocator_from_params(allocator, pParams);
        allocator.flags = ALLOCATOR_NOT_DEFAULT_FLAG;
      }
      else {
        _init_default_ctx_allocator(allocator, get_default_allocator_params(instance));
        allocator.flags = 0;
      }

#if defined(AGT_COMPILE_SINGLE_THREADED)
      allocator.flags |= ALLOCATOR_IS_SINGLE_THREADED_FLAG;
#endif
      return status;
    }

    AGT_forceinline inline static size_t    ctx_allocator_lut_required_size(agt_instance_t instance, const agt_allocator_params_t* params) noexcept {
      if (params)
        return _calculate_lut_required_size(*params);
      return _deref(get_default_allocator_params(instance)).poolLookupTableSize * sizeof(void*);
    }

#endif





#if !defined(AGT_NO_DEFAULT_ALLOCATOR_PARAMS)
#define AGT_declare_default_allocator_params(name) agt::alloc_impl::instance_default_allocator_params name
#define AGT_DEFINE_GET_DEFAULT_ALLOCATOR_PARAMS_FUNC(fieldName) AGT_forceinline agt::alloc_impl::get_default_allocator_params_return_type agt::alloc_impl::get_default_allocator_params(agt_instance_t instance) noexcept { return instance->fieldName; }
#else
#define AGT_declare_default_allocator_params(name)
#define AGT_DEFINE_GET_DEFAULT_ALLOCATOR_PARAMS_FUNC(fieldName)
#endif
  }




  // PUBLIC INTERFACES:

    // agt_status_t    init_ctx_allocator(agt_instance_t, ctx_allocator&, const agt_allocator_params_t*);

    // void            destroy_ctx_allocator(agt_instance_t, ctx_allocator&);

    // agt_status_t    init_default_allocator_params(instance_default_allocator_params&, const agt_allocator_params_t*);

    // void            destroy_default_allocator_params(agt_instance_t, instance_default_allocator_params&) noexcept;

    // size_t          ctx_allocator_lut_required_size(agt_instance_t, const agt_allocator_params_t*) noexcept;

    // impl::ctx_pool* get_ctx_pool_unchecked(ctx_allocator&, size_t) noexcept;

    // size_t          get_max_alloc_size(const ctx_allocator& allocator) noexcept;

    // size_t          get_max_chunk_size(const ctx_allocator& allocator) noexcept;

}

#endif//AGATE_CORE_IMPL_ALLOCATOR_HPP
