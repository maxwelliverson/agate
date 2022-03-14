//
// Created by maxwe on 2022-03-14.
//

#include "shared.hpp"
#include "support/atomic.hpp"


#include <utility>


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


#pragma comment(lib, "mincore")


namespace {

  inline constexpr wchar_t SharedControlBlockName[] = L"agate-scb"; // NOTE: Might need to make a more complex name to avoid accidental clashes.

  inline constexpr size_t SharedControlBlockMinLength = AGT_VIRTUAL_PAGE_SIZE;

  inline constexpr size_t SharedControlBlockDefaultLength = SharedControlBlockMinLength;

  inline constexpr size_t SharedControlBlockAlignment = AGT_VIRTUAL_PAGE_SIZE;

  // Arbitrary, should probably change after profiling. Currently 64 MB.
  inline constexpr size_t SharedControlBlockVirtualSize = (0x1 << 26);

  inline constexpr size_t SharedControlBlockMaxNameLength = 64;

  inline constexpr size_t SharedSegmentMaxNameLength = 40;

  struct shared_segment {
    agt_u32_t id;
    agt_u32_t next;
    agt_u32_t prev;
    agt_u32_t refCount;
    size_t    size;
    char      name[SharedSegmentMaxNameLength];
  };
}


struct agt::process_descriptor {

};

struct agt::shared_registry {

};

struct agt::shared_cb {
  agt_u32_t initialized;
  agt_u32_t initializingLock;
  agt_u32_t initializationLock;
  size_t    cbSize;
  size_t    virtualSize;
  agt_u32_t abiVersion;
  agt_u32_t flags;
  size_t    registryOffset;
  size_t    processesOffset;
  size_t    allocatorOffset;
};

struct agt::shared_segment_descriptor {
  agt_u32_t                  refCount;
  size_t                     size;
  void*                      address;
  shared_segment_descriptor* parent;
};

struct agt::shared_allocation_descriptor {
  shared_allocation_id       id;
  size_t                     size;
  void*                      address;
  shared_segment_descriptor* segment;
};

namespace {

  void doInitSharedCb(agt::shared_cb* cb) {

  }

  void initializeSharedCb(agt::shared_cb* cb) {
    if (cb->initialized)
      return;

    agt_u32_t lockValue;

    do {
      lockValue = 0;
      if (agt::impl::atomicCompareExchange(cb->initializingLock, lockValue, 1))
        break;
      agt::impl::atomicWait(cb->initializingLock, lockValue);
    } while (true);

    if (!cb->initialized) {
      doInitSharedCb(cb);
      cb->initialized = true;
    }

    agt::impl::atomicStore(cb->initializingLock, 0);
    agt::impl::atomicNotifyOne(cb->initializingLock);
  }

  agt_status_t initSharedAllocationLookupTable(agt::shared_ctx* ctx) noexcept {

  }


  struct file {
    using handle_type = void*;

    using handle_type = void*;

    static size_t getSize(handle_type hdl) noexcept {
      LARGE_INTEGER size;
      if (!GetFileSizeEx(hdl, &size)) {
        // TODO: Find some fix? Should only fail here if the file was created without FILE_READ_ATTRIBUTES access, which shuoldn't happen...
        return 0;
      }
      return size.QuadPart;
    }

    static void   destroy(handle_type hdl, size_t siz) noexcept {
      CloseHandle(hdl);
    }
  };

  struct placeholder_address {

    using handle_type = void*;

    static size_t getSize(handle_type hdl) noexcept {
      return 0;
    }

    static void   destroy(handle_type hdl, size_t siz) noexcept {
      VirtualFree(hdl, siz, 0);
    }

  };

  struct mapping {

    using handle_type = void*;

    static size_t getSize(handle_type hdl) noexcept {
      return 0;
    }

    static void   destroy(handle_type hdl, size_t siz) noexcept {
      UnmapViewOfFile2(INVALID_HANDLE_VALUE, hdl, 0);
    }
  };




  template <typename T>
  class scoped {
  public:
    using value_type = typename T::handle_type;

    scoped(value_type val) noexcept : scoped(val, T::getSize(val)) {}
    scoped(value_type val, size_t siz) noexcept : value_(val), size_(siz), hasOwnership_(true) {}

    ~scoped() {
      if (hasOwnership_) {
        T::destroy(value_, size_);
      }
    }


    [[nodiscard]] size_t size() const noexcept {
      return size_;
    }

    value_type value() const noexcept {
      return value_;
    }

    value_type consume() noexcept {
      hasOwnership_ = false;
      return value_;
    }

    void invalidate() noexcept {
      hasOwnership_ = false;
    }

    void setSize(size_t size) noexcept {
      size_ = size;
    }

    void setValue(value_type val) noexcept {
      value_ = val;
    }

    void setOwnership(bool owns) noexcept {
      hasOwnership_ = owns;
    }

  private:
    value_type value_;
    size_t     size_;
    bool       hasOwnership_;
  };
}




agt_status_t agt::sharedControlBlockInit(shared_ctx* pSharedCtx) noexcept {

  MEM_ADDRESS_REQUIREMENTS addressRequirements{
      .LowestStartingAddress = nullptr,
      .HighestEndingAddress = nullptr,
      .Alignment = SharedControlBlockAlignment
  };

  MEM_EXTENDED_PARAMETER extParams[] = {
      {
          .Type = MemExtendedParameterAddressRequirements,
          .Pointer = &addressRequirements
      }
  };

  scoped<file> fileMapping = CreateFileMapping2(
      INVALID_HANDLE_VALUE,
      nullptr,
      FILE_MAP_ALL_ACCESS,
      PAGE_READWRITE,
      0,
      SharedControlBlockDefaultLength,
      SharedControlBlockName,
      extParams,
      sizeof(extParams) / sizeof(MEM_EXTENDED_PARAMETER)
      );

  if (!fileMapping.value())
    return AGT_ERROR_UNKNOWN;

  scoped<placeholder_address> baseAddress = VirtualAlloc2(
      INVALID_HANDLE_VALUE,
      nullptr,
      SharedControlBlockVirtualSize,
      MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
      PAGE_NOACCESS,
      extParams,
      sizeof(extParams) / sizeof(MEM_EXTENDED_PARAMETER)
      );

  if (!baseAddress.value())
    return AGT_ERROR_BAD_ALLOC;

  baseAddress.setSize(SharedControlBlockVirtualSize);

  if (!VirtualFree(baseAddress.value(), fileMapping.size(), MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER))
    return AGT_ERROR_BAD_ALLOC;

  scoped<placeholder_address> controlBlockAddr{ baseAddress.value(), fileMapping.size() };
  baseAddress.setValue(((char*)baseAddress.value()) + fileMapping.size());
  baseAddress.setSize(baseAddress.size() - fileMapping.size());

  scoped<mapping> cbMapping {
      MapViewOfFile3(
          fileMapping.value(),
          INVALID_HANDLE_VALUE,
          controlBlockAddr.value(),
          0,
          fileMapping.size(),
          MEM_REPLACE_PLACEHOLDER,
          PAGE_READWRITE,
          nullptr,
          0),
      fileMapping.size()
  };

  if (!cbMapping.value())
    return AGT_ERROR_BAD_ALLOC;

  auto cb = static_cast<shared_cb*>(cbMapping.value());

  initializeSharedCb(cb);


  pSharedCtx->cb = cb;
  pSharedCtx->self = nullptr; // TODO: Implement
  pSharedCtx->registry = static_cast<shared_registry*>(static_cast<void*>((static_cast<char*>(cbMapping.value()) + cb->registryOffset)));
  pSharedCtx->processes = static_cast<shared_processes*>(static_cast<void*>((static_cast<char*>(cbMapping.value()) + cb->processesOffset)));
  pSharedCtx->allocator = static_cast<shared_allocator*>(static_cast<void*>((static_cast<char*>(cbMapping.value()) + cb->allocatorOffset)));
  if (auto status = initSharedAllocationLookupTable(pSharedCtx))
    return status;

  cbMapping.setOwnership(false);
  baseAddress.setOwnership(false);
  controlBlockAddr.setOwnership(false);

  return AGT_SUCCESS;
}


void         agt::sharedControlBlockClose(shared_ctx* pSharedCtx) noexcept {}

