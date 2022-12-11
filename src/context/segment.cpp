//
// Created by maxwe on 2022-03-08.
//

#include "segment.hpp"


#include <cstdlib>
#include <cstring>
#include <bit>

namespace {

  inline constexpr agt_u32_t DefaultBucketCount = 16;
  inline size_t getMinBucketToReserveForEntries(size_t entries) noexcept {
    if ( entries == 0 )
      return DefaultBucketCount;
    return std::bit_ceil(entries * 4 / 3 + 1);
  }



  struct shared_seg_descriptor {
    agt_u32_t          importCount;
    agt::seg_type      type;
    agt_flags32_t      flags;
    agt::shared_handle allocId;
    size_t             cellSize;
    size_t             cellCount;
    char               name[24];
  };




  // Each "page" ID is associated with an entry in the page table, and each entry has a state
  enum class seg_state {
    invalid,    // indicates an unused bucket
    abandonned, // the specified segment has been deallocated, no associated descriptor exists any longer
    deprecated, // the specified segment is not currently in use, but has not yet been deallocated
    active,     // the specified segment is currently in use
  };




  inline agt_u32_t generateSegmentId(agt::seg_manager* manager) noexcept {
    return agt::rngNext(&manager->idGenerator) >> (32 - agt::SegmentIdBits);
  }



}

struct agt::seg_descriptor : agt::seg_descriptor_header {
  void*                  address;
  shared_seg_descriptor* sharedDescriptor;
  agt_u32_t              refCount;
  agt_u32_t              id;
  seg_type               type;
  agt_flags32_t          flags;
  size_t                 size;
  // 8 free bytes?
};


struct agt::seg_table_entry {
  agt_u32_t       id;
  seg_state       state;
  seg_descriptor* descriptor;
};


static void  deleteSegDescriptor(agt::seg_manager* st, agt::seg_descriptor* seg) noexcept {

}

static void  releaseSegDescriptor(agt::seg_manager* st, agt::seg_descriptor* seg) noexcept {
  if (--seg->refCount == 0)
    deleteSegDescriptor(st, seg);
}


static void  segTableInit(agt::seg_table* st, size_t tableBuckets) noexcept {
  size_t bucketCount = getMinBucketToReserveForEntries(tableBuckets);
  st->table = (agt::seg_table_entry*) malloc(bucketCount * sizeof(agt::seg_table_entry));
  st->bucketCount = bucketCount;
  st->abandonnedCount = 0;
  st->entryCount = 0;

  for (size_t i = 0; i < bucketCount; ++i) {
    auto& bucket      = st->table[i];
    bucket.state      = seg_state::invalid;
    bucket.id         = 0;
    bucket.descriptor = nullptr;
  }
}

static void  segTableRehash(agt::seg_table* st) noexcept {
  size_t newSize;
  auto*  table = st->table;

  // If the hash table is now more than 3/4 full, or if fewer than 1/8 of
  // the buckets are empty (meaning that many are filled with tombstones),
  // grow/rehash the table.
  if (st->entryCount * 4 > st->bucketCount * 3) [[unlikely]]
    newSize = st->bucketCount * 2;
  else if (st->bucketCount - (st->entryCount + st->abandonnedCount) <= st->bucketCount / 8) [[unlikely]]
    newSize = st->bucketCount;
  else
    return;


  auto newTable = (agt::seg_table_entry*) malloc(newSize * sizeof(agt::seg_table_entry));
  std::memset(newTable, 0, newSize * sizeof(agt::seg_table_entry));

  // Rehash all the items into their new buckets.  Luckily :) we already have
  // the hash values available, so we don't have to rehash any strings.
  for (size_t i = 0, e = st->bucketCount; i != e; ++i) {
    auto& bucket = table[i];
    if (bucket.state == seg_state::active || bucket.state == seg_state::deprecated) {
      size_t newBucketNo = bucket.id & (newSize - 1);
      if (newTable[newBucketNo].state == seg_state::invalid) [[likely]] {
        newTable[newBucketNo].state = bucket.state;
        newTable[newBucketNo].id = bucket.id;
        newTable[newBucketNo].descriptor = bucket.descriptor;
        continue;
      }

      size_t probeSize = 1;
      do {
        newBucketNo = (newBucketNo + probeSize++) & (newSize - 1);
      } while (newTable[newBucketNo].state != seg_state::invalid);

      newTable[newBucketNo].state = bucket.state;
      newTable[newBucketNo].id = bucket.id;
      newTable[newBucketNo].descriptor = bucket.descriptor;
    }
  }

  free(st->table);

  st->table = newTable;
  st->bucketCount = newSize;
  st->abandonnedCount = 0;
}

static agt::seg_descriptor*  segTableLookup(agt::seg_table* st, agt_u32_t id) noexcept {
  auto *table = st->table;
  size_t tableSize = st->bucketCount;
  size_t bucketNo = id & (tableSize - 1);

  unsigned probeAmt = 1;

  while (true) {
    const auto &bucketItem = table[bucketNo];

    switch (bucketItem.state) {
      case seg_state::invalid:
        return nullptr;
      case seg_state::abandonned:
      case seg_state::deprecated:
        if (bucketItem.id == id)
          return nullptr;
      case seg_state::active:
        if (bucketItem.id == id)
          return bucketItem.descriptor;
        break;
        AGT_no_default;
    }

    // Okay, we didn't find the item.  Probe to the next bucket.
    bucketNo = (bucketNo + probeAmt) & (tableSize - 1);

    // Use quadratic probing, it has fewer clumping artifacts than linear
    // probing and has good cache behavior in the common case.
    ++probeAmt;
  }
}

static agt::seg_table_entry* segTableGetBucket(agt::seg_table* st, agt_u32_t id) noexcept {
  auto *table = st->table;
  size_t tableSize = st->bucketCount;
  size_t bucketNo = id & (tableSize - 1);

  unsigned probeAmt = 1;

  while (true) {
    auto* bucket = table + bucketNo;

    if (bucket->state == seg_state::invalid)
      return nullptr;

    if (bucket->id == id)
      return bucket;

    bucketNo = (bucketNo + probeAmt) & (tableSize - 1);
    ++probeAmt;
  }
}

static bool                  segTableInsert(agt::seg_table* st, agt::seg_descriptor* segment) noexcept {
  agt_u32_t id = segment->id;
  auto *table = st->table;
  size_t tableSize = st->bucketCount;
  size_t bucketNo = id & (tableSize - 1);

  unsigned probeAmt = 1;

  while (true) {
    auto &bucketItem = table[bucketNo];

    switch (bucketItem.state) {
      case seg_state::abandonned:
        st->abandonnedCount -= 1;
      case seg_state::invalid:
        bucketItem.state = seg_state::active;
        bucketItem.id = id;
        bucketItem.descriptor = segment;
        ++st->entryCount;
        AGT_assert(st->entryCount + st->abandonnedCount <= st->bucketCount);
        segTableRehash(st);
        return true;

      case seg_state::deprecated:
      case seg_state::active:
        if (bucketItem.id == id)
          return false;
        break;
        AGT_no_default;
    }

    // Okay, we didn't find the item.  Probe to the next bucket.
    bucketNo = (bucketNo + probeAmt) & (tableSize - 1);

    // Use quadratic probing, it has fewer clumping artifacts than linear
    // probing and has good cache behavior in the common case.
    ++probeAmt;
  }
}

static void                  segTableDestroy(agt::seg_manager* sm) noexcept {
  auto* st = &sm->table;
  auto* table = st->table;

  for (size_t i = 0, e = st->bucketCount; i != e; ++i) {
    auto& bucket = table[i];
    if (bucket.state == seg_state::deprecated || bucket.state == seg_state::active)
      releaseSegDescriptor(sm, table->descriptor);
  }

  free(table);
  st->table = nullptr;
}


static agt::seg_descriptor*  newSegDescriptor(agt::seg_manager* sm) noexcept {

  auto descriptor = new agt::seg_descriptor;

  do {
    descriptor->id = generateSegmentId(sm);
  } while (!segTableInsert(&sm->table, descriptor));

  return descriptor;
}




void         agt::segManagerInit(seg_manager* sm, size_t tableBuckets, rng_seeder* seeder) noexcept {
  segTableInit(&sm->table, tableBuckets);
  initRng(&sm->idGenerator, seeder);
  // TODO: Initialize the rest :)

}

agt_status_t agt::segManagerLookup(seg_manager* sm, agt_u32_t id, seg_descriptor** pDescriptor) noexcept {

}

agt_status_t agt::segManagerAlloc(seg_manager* sm, seg_type type, size_t size, seg_descriptor** pDescriptor) noexcept {

}

void         agt::segManagerDestroy(seg_manager *sm) noexcept {
  segTableDestroy(sm);
}

