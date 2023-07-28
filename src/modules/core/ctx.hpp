//
// Created by maxwe on 2022-12-08.
//

#ifndef AGATE_CONTEXT_THREAD_CONTEXT_HPP
#define AGATE_CONTEXT_THREAD_CONTEXT_HPP

#include "config.hpp"

#include "agate/align.hpp"
#include "error.hpp"
#include "instance.hpp"



#include <bit>
#include <type_traits>
#include <concepts>
#include <array>
#include <algorithm>

namespace agt {

  struct ctx_vtable {
    agt_status_t (* create)(agt_ctx_t* pResult, agt_instance_t instance, agt_executor_t executor, const agt_allocator_params_t* allocParams);
    void         (* destroy)(agt_ctx_t ctx);
  };

  /*struct ctx_params {
    const agt_allocator_params_t* customAllocatorParams;

  };

  namespace alloc_impl {
    struct dual_sizes{
      size_t small;
      size_t large;
    };


#if !defined(AGT_STATIC_ALLOCATOR_SIZES)


    inline constexpr static size_t PoolStaticBlockSizes[] = {
#include "modules/core/impl/static_pool_sizes.inl"
    };
    inline constexpr static size_t PoolStaticBlockPartnerSizes[][2] = {
#include "modules/core/impl/static_pool_partner_sizes.inl"
    };
    inline constexpr static size_t PoolStaticBlocksPerChunk[] = {
#include "modules/core/impl/static_blocks_per_chunk.inl"
    };
    inline constexpr static size_t PoolStaticSoloBlockSizes[] = {
#include "modules/core/impl/static_solo_pool_sizes.inl"
    };

    inline constexpr static size_t PoolStaticBlockSizeCount = std::size(PoolStaticBlockSizes);
    // inline constexpr static size_t PoolStaticBlockPartnerSizeCount = std::size(PoolStaticBlockPartnerSizes);
    // inline constexpr static size_t PoolStaticBlocksPerChunkCount = std::size(PoolStaticBlocksPerChunk);

    inline constexpr static auto   GetPoolIndexForSize = [](size_t n){
      size_t minValue = std::numeric_limits<size_t>::max();
      size_t minValueIndex = minValue;
      for (size_t i = 0; i < PoolStaticBlockSizeCount; ++i) {
        size_t poolSize = PoolStaticBlockSizes[i];
        if (n <= poolSize && poolSize < minValue) {
          minValue = poolSize;
          minValueIndex = i;
        }
      }
      return minValueIndex;
    };
    inline constexpr static size_t MaxAllocSize        = std::ranges::max(PoolStaticBlockSizes);
    // inline constexpr static size_t MinAllocSize        = std::ranges::min(PoolStaticBlockSizes);
    inline constexpr static size_t MinAllocAlignment   = []{

      // While this could currently be calculated just as easily by checking the difference between
      // the first two sizes, this allows for potentially changing to arbitrary pool sizes in the
      // future to allow for better benchmarking/optimizing.

      size_t alignment = MaxAllocSize / 2;

      for (; alignment > 1; alignment /= 2) {
        if (std::ranges::all_of(PoolStaticBlockSizes, [alignment](size_t size){ return size % alignment == 0; }))
          return alignment;
      }

      return alignment;


    }();

    inline constexpr static size_t NoPartnerPool       = static_cast<size_t>(-1);

    inline constexpr static size_t StaticPoolIndexArraySize = (MaxAllocSize / MinAllocAlignment) + 1;

    struct static_ctx_allocator {
      impl::ctx_pool pools[PoolStaticBlockSizeCount];
    };

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
      return std::ranges::any_of(PoolStaticSoloBlockSizes, [slotSize](size_t size){ return size == slotSize; });
    }

    inline constexpr static auto   GetPartnerPoolIndex = [](size_t i) {
      size_t slotSize = PoolStaticBlockSizes[i];
      size_t partnerPoolIndex = NoPartnerPool;
      if (!_is_solo_pool_size(slotSize)) {
        for (auto&& partnerSizes : PoolStaticBlockPartnerSizes) {
          if (partnerSizes[0] == slotSize) {
            partnerPoolIndex = _get_static_pool_index(partnerSizes[1]);
            break;
          }
          else if (partnerSizes[1] == slotSize) {
            partnerPoolIndex = _get_static_pool_index(partnerSizes[0]);
            break;
          }
        }
      }
      return partnerPoolIndex;
    };

    template <size_t I, size_t ...Indices>
    struct static_partner_pool_indices : static_partner_pool_indices<I + 1, Indices..., GetPartnerPoolIndex(I)> { };
    template <size_t ...Indices>
    struct static_partner_pool_indices<PoolStaticBlockSizeCount, Indices...> {
      inline constexpr static size_t array[PoolStaticBlockSizeCount] = {
          Indices...
      };
    };


    inline static impl::ctx_pool* _get_partner_pool(static_ctx_allocator& allocator, size_t i) noexcept {
      const size_t index = static_partner_pool_indices<0>::array[i];
      return index == NoPartnerPool ? nullptr : &allocator.pools[index];
    }

    inline static void _init_static_ctx_allocator(static_ctx_allocator& allocator) noexcept {
      for (size_t i = 0; i < PoolStaticBlockSizeCount; ++i) {
        impl::_init_ctx_pool(
            allocator.pools[i],
            PoolStaticBlockSizes[i],
            PoolStaticBlocksPerChunk[i],
            _get_partner_pool(allocator, i));
      }
    }

#else

#endif

    inline constexpr static size_t     SoloChunkAllocationSizes[] = {
        16,
        32,
        64,
        128,
        256
    };
    inline constexpr static dual_sizes DualChunkAllocationSizes[] = {
        { 24,  40  },
        { 56,  72  },
        { 48,  80  },
        { 40,  88  },
        { 32,  96  },
        { 112, 144 },
        { 96,  160 },
        { 64,  192 },
        { 32,  224 }
    };
    inline constexpr static size_t     ObjectPoolSizes[] = {
        16,
        24,
        32,
        40,
        48,
        56,
        64,
        72,
        80,
        88,
        96,
        112,
        128,
        144,
        160,
        192,
        224,
        256
    };
    inline constexpr static size_t SoloChunkPoolsCount = std::size(SoloChunkAllocationSizes);
    inline constexpr static size_t DualChunkPoolsCount = std::size(DualChunkAllocationSizes);
    inline constexpr static size_t ObjectPoolCount     = std::size(ObjectPoolSizes);


    struct object_allocator {
      // impl::solo_pool_chunk_allocator soloChunkAllocators[SoloChunkAllocatorCount];
      // impl::dual_pool_chunk_allocator dualChunkAllocators[DualChunkAllocatorCount];
      object_pool objectPools[ObjectPoolCount];
    };


    template <size_t N>
    struct find_pool_index_for {
      inline constexpr static size_t value = []{
        const size_t* poolBegin = std::begin(ObjectPoolSizes);
        const size_t* poolEnd   = std::end(ObjectPoolSizes);
        return std::find(poolBegin, poolEnd, N) - poolBegin;
      }();
    };

    template <dual_sizes Size>
    struct find_pool_indices_for {
      inline constexpr static dual_sizes value = []{
        return dual_sizes{ find_pool_index_for<Size.small>::value, find_pool_index_for<Size.large>::value };
      }();
    };

    template <size_t PoolSize>
    struct get_chunk_allocator_index {
      inline constexpr static size_t value = []{
        size_t i = 0;
        for (; i < SoloChunkAllocatorCount; ++i)
          if (SoloChunkAllocatorSizes[i] == PoolSize)
            return i;
        for (size_t j = 0; j < DualChunkAllocatorCount; ++i, ++j) {
          if (DualChunkAllocatorSizes[j].small == PoolSize)
            return i;
          if (DualChunkAllocatorSizes[j].large == PoolSize)
            return i;
        }
        return static_cast<size_t>(-1);
      }();
    };

    template <auto ...SplitArray>
    struct metavalue_list{};

    template <const auto& Array, size_t ...I>
    consteval static auto make_metavalue_list_fn(std::index_sequence<I...>) -> metavalue_list<Array[I]...>;

    template <const auto& Array>
    consteval static auto make_metavalue_list_fn() -> decltype(make_metavalue_list_fn<Array>(std::make_index_sequence<std::size(Array)>{}));

    template <const auto& Array>
    using make_metavalue_list = decltype(make_metavalue_list_fn<Array>());

    template <typename MetaList>
    struct make_array_from_metavalue_list;
    template <typename T, T ...Values>
    struct make_array_from_metavalue_list<metavalue_list<Values...>>{
      inline constexpr static T value[sizeof...(Values)] = {
          Values...
      };
    };

    template <typename MetaList, template <auto> typename Fn>
    struct map_list;

    template <typename T, T ...MetaListValues, template <T> typename Fn>
    struct map_list<metavalue_list<MetaListValues...>, Fn>{
      using type = metavalue_list<Fn<MetaListValues>::value...>;
    };

    template <const auto& Array, template <auto> typename Fn>
    using index_array_t = make_array_from_metavalue_list<typename map_list<make_metavalue_list<Array>, Fn>::type>;



    inline constexpr static auto   GetPoolIndexForSize = [](size_t n){
      size_t minValue = std::numeric_limits<size_t>::max();
      size_t minValueIndex = minValue;
      for (size_t i = 0; i < ObjectPoolCount; ++i) {
        size_t poolSize = ObjectPoolSizes[i];
        if (n <= poolSize && poolSize < minValue) {
          minValue = poolSize;
          minValueIndex = i;
        }
      }
      return minValueIndex;
    };

    inline constexpr static size_t MaxAllocSize            = std::ranges::max(ObjectPoolSizes);
    inline constexpr static size_t MinAllocSize            = std::ranges::min(ObjectPoolSizes);
    inline constexpr static size_t MinAllocAlignment       = []{

      // While this could currently be calculated just as easily by checking the difference between
      // the first two sizes, this allows for potentially changing to arbitrary pool sizes in the
      // future to allow for better benchmarking/optimizing.

      size_t alignment = MaxAllocSize / 2;

      for (; alignment > 1; alignment /= 2) {
        if (std::ranges::all_of(ObjectPoolSizes, [alignment](size_t size){ return size % alignment == 0; }))
          return alignment;
      }

      return alignment;


    }();



    template <size_t N, size_t ...Values>
    struct pool_index_lut_generator : pool_index_lut_generator<N + MinAllocAlignment, Values..., GetPoolIndexForSize(N)> { };
    template <size_t ...Values>
    struct pool_index_lut_generator<MaxAllocSize + MinAllocAlignment, Values...> {
      inline constexpr static size_t value[] = {
          Values...
      };
    };

    inline constexpr static auto& PoolIndexLookupTable = pool_index_lut_generator<0>::value;
    inline constexpr static auto& SoloPoolIndexArray = index_array_t<SoloChunkAllocatorSizes, find_pool_index_for>::value;
    inline constexpr static auto& DualPoolIndicesArray = index_array_t<DualChunkAllocatorSizes, find_pool_indices_for>::value;
    inline constexpr static auto& ChunkAllocatorIndexArray = index_array_t<ObjectPoolSizes, get_chunk_allocator_index>::value;


    AGT_forceinline constexpr static size_t get_pool_index(size_t size) noexcept {
      return PoolIndexLookupTable[size / MinAllocAlignment];
    }

    AGT_noinline void reportBadSize(object_allocator* self, size_t badSize) noexcept;


    class initializer {

      inline constexpr static size_t TotalChunkAllocatorCount = SoloChunkAllocatorCount + DualChunkAllocatorCount;

      uintptr_t                   slotsPerChunkMap[SoloChunkAllocatorCount];
      impl::pool_chunk_allocator* chunkAllocators[TotalChunkAllocatorCount];



      [[nodiscard]] AGT_forceinline size_t get_slots_per_chunk_for(size_t slotSize) const noexcept {
        AGT_assert(std::has_single_bit(slotSize));
        AGT_assert(32 <= slotSize && slotSize <= 256);
        size_t index = std::countr_zero(slotSize) - std::countr_zero(32ull);
        return slotsPerChunkMap[index];
      }

      [[nodiscard]] AGT_forceinline size_t get_slots_per_chunk_for(dual_sizes sizes) const noexcept {
        return get_slots_per_chunk_for(sizes.small + sizes.large);
      }

      [[nodiscard]] AGT_forceinline impl::pool_chunk_allocator& chunk_allocator_for_pool(size_t i) const noexcept {
        const size_t index = ChunkAllocatorIndexArray[i];
        return *chunkAllocators[index];
      }

      [[nodiscard]] AGT_forceinline static object_pool &get_pool_for_solo(object_allocator *self, size_t i) noexcept {
        return self->objectPools[SoloPoolIndexArray[i]];
      }

      [[nodiscard]] AGT_forceinline static std::pair<object_pool &, object_pool &> get_pools_for_dual(object_allocator *self, size_t i) noexcept {
        auto [smallIndex, largeIndex] = DualPoolIndicesArray[i];
        return {self->objectPools[smallIndex], self->objectPools[largeIndex]};
      }


    public:

      inline initializer(agt_instance_t inst, object_allocator *self) noexcept
          : slotsPerChunkMap{
                AGT_builtin(inst, object_slots_per_chunk16),
                AGT_builtin(inst, object_slots_per_chunk32),
                AGT_builtin(inst, object_slots_per_chunk64),
                AGT_builtin(inst, object_slots_per_chunk128),
                AGT_builtin(inst, object_slots_per_chunk256)
            },
            chunkAllocators{} {
        size_t i = 0;
        for (; i < SoloChunkAllocatorCount; ++i)
          chunkAllocators[i] = &self->soloChunkAllocators[i];
        for (size_t j = 0; j < DualChunkAllocatorCount; ++i, ++j)
          chunkAllocators[i] = &self->dualChunkAllocators[j];
      }

      AGT_forceinline void init_pool(object_allocator* self, size_t i) const noexcept {
        impl::_init_local_pool(self->objectPools[i],
                               ObjectPoolSizes[i],
                               chunk_allocator_for_pool(i));
      }

      AGT_forceinline void init_solo_chunk_allocator(object_allocator *self, size_t i) const noexcept {
        impl::_init_chunk_allocator(
            self->soloChunkAllocators[i],
            get_pool_for_solo(self, i),
            get_slots_per_chunk_for(SoloChunkAllocatorSizes[i])
            );
      }

      AGT_forceinline void init_dual_chunk_allocator(object_allocator *self, size_t i) const noexcept {
        auto &&[smallPool, largePool] = get_pools_for_dual(self, i);
        impl::_init_chunk_allocator(self->dualChunkAllocators[i],
                                    smallPool,
                                    largePool,
                                    get_slots_per_chunk_for(DualChunkAllocatorSizes[i]));
      }
    };


    AGT_forceinline void    _init_local_allocator(agt_instance_t inst, object_allocator* allocator) noexcept {
      initializer helper{inst, allocator};

      for (size_t i = 0; i < ObjectPoolCount; ++i)
        helper.init_pool(allocator, i);
      for (size_t i = 0; i < SoloChunkAllocatorCount; ++i)
        helper.init_solo_chunk_allocator(allocator, i);
      for (size_t i = 0; i < DualChunkAllocatorCount; ++i)
        helper.init_dual_chunk_allocator(allocator, i);
    }

    template <size_t Size>
    AGT_forceinline object* _local_alloc(object_allocator* allocator) noexcept;

    AGT_forceinline object* _local_dyn_alloc(object_allocator* allocator, size_t size) noexcept {
      static_assert(std::has_single_bit(MinAllocAlignment));

      if (size > MaxAllocSize) [[unlikely]] {
        reportBadSize(allocator, size);
        return nullptr;
      }

      const size_t alignedSize = align_to<MinAllocAlignment>(size);
      const size_t poolIndex   = PoolIndexLookupTable[ alignedSize / MinAllocAlignment ];

      return pool_alloc<thread_unsafe>(allocator->objectPools[poolIndex]);


    }

    AGT_forceinline void    _destroy_local_allocator(object_allocator* allocator) noexcept;

    AGT_forceinline void    _trim_local_allocator(object_allocator* allocator, int poolIndex = -1) noexcept;
  }*/


  [[nodiscard]] agt_ctx_t    acquire_ctx(agt_instance_t instance = nullptr) noexcept;

  // Returns nullptr if ctx has not been initialized
  // Should only really be called if it is already known that the local context has been initialized within the calling module
  // Note that this is specific to the calling module
  [[nodiscard]] agt_ctx_t    get_ctx() noexcept;

  [[nodiscard]] agt_status_t new_ctx(agt_ctx_t& ctx, agt_instance_t instance, agt_executor_t executor, const agt_allocator_params_t* allocatorParams) noexcept;

  void                       destroy_ctx(agt_ctx_t ctx) noexcept;




  [[nodiscard]] inline agt_instance_t get_instance(agt_ctx_t ctx) noexcept;

  [[nodiscard]] inline bool           is_agent_execution_context(agt_ctx_t ctx) noexcept;

  inline agt_ctx_t                    set_ctx(agt_ctx_t ctx) noexcept;

  [[nodiscard]] inline agt_executor_t get_executor(agt_ctx_t ctx) noexcept;

  [[nodiscard]] inline agt_agent_t    get_bound_agent(agt_ctx_t ctx) noexcept;

  [[nodiscard]] inline agt_message_t  get_current_message(agt_ctx_t ctx) noexcept;

}// namespace agt

extern "C" {

struct agt_ctx_st {
  agt_flags32_t                  flags;
  agt_u32_t                      threadId;
  agt_executor_t                 executor;
  agt_agent_t                    boundAgent;
  agt_message_t                  currentMessage;
  agt_instance_t                 instance;
  uintptr_t                      fastThreadId;
  const agt::export_table*       exports;
  agt::reg_impl::registry_cache  registryCache;
  agt::alloc_impl::ctx_allocator allocator;
};

}


template<size_t Size>
agt::impl::ctx_pool&           agt::get_ctx_pool(agt_ctx_t ctx) noexcept {
  // constexpr static size_t PoolIndex = alloc_impl::get_pool_index(Size);
  AGT_invariant( ctx != nullptr );

#if defined(AGT_STATIC_ALLOCATOR_SIZES)
  static_assert( Size <= alloc_impl::MaxAllocSize, "Size parameter to get_ctx_pool<Size>(ctx) exceeds the maximum possible allocation size" );
#else
  if ( Size > alloc_impl::get_max_alloc_size(ctx->allocator) ) [[unlikely]] {
    // TODO: throw error of some sort....
    abort();
  }
#endif
  return *alloc_impl::get_ctx_pool_unchecked(ctx->allocator, Size);
}

agt::impl::ctx_pool&           agt::get_ctx_pool_dyn(size_t size, agt_ctx_t ctx) noexcept {
  // constexpr static size_t PoolIndex = alloc_impl::get_pool_index(Size);
  AGT_invariant( ctx != nullptr );
  if ( size > alloc_impl::get_max_alloc_size(ctx->allocator) ) [[unlikely]] {
    // TODO: throw error of some sort....
    abort();
  }
  return *alloc_impl::get_ctx_pool_unchecked(ctx->allocator, size);
}



AGT_forceinline agt_instance_t agt::get_instance(agt_ctx_t ctx) noexcept {
  return ctx->instance;
}

AGT_forceinline agt_executor_t agt::get_executor(agt_ctx_t ctx) noexcept {
  return ctx->executor;
}

AGT_forceinline agt_agent_t    agt::get_bound_agent(agt_ctx_t ctx) noexcept {
  return ctx->boundAgent;
}

AGT_forceinline agt_message_t  agt::get_current_message(agt_ctx_t ctx) noexcept {
  return ctx->currentMessage;
}







#endif//AGATE_CONTEXT_THREAD_CONTEXT_HPP
