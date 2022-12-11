//
// Created by maxwe on 2021-11-25.
//

#include "context.hpp"
#include "core/objects.hpp"
#include "support/pool.hpp"
#include "shared.hpp"

#include <cstdlib>
#include <memory>
#include <mutex>


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#pragma comment(lib, "mincore")



using namespace agt;

namespace {

  enum class shared_handle_kind : agt_u32_t {
    internal,
    internal_transient,
    async_data,
    agent,
    message,
    message_pool,
    alloc,
    invalid
  };


  AGT_forceinline agt::shared_handle make_shared_handle(shared_handle_kind kind, agt_u32_t epoch, agt_u32_t segmentId, agt_u32_t pageId, agt_u32_t pageOffset) noexcept;

  AGT_forceinline shared_handle_kind kind(agt::shared_handle handle) noexcept;
  AGT_forceinline agt_u16_t          epoch(agt::shared_handle handle) noexcept;
  AGT_forceinline agt_u32_t          segment_id(agt::shared_handle handle)noexcept;
  AGT_forceinline agt_u32_t          page_id(agt::shared_handle handle) noexcept;
  AGT_forceinline agt_u32_t          page_offset(agt::shared_handle handle) noexcept;

  class _shared_handle {
  public:

    inline constexpr static agt_u64_t KindBits       = 3;
    inline constexpr static agt_u64_t PageIdBits     = 14;
    inline constexpr static agt_u64_t PageOffsetBits = 15;
    inline constexpr static agt_u64_t EpochBits      = 10;
    inline constexpr static agt_u64_t SegmentIdBits  = 22;

    [[nodiscard]] AGT_forceinline shared_handle_kind kind() const noexcept {
      return static_cast<shared_handle_kind>(m_kind);
    }
    [[nodiscard]] AGT_forceinline agt_u16_t   epoch() const noexcept {
      return m_epoch;
    }
    [[nodiscard]] AGT_forceinline agt_u32_t   segment_id() const noexcept {
      return m_segmentId;
    }
    [[nodiscard]] AGT_forceinline agt_u32_t   page_id() const noexcept {
      return m_pageId;
    }
    [[nodiscard]] AGT_forceinline agt_u32_t   page_offset() const noexcept {
      return m_pageOffset;
    }

    [[nodiscard]] AGT_forceinline agt::shared_handle as_handle() const noexcept {
      return static_cast<agt::shared_handle>(m_bits);
    }

    AGT_forceinline friend agt::shared_handle make_shared_handle(shared_handle_kind kind, agt_u32_t epoch, agt_u32_t segmentId, agt_u32_t pageId, agt_u32_t pageOffset) noexcept;
    AGT_forceinline friend shared_handle_kind kind(agt::shared_handle handle) noexcept;
    AGT_forceinline friend agt_u16_t          epoch(agt::shared_handle handle) noexcept;
    AGT_forceinline friend agt_u32_t          segment_id(agt::shared_handle handle)noexcept;
    AGT_forceinline friend agt_u32_t          page_id(agt::shared_handle handle) noexcept;
    AGT_forceinline friend agt_u32_t          page_offset(agt::shared_handle handle) noexcept;

  private:
    union {
      struct {
        agt_u64_t m_kind       : KindBits;
        agt_u64_t m_pageId     : PageIdBits;
        agt_u64_t m_pageOffset : PageOffsetBits;
        agt_u64_t m_epoch      : EpochBits;
        agt_u64_t m_segmentId  : SegmentIdBits;
      };
      agt_u64_t m_bits;
    };
  };

  AGT_forceinline agt::shared_handle make_shared_handle(shared_handle_kind kind, agt_u32_t epoch, agt_u32_t segmentId, agt_u32_t pageId, agt_u32_t pageOffset) noexcept {
    _shared_handle id;
    id.m_kind       = static_cast<agt_u32_t>(kind);
    id.m_epoch      = epoch;
    id.m_segmentId  = segmentId;
    id.m_pageId     = pageId;
    id.m_pageOffset = pageOffset;
    return id.as_handle();
  }

  AGT_forceinline shared_handle_kind kind(agt::shared_handle handle) noexcept {
    return ((_shared_handle&)handle).kind();
  }
  AGT_forceinline agt_u16_t          epoch(agt::shared_handle handle) noexcept {
    return ((_shared_handle&)handle).epoch();
  }
  AGT_forceinline agt_u32_t          segment_id(agt::shared_handle handle) noexcept {
    return ((_shared_handle&)handle).segment_id();
  }
  AGT_forceinline agt_u32_t          page_id(agt::shared_handle handle) noexcept {
    return ((_shared_handle&)handle).page_id();
  }
  AGT_forceinline agt_u32_t          page_offset(agt::shared_handle handle) noexcept {
    return ((_shared_handle&)handle).page_offset();
  }



  inline constexpr size_t LocalCellSize      = 32;
  inline constexpr size_t LocalCellAlignment = 32;

  enum PageFlags : agt_u32_t {
    page_is_shared = 0x1,
    page_is_active = 0x2
  };
  enum CellFlags : agt_u16_t {
    cell_object_is_private = 0x1,
    cell_object_is_shared  = 0x2,
    cell_is_shared_export  = 0x4,
    cell_is_shared_import  = 0x8
  };

  class Id final {
  public:

    enum ObjectKind {
      eInvalid,
      eObjectInstance,
      eHandle,
      eInternal
    };

    inline constexpr static agt_u64_t SharedFlagBits = 1;
    inline constexpr static agt_u64_t KindBits       = 2;
    inline constexpr static agt_u64_t PageIdBits     = 14;
    inline constexpr static agt_u64_t PageOffsetBits = 15;
    inline constexpr static agt_u64_t EpochBits      = 10;
    inline constexpr static agt_u64_t ProcessIdBits  = 22;
    inline constexpr static agt_u64_t SegmentIdBits  = ProcessIdBits;

    [[nodiscard]] AGT_forceinline ObjectKind  getKind() const noexcept {
      return static_cast<ObjectKind>(local.kind);
    }
    [[nodiscard]] AGT_forceinline agt_u16_t   getEpoch() const noexcept {
      return local.epoch;
    }
    [[nodiscard]] AGT_forceinline agt_u32_t   getProcessId() const noexcept {
      return local.processId;
    }
    [[nodiscard]] AGT_forceinline agt_u32_t   getSegmentId() const noexcept {
      return shared.segmentId;
    }
    [[nodiscard]] AGT_forceinline bool        isShared() const noexcept {
      return local.isShared;
    }
    [[nodiscard]] AGT_forceinline agt_u32_t   getPageId() const noexcept {
      return local.pageId;
    }
    [[nodiscard]] AGT_forceinline agt_u32_t   getPageOffset() const noexcept {
      return local.pageOffset;
    }

    [[nodiscard]] AGT_forceinline agt_object_id_t toRaw() const noexcept {
      return bits;
    }


    AGT_forceinline static Id makeLocal(agt_u32_t epoch, agt_u32_t procId, agt_u32_t pageId, agt_u32_t pageOffset) noexcept {
      Id id;
      id.local.epoch      = epoch;
      id.local.processId  = procId;
      id.local.isShared   = 0;
      id.local.pageId     = pageId;
      id.local.pageOffset = pageOffset;
      return id;
    }
    AGT_forceinline static Id makeShared(agt_u32_t epoch, agt_u32_t segmentId, agt_u32_t pageId, agt_u32_t pageOffset) noexcept {
      Id id;
      id.shared.epoch      = epoch;
      id.shared.segmentId  = segmentId;
      id.shared.isShared   = 1;
      id.shared.pageId     = pageId;
      id.shared.pageOffset = pageOffset;
      return id;
    }

    AGT_forceinline static Id convert(agt_object_id_t id) noexcept {
      Id realId;
      realId.bits = id;
      return realId;
    }

    AGT_forceinline static Id invalid() noexcept {
      return convert(AGT_INVALID_OBJECT_ID);
    }


    AGT_forceinline friend bool operator==(const Id& a, const Id& b) noexcept {
      return a.bits == b.bits;
    }


  private:
    union {
      struct {
        agt_u64_t isShared   : SharedFlagBits;
        agt_u64_t kind       : KindBits;
        agt_u64_t pageId     : PageIdBits;
        agt_u64_t pageOffset : PageOffsetBits;
        agt_u64_t epoch      : EpochBits;
        agt_u64_t processId  : ProcessIdBits;
      } local;
      struct {
        agt_u64_t isShared   : SharedFlagBits;
        agt_u64_t kind       : KindBits;
        agt_u64_t pageId     : PageIdBits;
        agt_u64_t pageOffset : PageOffsetBits;
        agt_u64_t epoch      : EpochBits;
        agt_u64_t segmentId  : SegmentIdBits;
      } shared;
      agt_u64_t bits;
    };
  };

  enum class page_type {
    local_object_info
  };

  namespace ctx_impl {
    class borrow_list_base;

    inline constexpr size_t CellSize      = 32;
    inline constexpr size_t CellAlignment = 32;
  }

  class borrowable {
    borrowable* next;
    borrowable* prev;

    friend class ctx_impl::borrow_list_base;
  };

  namespace ctx_impl {

    struct page_header {
      agt_u32_t     id;
      agt_flags32_t flags;
    };

    struct alignas(CellAlignment) cell {
      cell*            nextFreeCell;
      agt_u16_t        flags;
      agt_u16_t        epoch;
      agt_u32_t        pageId;
      agt_u32_t        pageOffset;
      agt::object_type objectType;
      handle_header*   object;
    };

    struct alignas(AGT_PHYSICAL_PAGE_SIZE) page {
      simple_mutex_t writeLock;
      atomic_u32_t   refCount;
      agt_flags32_t  flags;
      cell*          nextFreeCell;
      agt_u32_t      freeCells;
      agt_u32_t      totalCells;
      cell           cells[];


      AGT_forceinline cell* allocCell(page* p) noexcept {
        AGT_assert( !full(p) );
        auto c = p->nextFreeCell;
        p->nextFreeCell = c->nextFreeCell;
        --p->freeCells;
        return c;
      }
      AGT_forceinline void  freeCell(page* p, cell* c) noexcept {
        AGT_assert( !empty(p) );
        c->nextFreeCell = std::exchange(p->nextFreeCell, c);
        ++c->epoch;
        ++p->freeCells;
      }

      AGT_forceinline bool empty(page* p) noexcept {
        return p->freeCells == p->totalCells;
      }
      AGT_forceinline bool full(page* p) noexcept {
        return p->freeCells == 0;
      }
    };


    class borrow_list_base {
    protected:

      borrowable head;

      borrow_list_base() {
        head.prev = &head;
        head.next = &head;
      }

      // insert element to after self in list
      static void doInsertAfter(borrowable* self, borrowable* element) noexcept {
        element->prev = self;
        element->next = self->next;
        self->next->prev = element;
        self->next = element;
      }
      // insert element to before self in list
      static void doInsertBefore(borrowable* self, borrowable* element) noexcept {
        element->prev = self->prev;
        element->next = self;
        self->prev->next = element;
        self->prev = element;
      }
      // move element to after self in list
      static void doMoveToAfter(borrowable* self, borrowable* element) noexcept {
        doRemove(self);
        doInsertAfter(self, element);
      }
      // move element to before self in list
      static void doMoveToBefore(borrowable* self, borrowable* element) noexcept {
        doRemove(self);
        doInsertBefore(self, element);
      }
      // remove self from list, self is in indeterminate state after call
      static void doRemove(borrowable* self) noexcept {
        self->next->prev = self->prev;
        self->prev->next = self->next;
      }
    };
  }

  template <typename T>
  class borrow_list : public ctx_impl::borrow_list_base {
  public:


  };


  class local_object_manager {



  public:

  };

  struct page_table_entry : borrowable {
    ctx_impl::page_header* page;
    page_type              type;
  };




  struct alignas(LocalCellAlignment) ObjectInfoCell {
    agt_u32_t        nextFreeCell;
    agt_u16_t        epoch;
    agt_u16_t        flags;

    agt_u32_t        thisIndex;
    agt_u32_t        pageId;
    agt_u32_t        pageOffset;

    agt::object_type objectType;

    handle_header*   object;
  };

  class alignas(AGT_PHYSICAL_PAGE_SIZE) Page {

    simple_mutex_t writeLock;
    atomic_u32_t   refCount;
    agt_flags32_t  flags;
    agt_u32_t      freeCells;
    agt_u32_t      nextFreeCell;
    agt_u32_t      totalCells;
    ObjectInfoCell cells[];

  public:

    Page(size_t pageSize, agt_flags32_t pageFlags = 0) noexcept
        : freeCells((pageSize / LocalCellSize) - 1),
          nextFreeCell(1),
          totalCells(freeCells),
          flags(pageFlags)
    {
      agt_u32_t index = 0;
      agt_u16_t cellFlags = (pageFlags & page_is_shared) ? (cell_object_is_shared | cell_is_shared_export) : 0;
      while (index < totalCells) {
        auto& c = cell(index);
        c.thisIndex = index;
        c.nextFreeCell = ++index;
        c.epoch = 0;
        c.flags = cellFlags;
      }
    }


    AGT_forceinline ObjectInfoCell& allocCell() noexcept {
      AGT_assert( !full() );
      auto& c = cell(nextFreeCell);
      nextFreeCell = c.nextFreeCell;
      --freeCells;
      return c;
    }
    AGT_forceinline void            freeCell(agt_u32_t index) noexcept {
      AGT_assert( !empty() );
      auto& c = cell(index);
      c.nextFreeCell = std::exchange(nextFreeCell, index);
      ++c.epoch;
      ++freeCells;
    }

    AGT_forceinline ObjectInfoCell& cell(agt_u32_t index) const noexcept {
      return const_cast<ObjectInfoCell&>(cells[index]);
    }

    AGT_forceinline bool empty() const noexcept {
      return freeCells == totalCells;
    }
    AGT_forceinline bool full() const noexcept {
      return freeCells == 0;
    }
  };

  struct ListNodeBase {
    ListNodeBase* next;
    ListNodeBase* prev;

    void insertAfter(ListNodeBase* node) noexcept {
      prev = node;
      next = node->next;
      node->prev->next = this;
      node->prev = this;
    }
    void insertBefore(ListNodeBase* node) noexcept {
      prev = node->prev;
      next = node;
      node->next->prev = this;
      node->next = this;
    }
    void moveToAfter(ListNodeBase* node) noexcept {
      remove();
      insertAfter(node);
    }
    void moveToBefore(ListNodeBase* node) noexcept {
      remove();
      insertBefore(node);
    }
    void remove() noexcept {
      next->prev = prev;
      prev->next = next;
    }
  };

  struct ListNode : ListNodeBase {
    Page* page;
  };

  class PageList {

    ListNodeBase base;
    size_t       entryCount;

  public:

    PageList() : base{ &base, &base }, entryCount(0){ }


    AGT_forceinline size_t   size()  const noexcept {
      return entryCount;
    }
    AGT_forceinline bool      empty() const noexcept {
      return entryCount == 0;
    }

    AGT_forceinline ListNode* front() const noexcept {
      AGT_assert(!empty());
      return static_cast<ListNode*>(base.next);
    }
    AGT_forceinline ListNode* back() const noexcept {
      AGT_assert(!empty());
      return static_cast<ListNode*>(base.prev);
    }

    AGT_forceinline void      pushFront(ListNode* node) noexcept {
      node->insertAfter(&base);
      ++entryCount;
    }
    AGT_forceinline ListNode* popFront() noexcept {
      ListNode* node = front();
      node->remove();
      --entryCount;
      return node;
    }
    AGT_forceinline void      pushBack(ListNode* node) noexcept {
      node->insertBefore(&base);
      ++entryCount;
    }
    AGT_forceinline ListNode* popBack() noexcept {
      ListNode* node = back();
      node->remove();
      --entryCount;
      return node;
    }

    AGT_forceinline void      moveToFront(ListNode* node) noexcept {
      node->moveToAfter(&base);
    }
    AGT_forceinline void      moveToBack(ListNode* node) noexcept {
      node->moveToBefore(&base);
    }

    AGT_forceinline ListNode* peek() const noexcept {
      return front();
    }
  };

  class SharedAllocator {

    struct FreeListEntry;

    struct Segment;

    struct SegmentHeader {
      size_t                          segmentSize;
      agt_u32_t                       allocCount;
      ipc::offset_ptr<FreeListEntry*> rootEntry;
    };

    struct FreeListEntry {
      ipc::offset_ptr<SegmentHeader*> thisSegment;
      size_t                          entrySize;
      ipc::offset_ptr<FreeListEntry*> rightEntry;
      ipc::offset_ptr<FreeListEntry*> leftEntry;
    };

    struct AllocationEntry {
      ipc::offset_ptr<SegmentHeader*> thisSegment;
      size_t                          allocationSize;
      char                            allocation[];
    };

    struct FreeList {


    };

    struct Segment {
      SegmentHeader* header;
      size_t         maxSize;
    };
  };

  class SharedPageMap {

    struct MapNode {
      agt_u32_t pageId;
      Page*     mappedPage;
    };

    MapNode*  pNodeTable;
    agt_u32_t tableSize;
    agt_u32_t bucketCount;
    agt_u32_t tombstoneCount;

  public:


  };


  static_assert(sizeof(ObjectInfoCell) == CellSize);
  static_assert(alignof(ObjectInfoCell) == CellAlignment);

  using SharedHandlePool = ObjectPool<SharedHandle, (0x1 << 14) / sizeof(SharedHandle)>;
}


/// Shared Systems

namespace {
  inline constexpr wchar_t SharedControlBlockName[] = L"agate-scb"; // NOTE: Might need to make a more complex name to avoid accidental clashes.

  inline constexpr size_t SharedControlBlockMinLength = AGT_VIRTUAL_PAGE_SIZE;

  inline constexpr size_t SharedControlBlockDefaultLength = SharedControlBlockMinLength;

  inline constexpr size_t SharedControlBlockAlignment = AGT_VIRTUAL_PAGE_SIZE;

  // Arbitrary, should probably change after profiling. Currently 64 MB.
  inline constexpr size_t SharedControlBlockVirtualSize = (0x1 << 26);

  inline constexpr size_t SharedControlBlockMaxNameLength = 64;

  inline constexpr size_t SharedSegmentMaxNameLength = 32;

  struct shared_segment {
    agt_u32_t id;
    agt_u32_t next;
    agt_u32_t prev;
    agt_u32_t refCount;
    size_t    size;
    size_t    blockSize;
    char      name[SharedSegmentMaxNameLength];
  };

  struct shared_registry_entry {

  };


  struct shared_cb {
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
  struct shared_registry {};
  struct shared_processes {
    size_t descriptorCount;
    size_t tableOffset;
    size_t tableBucketCount;
  };
  struct shared_allocator {};
  struct shared_segment_descriptor {
    void*                      handle;
    agt_u32_t                  refCount;
    size_t                     size;
    void*                      address;
    shared_segment_descriptor* parent;
  };
  struct shared_allocation_descriptor {
    agt::shared_handle         handle;
    size_t                     size;
    void*                      address;
    shared_segment_descriptor* segment;
  };
  struct process_descriptor {};

  class shared_allocation_lut {
    shared_allocation_descriptor** allocTable;
    shared_segment_descriptor**    segTable;

    agt_u32_t                      allocBucketCount;
    agt_u32_t                      allocEntryCount;
    agt_u32_t                      allocTombstoneCount;

    agt_u32_t                      segBucketCount;
    agt_u32_t                      segEntryCount;
    agt_u32_t                      segTombstoneCount;
  public:
    shared_allocation_lut() noexcept
        : {}
  };





}

extern "C" {

struct agt_ctx_st {
  agt_u32_t        contextId;
  agt_u32_t        processId;

  SharedPageMap    sharedPageMap;
  PageList         localFreeList;
  ListNode*        emptyLocalPage;

  size_t          localPageSize;
  size_t          localEntryCount;




  shared_cb*            sharedCb;
  process_descriptor*   self;
  shared_registry*      sharedRegistry;
  shared_processes*     sharedProcesses;
  shared_allocator*     sharedAllocator;
  shared_allocation_lut sharedAllocations;

  /*SharedHandlePool handlePool;

  IpcBlock         ipcBlock;*/

};


}


namespace {

  std::mutex g_cleanupMtx;


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

  void doInitSharedCb(shared_cb* cb) {

  }

  void initializeSharedCb(shared_cb* cb) {
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

  agt_status_t initSharedAllocationLookupTable(agt_ctx_t ctx) noexcept {

  }

  agt_status_t sharedControlBlockInit(agt_ctx_t ctx) noexcept {

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


    ctx->sharedCb = cb;
    ctx->self     = nullptr; // TODO: Implement
    ctx->sharedRegistry = static_cast<shared_registry*>(static_cast<void*>((static_cast<char*>(cbMapping.value()) + cb->registryOffset)));
    ctx->sharedProcesses = static_cast<shared_processes*>(static_cast<void*>((static_cast<char*>(cbMapping.value()) + cb->processesOffset)));
    ctx->sharedAllocator = static_cast<shared_allocator*>(static_cast<void*>((static_cast<char*>(cbMapping.value()) + cb->allocatorOffset)));

    if (auto status = initSharedAllocationLookupTable(ctx))
      return status;

    cbMapping.setOwnership(false);
    baseAddress.setOwnership(false);
    controlBlockAddr.setOwnership(false);

    return AGT_SUCCESS;
  }


  void         sharedControlBlockClose(agt_ctx_t ctx) noexcept {}

}

/** ========= [ Creation/Destruction ] ========= */
agt_status_t agt::createCtx(agt_ctx_t& pCtx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
void         agt::destroyCtx(agt_ctx_t ctx) noexcept { }


/** ========= [ Memory Management ] ========= */
void*     agt::ctxLocalAlloc(agt_ctx_t ctx, size_t size, size_t alignment) noexcept {
  return _aligned_malloc(size, alignment);
}
void      agt::ctxLocalFree(agt_ctx_t ctx, void* memory, size_t size, size_t alignment) noexcept {
  _aligned_free(memory);
}



/** ========= [ Object Name Management ] ========= */

/**
 * @section Object Name Management Examples
 *
 * Intended usage of the name management functions is as follows:
 * @code
 *      agt_status_t createLocalObject(Object*& object, agt_ctx_t ctx, ...) {
 *          name_token nameToken;
 *          agt_status_t status;
 *          if ((status = ctxClaimLocalName(ctx, name, nameLength, nameToken)))
 *              return status;
 *
 *          ... Allocate and initialize object ...
 *
 *          if (status != AGT_SUCCESS) {
 *              ... Cleanup ...
 *              ctxReleaseLocalName(ctx, nameToken);
 *              return status;
 *          }
 *
 *          ctxBindName(ctx, nameToken, object);
 *
 *          return AGT_SUCCESS;
 *      }
 *
 *      agt_status_t createSharedObject(Object*& object, agt_ctx_t ctx, ...) {
*          name_token nameToken;
*          agt_status_t status;
*          if ((status = ctxClaimSharedName(ctx, name, nameLength, nameToken)))
*              return status;
*
*          ... Allocate and initialize object ...
*
*          if (status != AGT_SUCCESS) {
*              ... Cleanup ...
*              ctxReleaseSharedName(ctx, nameToken);
*              return status;
*          }
*
*          ctxBindName(ctx, nameToken, object);
*
*          return AGT_SUCCESS;
*      }
 *      @endcode
 *
 * */

/**
 * @fn ctxClaimLocalName
 * @brief Atomically acquires the specified name for use, and returns a token referring to the acquired name.
 * @returns AGT_SUCCESS if successful;
 *          AGT_ERROR_NAME_ALREADY_IN_USE if the specified name is already in use;
 *          AGT_ERROR_NAME_TOO_LONG if the specified name is greater than the maximum name length for this context;
 *          AGT_ERROR_BAD_ENCODING_IN_NAME if the specified name is not a valid string under the context's encoding;
 * */
agt_status_t     agt::ctxClaimLocalName(agt_ctx_t ctx, const char* pName, size_t nameLength, name_token& token) noexcept {
  // 1: if pName is null, set token to the "null token" and return AGT_SUCCESS
  // 2: if nameLength is zero, set nameLength to strlen(pName)
  // 3: checks if the desired nameLength is less than the maximum limit (default is 256). If not, return AGT_ERROR_NAME_TO_LONG
  // 4: checks if the desired name has valid encoding *if* the context has been set up to validate encoding. If not, return AGT_ERROR_BAD_ENCODING_IN_NAME
  // 5: checks if the desired name begins with a "shared prefix", if not return ... some error lmao
  // 6: checks if the desired name is already in use, and if not, claim it so that other callers can't.
  // 7: set token to the token corresponding to the acquired name block
}
/**
 *
 * */
void             agt::ctxReleaseLocalName(agt_ctx_t ctx, name_token nameToken) noexcept {
  // 1: if nameToken is "null", do nothing and return
  // 2: get the name block corresponding to nameToken
  // 3: release the name block
}
/**
 *
 * */
agt_status_t     agt::ctxClaimSharedName(agt_ctx_t ctx, const char* pName, size_t nameLength, name_token& token) noexcept {
  // TODO: Resolve: what should the local "shared-prefix" be? A prefix is needed on local entries to
  //       avoid name clashes between local and shared objects (as the possibility of this occuring
  //       would cause serious issue when importing shared objects).
  //       Should it be "{shared}" or something of the like? Or "{<shared-ctx-id>}"?

  // 1: if pName is null, set token to the "null token" and return AGT_SUCCESS
  // 2: if nameLength is zero, set nameLength to strlen(pName)
  // 3: checks if the desired nameLength is less than the maximum limit (default is 256). If not, return AGT_ERROR_NAME_TO_LONG
  // 4: checks if the desired name has valid encoding *if* the context has been set up to validate encoding. If not, return AGT_ERROR_BAD_ENCODING_IN_NAME
  // 5: checks if the desired name is already in use in the shared space, and if not, claim it in the shared space.
  // 6: claim the name locally (guaranteed it's available, given the shared prefix)
  // 7: set token to the token corresponding to the acquired name block
}
void             agt::ctxReleaseSharedName(agt_ctx_t ctx, name_token nameToken) noexcept {
  // 1: if nameToken is "null", do nothing and return
  // 2: get the name block corresponding to nameToken
  // 3: release the local name block
  // 4: release the shared name block (must be done in this order to maintain guarantee of local availablity in ctxClaimSharedName)
}
void             agt::ctxBindName(agt_ctx_t ctx, name_token nameToken, HandleHeader* handle) noexcept {
  // TODO: workout algorithm here. Probably dependent on choice of implementation of AgtContext_st
  // 1: idk
}





