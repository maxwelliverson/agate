//
// Created by maxwe on 2023-01-25.
//

#ifndef AGATE_SHMEM_H
#define AGATE_SHMEM_H

#include <agate/core.h>

AGT_begin_c_namespace

/**
 * Shared Memory Management
 * */




typedef agt_handle_t                   agt_shmem_t; // A handle with a named typedef for communicating purpose
typedef struct agt_shpool_st*          agt_shpool_t;
typedef struct agt_sheap_st*           agt_sheap_t;
typedef agt_u64_t                      agt_vmem_t;


typedef enum agt_shmem_alloc_flag_bits_t {
  AGT_SHMEM_ALLOC_TRANSIENT    = 0x1,  ///< By default, allocations are expected to remain fixed at the same location in memory for the duration of their lifetimes; this allows for persistent caching of local address mappings, but also forbids reallocation (as growing/shrinking may non-deterministically cause relocatation). If AGT_SHMEM_ALLOC_TRANSIENT is specified, the resulting allocation is not expected to remain in the same position in memory, so the trade-offs are reversed: reallocation is possible (and expected), but persistent address mappings may not be cached between uses; the local mapping must be reacquired every time the memory is accessed in any capacity. This should be used sparingly, but in cases where resizable shared memory is absolutely required, this flag must be specified.
  AGT_SHMEM_ALLOC_ONE_AND_DONE = 0x2   ///< By default, allocations are reference counted, and are freed when the reference count becomes 0. If AGT_SHMEM_ALLOC_ONE_AND_DONE is specified, the allocation is instead freed upon the first valid call to agt_release. Useful in cases where the lifetime of the allocation is known ahead of time; eg. sending data to a single other instance, who then releases the mapping once they're done. Also useful in cases where the allocation should persist while the reference count is 0 (eg. an allocation that is referred to by other shared allocations )
} agt_shmem_alloc_flag_bits_t;
typedef agt_flags32_t agt_shmem_alloc_flags_t;

typedef enum agt_sheap_flag_bits_t {
  AGT_SHEAP_SHARED_OWNERSHIP    = 0x1, ///< If specified, this sheap may be imported (and used) by other instances. This incurs a potentially not-insignificant overhead for allocation operations, so the behaviour is explicitly opt-in.
  AGT_SHEAP_FIXED_ADDRESS_SPACE = 0x2, ///< If specified, this sheap has a limited, fixed-size address space. While this means that memory exhaustion (and subsequently, allocation failure) is much more likely to occur, it guarantees that any fixed allocations made from the same sheap will have constant offsets from each other, allowing for very efficient internal references.
  AGT_SHEAP_TRANSIENT           = 0x4, ///< If specified, this sheap makes transient allocations, which are resizable at the expense of not having a
} agt_sheap_flag_bits_t;
typedef agt_flags32_t agt_sheap_flags_t;

typedef enum agt_shpool_flag_bits_t {

} agt_shpool_flag_bits_t;
typedef agt_flags32_t agt_shpool_flags_t;


typedef struct agt_sheap_create_info_t {
  agt_async_t  async;
  agt_name_t   name;
  agt_size_t   fixedAddressSpaceSize;
} agt_sheap_create_info_t;

typedef struct agt_shpool_create_info_t {
  agt_async_t  async;
  agt_name_t   name;
  agt_u32_t    blockSize;
  agt_u32_t    maxBlocksPerChunk;
} agt_shpool_create_info_t;


typedef struct agt_shalloc_result_t {
  agt_shmem_t handle;
  void*       address;
  size_t      size;
} agt_shalloc_result_t;

typedef struct agt_shmem_mapping_t {
  void*       address;
  size_t      size;
} agt_shmem_mapping_t;




/**
 * If size is 0, the whole range (starting from the specified offset) will be mapping, and the mapped range returned
 * */
AGT_shmem_api agt_status_t agt_shmem_map(agt_ctx_t ctx, agt_shmem_t mem, size_t size, size_t offset, agt_shmem_mapping_t* pMappingInfo) AGT_noexcept;
/**
 * */
AGT_shmem_api void         agt_shmem_unmap(agt_ctx_t ctx, void* address) AGT_noexcept;
/**
 * Will also unmap any local mappings.
 * */
AGT_shmem_api void         agt_shmem_free(agt_ctx_t ctx, agt_shmem_t shmem) AGT_noexcept;



AGT_shmem_api agt_status_t agt_new_sheap(agt_ctx_t ctx, agt_sheap_t* pSheap, const agt_sheap_create_info_t* pCreateInfo, agt_sheap_flags_t flags) AGT_noexcept;
AGT_shmem_api void         agt_destroy_sheap(agt_sheap_t heap) AGT_noexcept;

AGT_shmem_api agt_status_t agt_sheap_alloc(agt_sheap_t heap, agt_shmem_t* mem, size_t size, size_t alignment, agt_shmem_mapping_t* pMapping) AGT_noexcept;
AGT_shmem_api agt_status_t agt_sheap_realloc(agt_sheap_t heap, agt_shmem_t* mem, size_t size, size_t alignment, agt_shmem_mapping_t* pMapping) AGT_noexcept;




AGT_shmem_api agt_status_t agt_new_shpool(agt_ctx_t ctx, agt_shpool_t* pShPool, const agt_shpool_create_info_t* pCreateInfo, agt_shpool_flags_t flags) AGT_noexcept;
AGT_shmem_api void         agt_destroy_shpool(agt_shpool_t pool) AGT_noexcept;

AGT_shmem_api agt_status_t agt_shpool_alloc(agt_shpool_t pool, agt_shmem_t* mem, agt_shmem_mapping_t* pMapping) AGT_noexcept;
AGT_shmem_api void         agt_shpool_reset(agt_shpool_t pool) AGT_noexcept;


AGT_end_c_namespace

#endif//AGATE_SHMEM_H
