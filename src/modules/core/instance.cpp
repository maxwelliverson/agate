//
// Created by maxwe on 2023-07-28.
//


#include "instance.hpp"
#include "attr.hpp"
#include "time.hpp"

#if AGT_system_windows
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif AGT_system_linux
#include <unistd.h>
#endif

#include <init.hpp>
#include <utility>
#include <random>


namespace {
  inline agt_u32_t _get_thread_id() noexcept {
#if AGT_system_windows
    return static_cast<agt_u32_t>(GetCurrentThreadId());
#elif AGT_system_linux
    return static_cast<agt_u32_t>(gettid());
#endif
  }



  uint64_t get_tsc_hz() noexcept {
    DWORD data;
    DWORD dataSize = sizeof(DWORD);
    if (RegGetValue(HKEY_LOCAL_MACHINE,
                "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                "~MHz", RRF_RT_DWORD, nullptr, &data, &dataSize) == ERROR_SUCCESS) {
      return static_cast<uint64_t>(data) * 1000000;
    }
    return 0;
  }

  AGT_forceinline uint64_t get_100ns_timestamp() noexcept {
    LARGE_INTEGER lrgInt;
    QueryPerformanceCounter(&lrgInt);
    return lrgInt.QuadPart;
  }

  template <typename T>
  [[msvc::noinline, clang::noinline]] void do_not_optimize(T&&) noexcept { }

  std::pair<uint32_t, uint32_t> get_tsc_to_ns_ratio() noexcept {
    if (auto tscHz = get_tsc_hz()) {
      auto billion = 1000000000ull;
      const auto gcd = impl::calculate_gcd(tscHz, billion);
      if (gcd != 1) {
        tscHz /= gcd;
        billion /= gcd;
      }
      if (tscHz > UINT_MAX) {
        std::terminate(); // :(
      }

      return { static_cast<uint32_t>(billion), static_cast<uint32_t>(tscHz) };
    }


    constexpr static uint64_t EstimateTimeout_ms = 500;


    const auto start100ns = get_100ns_timestamp();
    const auto start      = __rdtsc();


    const uint64_t   target100ns = start100ns + (EstimateTimeout_ms * 10000);


    std::minstd_rand rng{};
    uint64_t state = 0;
    uint64_t curr100ns = get_100ns_timestamp();

    while (curr100ns < target100ns) {
      constexpr static auto BatchSize = 10000;
      rng.discard(BatchSize);
      state += rng();
      curr100ns = get_100ns_timestamp();
    }

    do_not_optimize(state);

    const auto end100ns = get_100ns_timestamp();
    const auto end = __rdtsc();

    return { static_cast<uint32_t>((end100ns - start100ns) * 100), static_cast<uint32_t>(end - start) };
  }
}


void         agt::inst_set_thread_ctx(agt_instance_t inst, agt_ctx_t ctx) noexcept {
  // inst->
}
agt_ctx_t    agt::inst_get_thread_ctx(agt_instance_t inst) noexcept {
  return nullptr;
}

bool agt::instance_may_ignore_errors(agt_instance_t inst) noexcept {
  return true; // TODO: implement
}


void* agt::instance_mem_alloc(agt_instance_t instance, size_t size, size_t alignment) noexcept {
  (void)instance;
  return _aligned_malloc(size, alignment);
}
void  agt::instance_mem_free(agt_instance_t instance, void* ptr, size_t size, size_t alignment) noexcept {
  (void)instance;
  (void)size;
  (void)alignment;
  return _aligned_free(ptr);
}

void* agt::instance_alloc_pages(agt_instance_t inst, size_t totalSize) noexcept {
  return VirtualAlloc(nullptr, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void  agt::instance_free_pages(agt_instance_t inst, void *addr, size_t totalSize) noexcept {
  VirtualFree(addr, totalSize, MEM_RELEASE | MEM_DECOMMIT);
}





namespace agt {

  void                 AGT_stdcall destroy_instance_private(agt_instance_t instance, bool invokedOnThreadExit) {
    if (instance) {
      auto exports = instance->exports;
      auto procEntries = instance->exports->pProcSet;
      delete[] procEntries;
      auto modules = exports->modules;
      impl::destroy_monitor_list_local(instance, instance->monitorList); // Don't worry about signalling all remaining waiters; the library is being destroyed, so they won't get a chance to do anything anyways if they *do* exist.
      destroy_default_allocator_params(instance, instance->defaultAllocatorParams);
      delete[] instance->attrTypes;
      delete[] instance->attrValues;

      impl::destroy_hazptr_manager(instance->hazptrManager);

      delete instance;

      if (invokedOnThreadExit)
        exports->_pfn_free_modules_on_thread_exit(modules);
      // else
        // exports->_pfn_free_modules(modules);

      // Really, we should store pointers to each export table that links to instance so that they may each be cleared when instance is destroyed...
      std::memset(exports, 0, exports->tableSize);
    }
  }
  void                 AGT_stdcall destroy_instance_shared(agt_instance_t instance, bool invokedOnThreadExit) {}

  agt_instance_t       AGT_stdcall create_instance_private(agt_config_t configTree, init::init_manager& manager) {

    auto shared = configTree->shared;
    auto exports = configTree->pExportTable; // TODO: Change this so that the loader tracks which table is the largest, ensuring the internal dispatch table is complete.

    const auto pAttrTypes = exports->attrTypes;
    const auto pAttrValues = exports->attrValues;
    const auto attrCount = exports->attrCount;

    auto inst      = new agt_instance_st;
    inst->flags    = 0;
    inst->id       = 0;
    inst->version  = version::from_integer(static_cast<agt_u32_t>(pAttrValues[AGT_ATTR_LIBRARY_VERSION]));
    inst->refCount = 0; // shared->loaderMap.size(); (for now, it is simply how many contexts are attached...)
    inst->exports  = exports;
    inst->attrTypes = exports->attrTypes;
    inst->attrValues = reinterpret_cast<const agt_value_t*>(exports->attrValues);
    inst->attrCount = exports->attrCount;
    inst->errorHandler = nullptr;
    inst->errorHandlerUserData = nullptr;
    inst->logCallback = shared->logHandler;
    inst->logCallbackUserData = shared->logHandlerUserData;
    inst->contextCount = 0;
    inst->defaultMaxObjectSize = 512; // Arbitrary, change later need be.

    agt::impl::init_monitor_list_local(inst, inst->monitorList, 512, attr::thread_count(inst));

    agt::impl::init_hazptr_manager(inst->hazptrManager);

    auto [ tscToNsNum, tscToNsDenom ] = get_tsc_to_ns_ratio();
    precompile_ratio(inst->clockToNsRatio, tscToNsNum, tscToNsDenom);
    precompile_ratio(inst->nsToClockRatio, tscToNsDenom, tscToNsNum);

    /*const auto nativeDurationUnit = pAttrValues[AGT_ATTR_NATIVE_DURATION_UNIT_SIZE_NS];
    const auto durationUnit = pAttrValues[AGT_ATTR_DURATION_UNIT_SIZE_NS];

    if (nativeDurationUnit == durationUnit) {
      set_divisor(inst->timeoutDivisor, 1);
    }
    else {
      if (nativeDurationUnit > durationUnit) {
        // TODO: Implement method of finding closest ratio!
        uint32_t divisor = nativeDurationUnit / durationUnit;
        set_divisor(inst->timeoutDivisor, divisor);
      }
      else {
        // TODO: Implement!!
      }
    }*/


    inst->instanceNameOffset = 0;
    inst->instanceNameLength = 0;
    inst->defaultCtxAllocatorParamsOffset = 0;
    inst->localNameRegistryOffset = 0;
    inst->pageAllocatorOffset = 0;
    inst->threadDescriptorsOffset = 0;

    const auto pAllocatorParams = shared->hasCustomAllocatorParams ? &shared->allocatorParams : nullptr;
    auto status = init_default_allocator_params(inst->defaultAllocatorParams, pAllocatorParams);

    if (status != AGT_SUCCESS) {
      manager.reportError("Invalid allocator parameters (agt_allocator_params_t)");
      return nullptr;
    }

    return inst;
  }
  agt_instance_t       AGT_stdcall create_instance_shared(agt_config_t configTree, init::init_manager& manager) {
    return nullptr;
  }


}
