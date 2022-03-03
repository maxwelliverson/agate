//
// Created by maxwe on 2021-11-25.
//

#include "context.hpp"
#include "core/objects.hpp"
#include "support/pool.hpp"
#include "ipc_block.hpp"

#include <cstdlib>
#include <memory>
#include <mutex>

using namespace Agt;

namespace {
  inline constexpr static jem_size_t CellSize      = 32;
  inline constexpr static jem_size_t CellAlignment = 32;

  enum PageFlags : AgtUInt32 {
    page_is_shared = 0x1,
    page_is_active = 0x2
  };
  enum CellFlags : jem_u16_t {
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

    inline constexpr static AgtUInt64 SharedFlagBits = 1;
    inline constexpr static AgtUInt64 KindBits       = 2;
    inline constexpr static AgtUInt64 PageIdBits     = 14;
    inline constexpr static AgtUInt64 PageOffsetBits = 15;
    inline constexpr static AgtUInt64 EpochBits      = 10;
    inline constexpr static AgtUInt64 ProcessIdBits  = 22;
    inline constexpr static AgtUInt64 SegmentIdBits  = ProcessIdBits;

    AGT_forceinline ObjectKind  getKind() const noexcept {
      return static_cast<ObjectKind>(local.kind);
    }
    AGT_forceinline AgtUInt16   getEpoch() const noexcept {
      return local.epoch;
    }
    AGT_forceinline AgtUInt32   getProcessId() const noexcept {
      return local.processId;
    }
    AGT_forceinline AgtUInt32   getSegmentId() const noexcept {
      return shared.segmentId;
    }
    AGT_forceinline bool        isShared() const noexcept {
      return local.isShared;
    }
    AGT_forceinline AgtUInt32   getPageId() const noexcept {
      return local.pageId;
    }
    AGT_forceinline AgtUInt32   getPageOffset() const noexcept {
      return local.pageOffset;
    }

    AGT_forceinline AgtObjectId toRaw() const noexcept {
      return bits;
    }


    AGT_forceinline static Id makeLocal(AgtUInt32 epoch, AgtUInt32 procId, AgtUInt32 pageId, AgtUInt32 pageOffset) noexcept {
      Id id;
      id.local.epoch      = epoch;
      id.local.processId  = procId;
      id.local.isShared   = 0;
      id.local.pageId     = pageId;
      id.local.pageOffset = pageOffset;
      return id;
    }
    AGT_forceinline static Id makeShared(AgtUInt32 epoch, AgtUInt32 segmentId, AgtUInt32 pageId, AgtUInt32 pageOffset) noexcept {
      Id id;
      id.shared.epoch      = epoch;
      id.shared.segmentId  = segmentId;
      id.shared.isShared   = 1;
      id.shared.pageId     = pageId;
      id.shared.pageOffset = pageOffset;
      return id;
    }

    AGT_forceinline static Id convert(AgtObjectId id) noexcept {
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
        AgtUInt64 isShared   : SharedFlagBits;
        AgtUInt64 kind       : KindBits;
        AgtUInt64 pageId     : PageIdBits;
        AgtUInt64 pageOffset : PageOffsetBits;
        AgtUInt64 epoch      : EpochBits;
        AgtUInt64 processId  : ProcessIdBits;
      } local;
      struct {
        AgtUInt64 isShared   : SharedFlagBits;
        AgtUInt64 kind       : KindBits;
        AgtUInt64 pageId     : PageIdBits;
        AgtUInt64 pageOffset : PageOffsetBits;
        AgtUInt64 epoch      : EpochBits;
        AgtUInt64 segmentId  : SegmentIdBits;
      } shared;
      AgtUInt64 bits;
    };
  };

  struct alignas(CellAlignment) ObjectInfoCell {
    AgtUInt32     nextFreeCell;
    jem_u16_t     epoch;
    jem_u16_t     flags;

    AgtUInt32     thisIndex;
    AgtUInt32     pageId;
    AgtUInt32     pageOffset;

    AgtHandleType objectType;

    ObjectHeader* object;
  };

  class alignas(AGT_PHYSICAL_PAGE_SIZE) Page {

    simple_mutex_t writeLock;
    atomic_u32_t   refCount;
    jem_flags32_t  flags;
    AgtUInt32      freeCells;
    AgtUInt32      nextFreeCell;
    AgtUInt32      totalCells;
    ObjectInfoCell cells[];

  public:

    Page(jem_size_t pageSize, jem_flags32_t pageFlags = 0) noexcept
        : freeCells((pageSize / CellSize) - 1),
          nextFreeCell(1),
          totalCells(freeCells),
          flags(pageFlags)
    {
      AgtUInt32 index = 0;
      jem_u16_t cellFlags = (pageFlags & page_is_shared) ? (cell_object_is_shared | cell_is_shared_export) : 0;
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
    AGT_forceinline void            freeCell(AgtUInt32 index) noexcept {
      AGT_assert( !empty() );
      auto& c = cell(index);
      c.nextFreeCell = std::exchange(nextFreeCell, index);
      ++c.epoch;
      ++freeCells;
    }

    AGT_forceinline ObjectInfoCell& cell(AgtUInt32 index) const noexcept {
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
    AgtSize      entryCount;

  public:

    PageList() : base{ &base, &base }, entryCount(0){ }


    AGT_forceinline AgtSize   size()  const noexcept {
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
      AgtUInt32                       allocCount;
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
      AgtUInt32 pageId;
      Page*     mappedPage;
    };

    MapNode*  pNodeTable;
    AgtUInt32 tableSize;
    AgtUInt32 bucketCount;
    AgtUInt32 tombstoneCount;

  public:


  };


  static_assert(sizeof(ObjectInfoCell) == CellSize);
  static_assert(alignof(ObjectInfoCell) == CellAlignment);

  using SharedHandlePool = ObjectPool<SharedHandle, (0x1 << 14) / sizeof(SharedHandle)>;
}

extern "C" {

struct AgtSharedContext_st {

};

struct AgtContext_st {

  AgtUInt32        processId;

  SharedPageMap    sharedPageMap;
  PageList         localFreeList;
  ListNode*        emptyLocalPage;

  AgtSize          localPageSize;
  AgtSize          localEntryCount;

  SharedHandlePool handlePool;

  IpcBlock         ipcBlock;

};


}


namespace {

  std::mutex g_cleanupMtx;



}

/** ========= [ Creation/Destruction ] ========= */
AgtStatus Agt::createCtx(AgtContext& pCtx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
void      Agt::destroyCtx(AgtContext ctx) noexcept { }


/** ========= [ Memory Management ] ========= */
void*     Agt::ctxLocalAlloc(AgtContext ctx, AgtSize size, AgtSize alignment) noexcept {
  return _aligned_malloc(size, alignment);
}
void      Agt::ctxLocalFree(AgtContext ctx, void* memory, AgtSize size, AgtSize alignment) noexcept {
  _aligned_free(memory);
}



/** ========= [ Object Name Management ] ========= */

/**
 * @section Object Name Management Examples
 *
 * Intended usage of the name management functions is as follows:
 *
 *      AgtStatus createLocalObject(Object*& object, AgtContext ctx, ...) {
 *          NameToken nameToken;
 *          AgtStatus status;
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
 *      AgtStatus createSharedObject(Object*& object, AgtContext ctx, ...) {
*          NameToken nameToken;
*          AgtStatus status;
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
 *
 * */

using ay = struct lmao;
/**
 * @fn ctxClaimLocalName
 * @brief Atomically acquires the specified name for use, and returns a token referring to the acquired name.
 * @returns AGT_SUCCESS if successful;
 *          AGT_ERROR_NAME_ALREADY_IN_USE if the specified name is already in use;
 *          AGT_ERROR_NAME_TOO_LONG if the specified name is greater than the maximum name length for this context;
 *          AGT_ERROR_BAD_ENCODING_IN_NAME if the specified name is not a valid string under the context's encoding;
 * */
AgtStatus     Agt::ctxClaimLocalName(AgtContext ctx, const char* pName, AgtSize nameLength, NameToken& token) noexcept {

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
void          Agt::ctxReleaseLocalName(AgtContext ctx, NameToken nameToken) noexcept {
  // 1: if nameToken is "null", do nothing and return
  // 2: get the name block corresponding to nameToken
  // 3: release the name block
}
/**
 *
 * */
AgtStatus     Agt::ctxClaimSharedName(AgtContext ctx, const char* pName, AgtSize nameLength, NameToken& token) noexcept {
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
void          Agt::ctxReleaseSharedName(AgtContext ctx, NameToken nameToken) noexcept {
  // 1: if nameToken is "null", do nothing and return
  // 2: get the name block corresponding to nameToken
  // 3: release the local name block
  // 4: release the shared name block (must be done in this order to maintain guarantee of local availablity in ctxClaimSharedName)
}
void          Agt::ctxBindName(AgtContext ctx, NameToken nameToken, HandleHeader* handle) noexcept {
  // TODO: workout algorithm here. Probably dependent on choice of implementation of AgtContext_st
  // 1: idk
}





