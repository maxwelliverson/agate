//
// Created by maxwe on 2022-08-18.
//

#ifndef AGATE_ALLOC_TEST_H
#define AGATE_ALLOC_TEST_H


#if defined(__cplusplus)
#define ALLOC_TEST_NAMESPACE_BEGIN extern "C" {
#define ALLOC_TEST_NAMESPACE_END }
#include <cstdint>
#else
#define ALLOC_TEST_NAMESPACE_BEGIN
#define ALLOC_TEST_NAMESPACE_END
#include <stdint.h>
#endif




ALLOC_TEST_NAMESPACE_BEGIN

typedef struct AllocTestOptions            AllocTestOptions;
typedef struct AllocTestAlgorithmVTable    AllocTestAlgorithmVTable;
typedef struct AllocTestDistributionVTable AllocTestDistributionVTable;
typedef struct AllocTestLibrary            AllocTestLibrary;
typedef struct AllocTestOp                 AllocTestOp;
typedef struct AllocTestRegistry_st*       AllocTestRegistry;


typedef enum   AllocTestOpKind {
  ALLOC_TEST_OP_ALLOC,
  ALLOC_TEST_OP_FREE
} AllocTestOpKind;


typedef uint32_t (*PFN_allocTestAlloc)(void* allocator, uint32_t count);
typedef void     (*PFN_allocTestFree)(void* allocator, uint32_t offset, uint32_t count);
typedef int      (*PFN_allocTestPrintState)(void* allocator, size_t* bytesWritten, char* buffer, size_t bufLength);
typedef void*    (*PFN_allocTestNewAllocator)(const AllocTestOptions* options);
typedef void     (*PFN_allocTestDeleteAllocator)(void* allocator);

typedef void     (*PFN_allocTestDistribute)(void* distribution, uint32_t randomBits, AllocTestOp* pOp);
typedef void*    (*PFN_allocTestNewDistribution)(const AllocTestOptions* options);
typedef void     (*PFN_allocTestDeleteDistribution)(void* distribution);

typedef void     (*PFN_allocTestRegisterAlgorithm)(AllocTestRegistry registry, const char* algorithmID, const char* description, const AllocTestAlgorithmVTable* vtable);
typedef void     (*PFN_allocTestRegisterDistribution)(AllocTestRegistry registry, const char* distributionID, const char* description, const AllocTestDistributionVTable* vtable);


struct AllocTestOptions {
  uint32_t                 structSize;
  uint32_t                 maxAllocSize;
  uint32_t                 minAllocSize;
  uint32_t                 maxAllocCount;
  uint64_t                 rngSeed;
  uint64_t                 allocatorBitMapSize;
  uint64_t                 bytesPerUnit;
  uint64_t                 opCount;
  const char* const*       algorithmIds;
  size_t                   algorithmCount;
  const char*              distributionId;
};

struct AllocTestAlgorithmVTable {
  PFN_allocTestAlloc           alloc;
  PFN_allocTestFree            free;
  PFN_allocTestPrintState      printState;
  PFN_allocTestNewAllocator    newAllocator;
  PFN_allocTestDeleteAllocator deleteAllocator;
};

struct AllocTestDistributionVTable {
  PFN_allocTestDistribute         distribute;
  PFN_allocTestNewDistribution    newDistribution;
  PFN_allocTestDeleteDistribution deleteDistribution;
};

struct AllocTestOp {
  AllocTestOpKind kind;
  union {
    uint32_t size;
    uint32_t index;
  };
};

struct AllocTestLibrary {
  AllocTestRegistry                 registry;
  PFN_allocTestRegisterAlgorithm    registerAlgorithm;
  PFN_allocTestRegisterDistribution registerDistribution;
};



// Libraries must implement the following

extern void allocTestRegisterLibrary(const AllocTestLibrary* library, const AllocTestOptions* options, int argc, char** argv);



#define allocTestRegisterAlgorithm(library, name, desc, vtable) ((*(library)->registerAlgorithm)((library)->registry, name, desc, vtable))
#define allocTestRegisterDistribution(library, name, desc, vtable) ((*(library)->registerDistribution)((library)->registry, name, desc, vtable))


ALLOC_TEST_NAMESPACE_END

#if defined(__cplusplus)

#include <concepts>
namespace alloc_test {
  template <typename T>
  concept allocator = requires(T* a, uint32_t u32, const AllocTestOptions& opt, char* buf, size_t n, size_t* pN) {
                             { T::name() }     -> std::same_as<const char*>;
                             { a->alloc(u32) } -> std::convertible_to<uint32_t>;
                             a->free(u32, u32);
                             { a->print(buf, n, pN) } -> std::same_as<bool>;
                             new T(opt);
                             delete a;
                           };

  template <typename T>
  concept distribution = requires(T* d, uint32_t u32, AllocTestOp& op, const AllocTestOptions& opt) {
                           { T::name() } -> std::same_as<const char*>;
                           d->distribute(u32, op);
                           new T(opt);
                           delete d;
                         };

  template <typename T>
  concept has_description = requires{
                              { T::description() } -> std::same_as<const char*>;
                            };
}



template <alloc_test::allocator A>
inline static void registerAlgorithm(const AllocTestLibrary* library) noexcept {
  AllocTestAlgorithmVTable vtable {
      [](void* a, uint32_t count) -> uint32_t { return static_cast<A*>(a)->alloc(count); },
      [](void* a, uint32_t index, uint32_t count) { static_cast<A*>(a)->free(index, count); },
      [](void* a, char* buf, size_t bufSize, size_t* bytesWritten) -> bool { return static_cast<A*>(a)->print(buf, bufSize, bytesWritten); },
      [](const AllocTestOptions& opt) -> void* { return new A(opt); },
      [](void* a){ delete static_cast<A*>(a); }
  };
  if constexpr (alloc_test::has_description<A>)
    (*library->registerAlgorithm)(library->registry, A::name(), A::description(), &vtable);
  else
    (*library->registerAlgorithm)(library->registry, A::name(), nullptr, &vtable);
}

template <alloc_test::distribution D>
inline static void registerDistribution(const AllocTestLibrary* library) noexcept {
  AllocTestDistributionVTable vtable {
      [](void* d, uint32_t value, AllocTestOp* pOp) { static_cast<D*>(d)->distribute(value, *pOp); },
      [](const AllocTestOptions& opt) -> void* { return new D(opt); },
      [](void* d){ delete static_cast<D*>(d); }
  };
  if constexpr (alloc_test::has_description<D>)
    (*library->registerDistribution)(library->registry, D::name(), D::description(), &vtable);
  else
    (*library->registerDistribution)(library->registry, D::name(), nullptr, &vtable);
}

#endif


#endif//AGATE_ALLOC_TEST_H
