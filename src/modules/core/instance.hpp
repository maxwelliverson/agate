//
// Created by maxwe on 2021-11-25.
//

#ifndef JEMSYS_AGATE2_CONTEXT_HPP
#define JEMSYS_AGATE2_CONTEXT_HPP

#include "config.hpp"

#include "agate/flags.hpp"
#include "agate/integer_division.hpp"
#include "agate/version.hpp"
#include "export_table.hpp"
#include "impl/allocator.hpp"
#include "impl/registry.hpp"
#include "impl/monitor_list.hpp"
#include "impl/hazptr_manager.hpp"



extern "C" {

struct agt_instance_st {
  agt_u32_t                  flags;
  agt_u32_t                  id;
  agt::version               version;
  agt_u32_t                  refCount; // How many times has agate been loaded with this instance; only relevant when linking to external libraries that also use agate
  agt::export_table*         exports;
  agt_u32_t                  asyncStructSize;
  const agt_value_type_t*    attrTypes;
  const agt_value_t*         attrValues;
  size_t                     attrCount;
  agt_error_handler_t        errorHandler;
  void*                      errorHandlerUserData;
  agt_internal_log_handler_t logCallback;
  void*                      logCallbackUserData;
  agt_u32_t                  contextCount;
  agt::impl::monitor_list    monitorList;



  agt_size_t                 defaultMaxObjectSize;


  ratio_multiplier           tscToNs;
  ratio_multiplier           nsToTsc;

  agt_u32_t                  instanceNameOffset;
  agt_u32_t                  instanceNameLength;
  agt_u32_t                  defaultCtxAllocatorParamsOffset;
  agt_u32_t                  localNameRegistryOffset;
  agt_u32_t                  pageAllocatorOffset;
  agt_u32_t                  threadDescriptorsOffset;



  AGT_declare_default_allocator_params(defaultAllocatorParams);

  // local name registry
  // page allocator
  // thread descriptors (unless context is single threaded)
  // shared context
  // shared memory allocator
  // shared name registry
  // shared processes
};

}

AGT_DEFINE_GET_DEFAULT_ALLOCATOR_PARAMS_FUNC(defaultAllocatorParams)

namespace agt {

  AGT_BITFLAG_ENUM(instance_flags, agt_u32_t) {
      is_shared            = 0x1,
      is_single_threaded   = 0x2,
      is_statically_linked = 0x4,
      may_ignore_errors    = 0x8,
  };

  struct private_instance : agt_instance_st {

  };

  struct st_private_instance : private_instance {

  };

  struct mt_private_instance : private_instance {

  };

  struct shared_instance : agt_instance_st {
    agt_u32_t        processId;
    /*SharedPageMap    sharedPageMap;
    PageList         localFreeList;
    ListNode*        emptyLocalPage;

    size_t          localPageSize;
    size_t          localEntryCount;

    shared_cb*            sharedCb;
    process_descriptor*   self;
    shared_registry*      sharedRegistry;
    shared_processes*     sharedProcesses;
    shared_allocator*     sharedAllocator;
    shared_allocation_lut sharedAllocations;*/
  };

  struct st_shared_instance : shared_instance {
    /*SharedPageMap    sharedPageMap;
    PageList         localFreeList;
    ListNode*        emptyLocalPage;

    size_t          localPageSize;
    size_t          localEntryCount;

    shared_cb*            sharedCb;
    process_descriptor*   self;
    shared_registry*      sharedRegistry;
    shared_processes*     sharedProcesses;
    shared_allocator*     sharedAllocator;
    shared_allocation_lut sharedAllocations;*/
  };

  struct mt_shared_instance : shared_instance {
    /*SharedPageMap    sharedPageMap;
    PageList         localFreeList;
    ListNode*        emptyLocalPage;

    size_t          localPageSize;
    size_t          localEntryCount;

    shared_cb*            sharedCb;
    process_descriptor*   self;
    shared_registry*      sharedRegistry;
    shared_processes*     sharedProcesses;
    shared_allocator*     sharedAllocator;
    shared_allocation_lut sharedAllocations;*/
  };

  enum class builtin_value : agt_u32_t {
    process_name,
    defaultPrivateChannelMessageSize,
    defaultLocalChannelMessageSize,
    defaultSharedChannelMessageSize,
    defaultPrivateChannelSlotCount,
    defaultLocalChannelSlotCount,
    defaultSharedChannelSlotCount,
    async_struct_size,
    signal_struct_size,
    object_slots_per_chunk16,
    object_slots_per_chunk32,
    object_slots_per_chunk64,
    object_slots_per_chunk128,
    object_slots_per_chunk256
  };

  namespace impl {
#define AGT_builtin_value(type_, value_) template <> struct builtin_value_info<builtin_value::value_> { inline constexpr static auto value = builtin_value::value_; using type = type_; }
    template <builtin_value Value>
    struct builtin_value_info;

    AGT_builtin_value(const char*, process_name);
    AGT_builtin_value(size_t,      async_struct_size);
    AGT_builtin_value(size_t,      signal_struct_size);
  }

#define AGT_builtin(inst_, value_) ((typename ::agt::impl::builtin_value_info<::agt::builtin_value::value_>::type)::agt::inst_get_builtin(inst_, ::agt::builtin_value::value_))


  uintptr_t     inst_get_builtin(agt_instance_t inst, builtin_value value) noexcept;

  void*         instance_mem_alloc(agt_instance_t instance, size_t size, size_t alignment) noexcept;
  void          instance_mem_free(agt_instance_t instance, void* ptr, size_t size, size_t alignment) noexcept;

  void*         instance_alloc_pages(agt_instance_t inst, size_t totalSize) noexcept;
  void          instance_free_pages(agt_instance_t inst, void* addr, size_t totalSize) noexcept;


  context_id    get_inst_id(agt_ctx_t ctx) noexcept;


  bool          instance_may_ignore_errors(agt_instance_t inst) noexcept;


  inline static void get_instance_name(agt_instance_t instance, agt_string_t& name) noexcept {

  }


/*

  void*         ctxLocalAlloc(agt_ctx_t ctx, size_t size, size_t alignment) noexcept;
  void          ctxLocalFree(agt_ctx_t ctx, void* memory, size_t size, size_t alignment) noexcept;

  void*         ctxSharedAlloc(agt_ctx_t ctx, size_t size, size_t alignment, shared_handle& allocationId) noexcept;
  void          ctxSharedFree(agt_ctx_t ctx, shared_handle allocId) noexcept;
  void          ctxSharedFree(agt_ctx_t ctx, void* memory, size_t size, size_t alignment) noexcept;

  HandleHeader* ctxAllocHandle(agt_ctx_t ctx, size_t size, size_t alignment) noexcept;
  void          ctxFreeHandle(agt_ctx_t ctx, HandleHeader* handle, size_t size, size_t alignment) noexcept;

  SharedObjectHeader* ctxAllocSharedObject(agt_ctx_t context, size_t size, size_t alignment) noexcept;
  void                ctxFreeSharedObject(agt_ctx_t ctx, SharedObjectHeader* object, size_t size, size_t alignment) noexcept;
*/


  shared_handle ctxExportSharedHandle(agt_instance_t inst, const void* sharedObject) noexcept;
  void*         ctxImportSharedHandle(agt_instance_t inst, shared_handle sharedHandle) noexcept;



  // agt_status_t ctxOpenHandleById(agt_instance_t inst, agt_object_id_t id, HandleHeader*& handle) noexcept;
  // agt_status_t ctxOpenHandleByName(agt_instance_t inst, const char* name, HandleHeader*& handle) noexcept;




  agt_status_t inst_claim_local_name(agt_instance_t inst, const char* pName, size_t nameLength, name_token& token) noexcept;
  void         inst_release_local_name(agt_instance_t inst, name_token nameToken) noexcept;
  agt_status_t inst_claim_shared_name(agt_instance_t inst, const char* pName, size_t nameLength, name_token& token) noexcept;
  void         inst_release_shared_name(agt_instance_t inst, name_token nameToken) noexcept;
  void         inst_bind_name(agt_instance_t inst, name_token nameToken, object* obj) noexcept;

  agt_status_t inst_lookup_object_by_name(agt_instance_t inst, const char* pName, size_t nameLength, object*& resultObject) noexcept;

  agt_status_t inst_enumerate_named_objects(agt_instance_t inst, size_t& count, const char** pNames) noexcept;
  agt_status_t inst_enumerate_shared_objects(agt_instance_t inst, size_t& count, const char** pNames) noexcept;


  void         inst_set_thread_ctx(agt_instance_t inst, agt_ctx_t ctx) noexcept;
  agt_ctx_t    inst_get_thread_ctx(agt_instance_t inst) noexcept;


  /*agt_status_t  createCtx(agt_ctx_t& pCtx) noexcept;
  void          destroyCtx(agt_ctx_t ctx) noexcept;*/


  void*         ctxAcquireLocalAsyncData(agt_ctx_t ctx) noexcept;
  void          ctxReleaseLocalAsyncData(agt_ctx_t ctx, void* data) noexcept;

  void*         ctxAcquireSharedAsyncData(agt_ctx_t ctx, async_data_t& handle) noexcept;
  void*         ctxMapSharedAsyncData(agt_ctx_t ctx, async_data_t handle) noexcept;
  void          ctxReleaseSharedAsyncData(agt_ctx_t ctx, async_data_t handle) noexcept;
}

#endif//JEMSYS_AGATE2_CONTEXT_HPP
