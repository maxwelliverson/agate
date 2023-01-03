//
// Created by maxwe on 2022-12-08.
//

#ifndef AGATE_CONTEXT_THREAD_CONTEXT_HPP
#define AGATE_CONTEXT_THREAD_CONTEXT_HPP

#include "config.hpp"

// #include "core/objects.hpp"
#include "agate/align.hpp"
#include "core/context.hpp"
#include "core/error.hpp"
#include "core/object_pool.hpp"


#include <bit>
#include <type_traits>
#include <concepts>
#include <array>
#include <algorithm>

namespace agt {

  namespace alloc_impl {
    struct dual_sizes{
      size_t small;
      size_t large;
    };

    /*inline constexpr static slots_info SlotsPerChunkMap[] = {
        { 32,  1024 },
        { 64,  1024 },
        { 128, 256  },
        { 256, 64   }
    };*/

    inline constexpr static size_t     SoloChunkAllocatorSizes[] = {
        16,
        32,
        64,
        128,
        256
    };
    inline constexpr static dual_sizes DualChunkAllocatorSizes[] = {
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
    inline constexpr static size_t SoloChunkAllocatorCount = std::size(SoloChunkAllocatorSizes);
    inline constexpr static size_t DualChunkAllocatorCount = std::size(DualChunkAllocatorSizes);
    inline constexpr static size_t ObjectPoolCount         = std::size(ObjectPoolSizes);

    struct object_allocator {
      impl::solo_pool_chunk_allocator soloChunkAllocators[SoloChunkAllocatorCount];
      impl::dual_pool_chunk_allocator dualChunkAllocators[DualChunkAllocatorCount];
      object_pool                     objectPools[ObjectPoolCount];
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

    consteval size_t get_pool_index_for_size(size_t n) noexcept {
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
    }

    template <size_t N, size_t ...Values>
    struct pool_index_lut_generator : pool_index_lut_generator<N + MinAllocAlignment, Values..., get_pool_index_for_size(N)> { };
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

      inline initializer(agt_ctx_t ctx, object_allocator *self) noexcept
          : slotsPerChunkMap{
                ctxGetBuiltin(ctx, builtin_value::objectSlotsPerChunk16),
                ctxGetBuiltin(ctx, builtin_value::objectSlotsPerChunk32),
                ctxGetBuiltin(ctx, builtin_value::objectSlotsPerChunk64),
                ctxGetBuiltin(ctx, builtin_value::objectSlotsPerChunk128),
                ctxGetBuiltin(ctx, builtin_value::objectSlotsPerChunk256)},
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


    AGT_forceinline void    _init_local_allocator(agt_ctx_t ctx, object_allocator* allocator) noexcept {
      initializer helper{ctx, allocator};

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
  }

  struct thread_state {
    agt_executor_t               executor;
    agt_agent_t                  boundAgent;
    agt_message_t                currentMessage;
    agt_u32_t                    threadId;
    uintptr_t                    fastThreadId;
    agt_ctx_t                    context;
    alloc_impl::object_allocator allocator;
  };

  agt_status_t                init_thread_state(agt_ctx_t globalCtx, agt_executor_t executor) noexcept;

  [[nodiscard]] agt_ctx_t     get_ctx() noexcept;

  [[nodiscard]] thread_state* get_thread_state() noexcept;

  [[nodiscard]] bool          is_agent_execution_context() noexcept;

  agt_ctx_t                   set_global_ctx(agt_ctx_t ctx) noexcept;

  thread_state *              set_thread_state(thread_state * ctx) noexcept;

  void                        set_agent_execution_context(bool isAEC = true) noexcept;



}// namespace agt









#endif//AGATE_CONTEXT_THREAD_CONTEXT_HPP
