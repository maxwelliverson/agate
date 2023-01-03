//
// Created by maxwe on 2022-08-07.
//

#ifndef AGATE_SUPPORT_MAP_HPP
#define AGATE_SUPPORT_MAP_HPP

#include "allocator.hpp"

#include <llvm/ADT/DenseMap.h>
#include "epoch_tracker.hpp"

#include <bit>
#include <concepts>

namespace agt {

  template <typename T>
  struct map_key_info;
  //static inline T get_empty_key();
  //static inline T get_tombstone_key();
  //static unsigned get_hash_value(const T &Val);
  //static bool     is_equal(const T &LHS, const T &RHS);

  template <typename T>
  struct map_key_info<T*> {
    static inline T* get_empty_key()     noexcept {
      return nullptr;
    }
    static inline T* get_tombstone_key() noexcept {
      return reinterpret_cast<T*>(static_cast<uintptr_t>(-1));
    }
    static unsigned  get_hash_value(const T* val) noexcept {
      return (unsigned((uintptr_t)val) >> 4) ^
             (unsigned((uintptr_t)val) >> 9);
    }
    static bool      is_equal(const T* LHS, const T* RHS) noexcept {
      return LHS == RHS;
    }
  };

  namespace impl {
    struct no_value{};

    template <typename Key, typename Val>
    struct bucket;
  }



  template <typename KeyT,
           typename  ValT,
           typename  InfoT = map_key_info<KeyT>,
           typename  Bucket   = impl::bucket<KeyT, ValT>,
           bool      IsConst  = false>
  class map_iterator;

  namespace impl {

    template <std::unsigned_integral I>
    inline static I nextPowerOf2(I i) noexcept {
      return std::bit_ceil(i + 1);
    }

    template <std::unsigned_integral I>
    inline unsigned log2Ceil(I i) {
      return std::numeric_limits<I>::digits - std::countl_zero(i - 1);
    }

    template <typename T>
    struct add_const_past_pointer;
    
    template <typename T> requires std::is_pointer_v<T>
    struct add_const_past_pointer<T> {
      using type = std::add_pointer_t<std::add_const_t<std::remove_pointer_t<T>>>;
    };
    
    template <typename T>
    struct const_pointer_or_const_ref {
      using type = const T&;
    };
    
    template <typename T> requires std::is_pointer_v<T>
    struct const_pointer_or_const_ref<T> {
      using type = typename add_const_past_pointer<T>::type;
    };


    template <typename Key, typename Val>
    struct bucket {
      Key key;
      Val value;

      Key&       getFirst()        noexcept { return key; }
      const Key& getFirst()  const noexcept { return key; }
      Val&       getSecond()       noexcept { return value; }
      const Val& getSecond() const noexcept { return value; }
    };

    template <typename Key>
    struct bucket<Key, no_value> : no_value {
      Key key;

      Key&            getFirst()        noexcept { return key; }
      const Key&      getFirst()  const noexcept { return key; }
      no_value&       getSecond()       noexcept { return *this; }
      const no_value& getSecond() const noexcept { return *this; }
    };
    
    template <typename Derived,
              typename KeyT,
              typename ValT,
              typename InfoT,
              typename BucketT>
    class basic_map : public debug_epoch_base {
      template <typename T>
      using const_arg_type_t = typename const_pointer_or_const_ref<T>::type;
    public:
      using size_type   = uint32_t;
      using key_type    = KeyT;
      using mapped_type = ValT;
      using value_type  = BucketT;

      using iterator       = map_iterator<KeyT, ValT, InfoT, BucketT>;
      using const_iterator = map_iterator<KeyT, ValT, InfoT, BucketT, true>;


      inline iterator       begin()       noexcept {
        // When the map is empty, avoid the overhead of advancing/retreating past
        // empty buckets.
        if (empty())
          return end();
        if (shouldReverseIterate<KeyT>())
          return makeIterator(getBucketsEnd() - 1, getBuckets(), *this);
        return makeIterator(getBuckets(), getBucketsEnd(), *this);
      }
      inline iterator       end()         noexcept {
        return makeIterator(getBucketsEnd(), getBucketsEnd(), *this, true);
      }
      inline const_iterator begin() const noexcept {
        if (empty())
          return end();
        if (shouldReverseIterate<KeyT>())
          return makeConstIterator(getBucketsEnd() - 1, getBuckets(), *this);
        return makeConstIterator(getBuckets(), getBucketsEnd(), *this);
      }
      inline const_iterator end()   const noexcept {
        return makeConstIterator(getBucketsEnd(), getBucketsEnd(), *this, true);
      }

      AGT_nodiscard bool     empty() const noexcept {
        return getNumEntries() == 0;
      }
      AGT_nodiscard unsigned size()  const noexcept { return getNumEntries(); }
      
      void reserve(size_type entryCount) noexcept {
        auto NumBuckets = getMinBucketToReserveForEntries(entryCount);
        increment_epoch();
        if (NumBuckets > getNumBuckets())
          grow(NumBuckets);
      }
      
      void clear() noexcept {
        increment_epoch();
        if (getNumEntries() == 0 && getNumTombstones() == 0) return;

        // If the capacity of the array is huge, and the # elements used is small,
        // shrink the array.
        if (getNumEntries() * 4 < getNumBuckets() && getNumBuckets() > 64) {
          shrink_and_clear();
          return;
        }

        const KeyT EmptyKey = getEmptyKey(), TombstoneKey = getTombstoneKey();
        if (std::is_trivially_destructible<ValT>::value) {
          // Use a simpler loop when values don't need destruction.
          for (BucketT *P = getBuckets(), *E = getBucketsEnd(); P != E; ++P)
            P->getFirst() = EmptyKey;
        } else {
          unsigned NumEntries = getNumEntries();
          for (BucketT *P = getBuckets(), *E = getBucketsEnd(); P != E; ++P) {
            if (!InfoT::isEqual(P->getFirst(), EmptyKey)) {
              if (!InfoT::isEqual(P->getFirst(), TombstoneKey)) {
                P->getSecond().~ValT();
                --NumEntries;
              }
              P->getFirst() = EmptyKey;
            }
          }
          assert(NumEntries == 0 && "Node count imbalance!");
        }
        setNumEntries(0);
        setNumTombstones(0);
      }

      /// Return 1 if the specified key is in the map, 0 otherwise.
      AGT_nodiscard size_type count(const_arg_type_t<KeyT> Val) const noexcept {
        const BucketT *TheBucket;
        return lookup_bucket_for(Val, TheBucket) ? 1 : 0;
      }

      iterator       find(const_arg_type_t<KeyT> Val) noexcept {
        BucketT *TheBucket;
        if (lookup_bucket_for(Val, TheBucket))
          return makeIterator(TheBucket,
                              shouldReverseIterate<KeyT>() ? getBuckets()
                                                           : getBucketsEnd(),
                              *this, true);
        return end();
      }
      const_iterator find(const_arg_type_t<KeyT> Val) const noexcept {
        const BucketT *TheBucket;
        if (lookup_bucket_for(Val, TheBucket))
          return makeConstIterator(TheBucket,
                                   shouldReverseIterate<KeyT>() ? getBuckets()
                                                                : getBucketsEnd(),
                                   *this, true);
        return end();
      }

      /// Alternate version of find() which allows a different, and possibly
      /// less expensive, key type.
      /// The DenseMapInfo is responsible for supplying methods
      /// getHashValue(LookupKeyT) and isEqual(LookupKeyT, KeyT) for each key
      /// type used.
      template<class LookupKeyT>
      iterator find_as(const LookupKeyT &Val) noexcept {
        BucketT *TheBucket;
        if (lookup_bucket_for(Val, TheBucket))
          return makeIterator(TheBucket,
                              shouldReverseIterate<KeyT>() ? getBuckets()
                                                           : getBucketsEnd(),
                              *this, true);
        return end();
      }
      template<class LookupKeyT>
      const_iterator find_as(const LookupKeyT &Val) const noexcept {
        const BucketT *TheBucket;
        if (lookup_bucket_for(Val, TheBucket))
          return makeConstIterator(TheBucket,
                                   shouldReverseIterate<KeyT>() ? getBuckets()
                                                                : getBucketsEnd(),
                                   *this, true);
        return end();
      }

      /// lookup - Return the entry for the specified key, or a default
      /// constructed value if no such entry exists.
      ValT lookup(const_arg_type_t<KeyT> Val) const noexcept {
        const BucketT *TheBucket;
        if (lookup_bucket_for(Val, TheBucket))
          return TheBucket->getSecond();
        return ValT();
      }
      
      
      // Inserts key,value pair into the map if the key isn't already in the map.
      // If the key is already in the map, it returns false and doesn't update the
      // value.
      std::pair<iterator, bool> insert(const std::pair<KeyT, ValT> &KV) noexcept {
        return try_emplace(KV.first, KV.second);
      }
      
      // Inserts key,value pair into the map if the key isn't already in the map.
      // If the key is already in the map, it returns false and doesn't update the
      // value.
      std::pair<iterator, bool> insert(std::pair<KeyT, ValT> &&KV) noexcept {
        return try_emplace(std::move(KV.first), std::move(KV.second));
      }

      // Inserts key,value pair into the map if the key isn't already in the map.
      // The value is constructed in-place if the key is not in the map, otherwise
      // it is not moved.
      template <typename... Ts>
      std::pair<iterator, bool> try_emplace(KeyT &&Key, Ts &&... Args) noexcept {
        BucketT *TheBucket;
        if (lookup_bucket_for(Key, TheBucket))
          return std::make_pair(makeIterator(TheBucket,
                                             shouldReverseIterate<KeyT>()
                                                 ? getBuckets()
                                                 : getBucketsEnd(),
                                             *this, true),
                                false); // Already in map.

        // Otherwise, insert the new element.
        TheBucket =
            InsertIntoBucket(TheBucket, std::move(Key), std::forward<Ts>(Args)...);
        return std::make_pair(makeIterator(TheBucket,
                                           shouldReverseIterate<KeyT>()
                                               ? getBuckets()
                                               : getBucketsEnd(),
                                           *this, true),
                              true);
      }

      // Inserts key,value pair into the map if the key isn't already in the map.
      // The value is constructed in-place if the key is not in the map, otherwise
      // it is not moved.
      template <typename... Ts>
      std::pair<iterator, bool> try_emplace(const KeyT &Key, Ts &&... Args) noexcept {
        BucketT *TheBucket;
        if (lookup_bucket_for(Key, TheBucket))
          return std::make_pair(makeIterator(TheBucket,
                                             shouldReverseIterate<KeyT>()
                                                 ? getBuckets()
                                                 : getBucketsEnd(),
                                             *this, true),
                                false); // Already in map.

        // Otherwise, insert the new element.
        TheBucket = InsertIntoBucket(TheBucket, Key, std::forward<Ts>(Args)...);
        return std::make_pair(makeIterator(TheBucket,
                                           shouldReverseIterate<KeyT>()
                                               ? getBuckets()
                                               : getBucketsEnd(),
                                           *this, true),
                              true);
      }

      /// Alternate version of insert() which allows a different, and possibly
      /// less expensive, key type.
      /// The DenseMapInfo is responsible for supplying methods
      /// getHashValue(LookupKeyT) and isEqual(LookupKeyT, KeyT) for each key
      /// type used.
      template <typename LookupKeyT>
      std::pair<iterator, bool> insert_as(std::pair<KeyT, ValT> &&KV, const LookupKeyT &Val) noexcept {
        BucketT *TheBucket;
        if (lookup_bucket_for(Val, TheBucket))
          return std::make_pair(makeIterator(TheBucket,
                                             shouldReverseIterate<KeyT>()
                                                 ? getBuckets()
                                                 : getBucketsEnd(),
                                             *this, true),
                                false); // Already in map.

        // Otherwise, insert the new element.
        TheBucket = InsertIntoBucketWithLookup(TheBucket, std::move(KV.first),
                                               std::move(KV.second), Val);
        return std::make_pair(makeIterator(TheBucket,
                                           shouldReverseIterate<KeyT>()
                                               ? getBuckets()
                                               : getBucketsEnd(),
                                           *this, true),
                              true);
      }

      /// insert - Range insertion of pairs.
      template<typename InputIt>
      void insert(InputIt I, InputIt E) noexcept {
        for (; I != E; ++I)
          insert(*I);
      }

      bool erase(const KeyT &Val) noexcept {
        BucketT *TheBucket;
        if (!lookup_bucket_for(Val, TheBucket))
          return false; // not in map.

        TheBucket->getSecond().~ValT();
        TheBucket->getFirst() = getTombstoneKey();
        decrementNumEntries();
        incrementNumTombstones();
        return true;
      }
      void erase(iterator I) noexcept {
        BucketT *TheBucket = &*I;
        TheBucket->getSecond().~ValT();
        TheBucket->getFirst() = getTombstoneKey();
        decrementNumEntries();
        incrementNumTombstones();
      }

      value_type& find_and_construct(const KeyT &Key) noexcept {
        BucketT *TheBucket;
        if (lookup_bucket_for(Key, TheBucket))
          return *TheBucket;

        return *InsertIntoBucket(TheBucket, Key);
      }

      ValT& operator[](const KeyT &Key) noexcept {
        return find_and_construct(Key).second;
      }

      value_type& find_and_construct(KeyT &&Key) noexcept {
        BucketT *TheBucket;
        if (lookup_bucket_for(Key, TheBucket))
          return *TheBucket;

        return *InsertIntoBucket(TheBucket, std::move(Key));
      }

      ValT& operator[](KeyT &&Key) noexcept {
        return find_and_construct(std::move(Key)).second;
      }

      /// ispointer_into_buckets_array - Return true if the specified pointer points
      /// somewhere into the DenseMap's array of buckets (i.e. either to a key or
      /// value in the DenseMap).
      AGT_nodiscard bool is_pointer_into_buckets_array(const void *Ptr) const noexcept {
        return Ptr >= getBuckets() && Ptr < getBucketsEnd();
      }

      /// getpointer_into_buckets_array() - Return an opaque pointer into the buckets
      /// array.  In conjunction with the previous method, this can be used to
      /// determine whether an insertion caused the DenseMap to reallocate.
      AGT_nodiscard const void* get_pointer_into_buckets_array() const noexcept { return getBuckets(); }

    protected:
      basic_map() = default;

      void destroyAll() {
        if (getNumBuckets() == 0) // Nothing to do.
          return;

        const KeyT EmptyKey = getEmptyKey(), TombstoneKey = getTombstoneKey();
        for (BucketT *P = getBuckets(), *E = getBucketsEnd(); P != E; ++P) {
          if (!InfoT::isEqual(P->getFirst(), EmptyKey) &&
              !InfoT::isEqual(P->getFirst(), TombstoneKey))
            P->getSecond().~ValT();
          P->getFirst().~KeyT();
        }
      }

      void initEmpty() {
        setNumEntries(0);
        setNumTombstones(0);

        assert((getNumBuckets() & (getNumBuckets()-1)) == 0 &&
               "# initial buckets must be a power of two!");
        const KeyT EmptyKey = getEmptyKey();
        for (BucketT *B = getBuckets(), *E = getBucketsEnd(); B != E; ++B)
          ::new (&B->getFirst()) KeyT(EmptyKey);
      }

      /// Returns the number of buckets to allocate to ensure that the DenseMap can
      /// accommodate \p NumEntries without need to grow().
      unsigned getMinBucketToReserveForEntries(unsigned NumEntries) {
        // Ensure that "NumEntries * 4 < NumBuckets * 3"
        if (NumEntries == 0)
          return 0;
        // +1 is required because of the strict equality.
        // For example if NumEntries is 48, we need to return 401.
        return nextPowerOf2(NumEntries * 4 / 3 + 1);
      }

      void moveFromOldBuckets(BucketT *OldBucketsBegin, BucketT *OldBucketsEnd) {
        initEmpty();

        // Insert all the old elements.
        const KeyT EmptyKey = getEmptyKey();
        const KeyT TombstoneKey = getTombstoneKey();
        for (BucketT *B = OldBucketsBegin, *E = OldBucketsEnd; B != E; ++B) {
          if (!InfoT::isEqual(B->getFirst(), EmptyKey) &&
              !InfoT::isEqual(B->getFirst(), TombstoneKey)) {
            // Insert the key/value into the new table.
            BucketT *DestBucket;
            bool FoundVal = lookup_bucket_for(B->getFirst(), DestBucket);
            (void)FoundVal; // silence warning.
            assert(!FoundVal && "Key already in new map?");
            DestBucket->getFirst() = std::move(B->getFirst());
            ::new (&DestBucket->getSecond()) ValT(std::move(B->getSecond()));
            incrementNumEntries();

            // Free the value.
            B->getSecond().~ValT();
          }
          B->getFirst().~KeyT();
        }
      }

      template <typename OtherBaseT>
      void copyFrom(const basic_map<OtherBaseT, KeyT, ValT, InfoT, BucketT>& other) noexcept {
        assert(&other != this);
        assert(getNumBuckets() == other.getNumBuckets());

        setNumEntries(other.getNumEntries());
        setNumTombstones(other.getNumTombstones());

        if constexpr (std::is_trivially_copyable_v<KeyT> && std::is_trivially_copyable_v<ValT>) {
          std::memcpy(reinterpret_cast<void *>(getBuckets()), other.getBuckets(), getNumBuckets() * sizeof(BucketT));
        } else {
          for (size_t i = 0; i < getNumBuckets(); ++i) {
            ::new (&getBuckets()[i].getFirst())
                KeyT(other.getBuckets()[i].getFirst());
            if (!InfoT::is_equal(getBuckets()[i].getFirst(), getEmptyKey()) &&
                !InfoT::is_equal(getBuckets()[i].getFirst(), getTombstoneKey()))
              ::new (&getBuckets()[i].getSecond())
                  ValT(other.getBuckets()[i].getSecond());
          }
        }
      }

      static unsigned getHashValue(const KeyT &Val) {
        return InfoT::get_hash_value(Val);
      }

      template<typename LookupKeyT>
      static unsigned getHashValue(const LookupKeyT &Val) {
        return InfoT::get_hash_value(Val);
      }

      static const KeyT getEmptyKey() noexcept {
        static_assert(std::is_base_of<basic_map, Derived>::value,
                      "Must pass the derived type to this template!");
        return InfoT::get_empty_key();
      }

      static const KeyT getTombstoneKey() noexcept {
        return InfoT::get_tombstone_key();
      }

    private:
      iterator       makeIterator(BucketT *P, BucketT *E, debug_epoch_base& epoch, bool NoAdvance=false) {
        /*if (shouldReverseIterate<KeyT>()) {
          BucketT *B = P == getBucketsEnd() ? getBuckets() : P + 1;
          return iterator(B, E, epoch, NoAdvance);
        }*/
        return iterator(P, E, epoch, NoAdvance);
      }

      const_iterator makeConstIterator(const BucketT *P, const BucketT *E,
                                       const debug_epoch_base& epoch,
                                       const bool NoAdvance=false) const {
        /*if (shouldReverseIterate<KeyT>()) {
          const BucketT *B = P == getBucketsEnd() ? getBuckets() : P + 1;
          return const_iterator(B, E, Epoch, NoAdvance);
        }*/
        return const_iterator(P, E, epoch, NoAdvance);
      }

      AGT_nodiscard unsigned getNumEntries() const noexcept {
        return static_cast<const Derived*>(this)->getNumEntries();
      }

      void setNumEntries(unsigned Num) noexcept {
        static_cast<Derived *>(this)->setNumEntries(Num);
      }

      void incrementNumEntries() noexcept {
        setNumEntries(getNumEntries() + 1);
      }

      void decrementNumEntries() noexcept {
        setNumEntries(getNumEntries() - 1);
      }

      AGT_nodiscard unsigned getNumTombstones() const noexcept {
        return static_cast<const Derived *>(this)->getNumTombstones();
      }

      void setNumTombstones(unsigned Num) noexcept {
        static_cast<Derived*>(this)->setNumTombstones(Num);
      }

      void incrementNumTombstones() noexcept {
        setNumTombstones(getNumTombstones() + 1);
      }

      void decrementNumTombstones() noexcept {
        setNumTombstones(getNumTombstones() - 1);
      }

      AGT_nodiscard const BucketT *getBuckets() const noexcept {
        return static_cast<const Derived *>(this)->getBuckets();
      }

      AGT_nodiscard BucketT *getBuckets() noexcept {
        return static_cast<Derived*>(this)->getBuckets();
      }

      AGT_nodiscard unsigned getNumBuckets() const noexcept {
        return static_cast<const Derived *>(this)->getNumBuckets();
      }

      BucketT *getBucketsEnd() noexcept {
        return getBuckets() + getNumBuckets();
      }

      const BucketT *getBucketsEnd() const noexcept {
        return getBuckets() + getNumBuckets();
      }

      void grow(unsigned AtLeast) noexcept {
        static_cast<Derived *>(this)->grow(AtLeast);
      }

      void shrink_and_clear() noexcept {
        static_cast<Derived *>(this)->shrink_and_clear();
      }

      template <typename KeyArg, typename... ValueArgs>
      BucketT *InsertIntoBucket(BucketT *TheBucket, KeyArg &&Key,
                                ValueArgs &&... Values) {
        TheBucket = InsertIntoBucketImpl(Key, Key, TheBucket);

        TheBucket->getFirst() = std::forward<KeyArg>(Key);
        ::new (&TheBucket->getSecond()) ValT(std::forward<ValueArgs>(Values)...);
        return TheBucket;
      }

      template <typename LookupKeyT>
      BucketT *InsertIntoBucketWithLookup(BucketT *TheBucket, KeyT &&Key,
                                          ValT &&Value, LookupKeyT &Lookup) {
        TheBucket = InsertIntoBucketImpl(Key, Lookup, TheBucket);

        TheBucket->getFirst() = std::move(Key);
        ::new (&TheBucket->getSecond()) ValT(std::move(Value));
        return TheBucket;
      }

      template <typename LookupKeyT>
      BucketT *InsertIntoBucketImpl(const KeyT &Key, const LookupKeyT &Lookup,
                                    BucketT *TheBucket) {
        increment_epoch();

        // If the load of the hash table is more than 3/4, or if fewer than 1/8 of
        // the buckets are empty (meaning that many are filled with tombstones),
        // grow the table.
        //
        // The later case is tricky.  For example, if we had one empty bucket with
        // tons of tombstones, failing lookups (e.g. for insertion) would have to
        // probe almost the entire table until it found the empty bucket.  If the
        // table completely filled with tombstones, no lookup would ever succeed,
        // causing infinite loops in lookup.
        unsigned NewNumEntries = getNumEntries() + 1;
        unsigned NumBuckets = getNumBuckets();
        if (NewNumEntries * 4 >= NumBuckets * 3) [[likely]] {
          this->grow(NumBuckets * 2);
          lookup_bucket_for(Lookup, TheBucket);
          NumBuckets = getNumBuckets();
        } else if (NumBuckets-(NewNumEntries+getNumTombstones()) <=
                                 NumBuckets/8) [[unlikely]] {
          this->grow(NumBuckets);
          lookup_bucket_for(Lookup, TheBucket);
        }
        assert(TheBucket);

        // Only update the state after we've grown our bucket space appropriately
        // so that when growing buckets we have self-consistent entry count.
        incrementNumEntries();

        // If we are writing over a tombstone, remember this.
        const KeyT EmptyKey = getEmptyKey();
        if (!InfoT::is_equal(TheBucket->getFirst(), EmptyKey))
          decrementNumTombstones();

        return TheBucket;
      }

      /// lookup_bucket_for - Lookup the appropriate bucket for Val, returning it in
      /// FoundBucket.  If the bucket contains the key and a value, this returns
      /// true, otherwise it returns a bucket with an empty marker or tombstone and
      /// returns false.
      template<typename LookupKeyT>
      bool lookup_bucket_for(const LookupKeyT &Val, const BucketT *&FoundBucket) const noexcept {
        const BucketT *BucketsPtr = getBuckets();
        const unsigned NumBuckets = getNumBuckets();

        if (NumBuckets == 0) {
          FoundBucket = nullptr;
          return false;
        }

        // FoundTombstone - Keep track of whether we find a tombstone while probing.
        const BucketT *FoundTombstone = nullptr;
        const KeyT EmptyKey = getEmptyKey();
        const KeyT TombstoneKey = getTombstoneKey();
        assert(!InfoT::is_equal(Val, EmptyKey) &&
               !InfoT::is_equal(Val, TombstoneKey) &&
               "Empty/Tombstone value shouldn't be inserted into map!");

        unsigned BucketNo = getHashValue(Val) & (NumBuckets-1);
        unsigned ProbeAmt = 1;
        while (true) {
          const BucketT *ThisBucket = BucketsPtr + BucketNo;
          // Found Val's bucket?  If so, return it.
          if (InfoT::is_equal(Val, ThisBucket->getFirst())) [[likely]] {
            FoundBucket = ThisBucket;
            return true;
          }

          // If we found an empty bucket, the key doesn't exist in the set.
          // Insert it and return the default value.
          if (InfoT::is_equal(ThisBucket->getFirst(), EmptyKey)) [[likely]] {
            // If we've already seen a tombstone while probing, fill it in instead
            // of the empty bucket we eventually probed to.
            FoundBucket = FoundTombstone ? FoundTombstone : ThisBucket;
            return false;
          }

          // If this is a tombstone, remember it.  If Val ends up not in the map, we
          // prefer to return it than something that would require more probing.
          if (InfoT::isEqual(ThisBucket->getFirst(), TombstoneKey) &&
              !FoundTombstone)
            FoundTombstone = ThisBucket;  // Remember the first tombstone found.

          // Otherwise, it's a hash collision or a tombstone, continue quadratic
          // probing.
          BucketNo += ProbeAmt++;
          BucketNo &= (NumBuckets-1);
        }
      }

      template <typename LookupKeyT>
      bool lookup_bucket_for(const LookupKeyT &Val, BucketT *&FoundBucket) noexcept {
        const BucketT* ConstFoundBucket;
        bool Result = const_cast<const basic_map*>(this)->lookup_bucket_for(Val, ConstFoundBucket);
        FoundBucket = const_cast<BucketT *>(ConstFoundBucket);
        return Result;
      }

    public:
      /// Return the approximate size (in bytes) of the actual map.
      /// This is just the raw memory used by DenseMap.
      /// If entries are pointers to objects, the size of the referenced objects
      /// are not included.
      AGT_nodiscard size_t get_memory_size() const noexcept {
        return getNumBuckets() * sizeof(BucketT);
      }
    };
  }

  template <typename Key, typename Val,
            typename KeyInfo = map_key_info<Key>,
            typename Bucket = impl::bucket<Key, Val>,
            typename Alloc = default_allocator>
  class map
      : public impl::basic_map<map<Key, Val, KeyInfo, Bucket, Alloc>, Key, Val, KeyInfo, Bucket>,
        Alloc
      {

    friend class impl::basic_map<map<Key, Val, KeyInfo, Bucket, Alloc>, Key, Val, KeyInfo, Bucket>;

    using base_type = impl::basic_map<map<Key, Val, KeyInfo, Bucket, Alloc>, Key, Val, KeyInfo, Bucket>;

    Bucket*  buckets;
    uint32_t entryCount;
    uint32_t tombstoneCount;
    uint32_t bucketCount;

  public:

    explicit map(unsigned InitialReserve = 0) { init(InitialReserve); }

    map(const map &other) : base_type() {
      init(0);
      copyFrom(other);
    }

    map(map &&other) noexcept : base_type() {
      init(0);
      swap(other);
    }

    template<typename InputIt>
    map(const InputIt &I, const InputIt &E) {
      init(std::distance(I, E));
      this->insert(I, E);
    }

    map(std::initializer_list<typename base_type::value_type> Vals) {
      init(Vals.size());
      this->insert(Vals.begin(), Vals.end());
    }

    ~map() {
      this->destroyAll();
      free_array(buckets, bucketCount, alignof(Bucket));
    }

    void swap(map& RHS) {
      this->increment_epoch();
      RHS.increment_epoch();
      std::swap(buckets, RHS.buckets);
      std::swap(entryCount, RHS.entryCount);
      std::swap(tombstoneCount, RHS.tombstoneCount);
      std::swap(bucketCount, RHS.bucketCount);
    }

    map& operator=(const map& other) {
      if (&other != this)
        copyFrom(other);
      return *this;
    }

    map& operator=(map &&other) noexcept {
      this->destroyAll();
      free_array(buckets, bucketCount, alignof(Bucket));
      init(0);
      swap(other);
      return *this;
    }

    void copyFrom(const map& other) noexcept {
      this->destroyAll();
      free_array(buckets, bucketCount, alignof(Bucket));
      if (allocateBuckets(other.NumBuckets)) {
        this->base_type::copyFrom(other);
      } else {
        entryCount = 0;
        tombstoneCount = 0;
      }
    }

    void init(unsigned InitNumEntries) noexcept {
      auto InitBuckets = base_type::getMinBucketToReserveForEntries(InitNumEntries);
      if (allocateBuckets(InitBuckets)) {
        this->BaseT::initEmpty();
      } else {
        entryCount = 0;
        tombstoneCount = 0;
      }
    }

    void grow(unsigned AtLeast) noexcept {
      unsigned OldNumBuckets = bucketCount;
      Bucket* oldBuckets = buckets;

      allocateBuckets(std::max<unsigned>(64, static_cast<unsigned>(impl::nextPowerOf2(AtLeast-1))));
      assert(buckets);
      if (!oldBuckets) {
        this->base_type::initEmpty();
        return;
      }

      this->moveFromOldBuckets(oldBuckets, oldBuckets+OldNumBuckets);

      // Free the old table.
      free_array(oldBuckets, OldNumBuckets, alignof(Bucket));
    }

    void shrink_and_clear() noexcept {
      unsigned OldNumBuckets = bucketCount;
      unsigned OldNumEntries = entryCount;
      this->destroyAll();

      // Reduce the number of buckets.
      unsigned NewNumBuckets = 0;
      if (OldNumEntries)
        NewNumBuckets = std::max(64, 1 << (impl::log2Ceil(OldNumEntries) + 1));
      if (NewNumBuckets == bucketCount) {
        this->BaseT::initEmpty();
        return;
      }

      free_array(buckets, OldNumBuckets, alignof(Bucket));
      init(NewNumBuckets);
    }

  private:
    unsigned getNumEntries() const noexcept {
      return entryCount;
    }

    void setNumEntries(unsigned Num) noexcept {
      entryCount = Num;
    }

    unsigned getNumTombstones() const noexcept {
      return tombstoneCount;
    }

    void setNumTombstones(unsigned Num) noexcept {
      tombstoneCount = Num;
    }

    Bucket* getBuckets() const noexcept {
      return buckets;
    }

    unsigned getNumBuckets() const noexcept {
      return bucketCount;
    }

    bool allocateBuckets(unsigned Num) noexcept {
      bucketCount = Num;
      if (bucketCount == 0) {
        buckets = nullptr;
        return false;
      }

      buckets = alloc_array<Bucket>(bucketCount, alignof(Bucket));

      // buckets = static_cast<Bucket*>(allocate_buffer(sizeof(Bucket) * bucketCount, alignof(Bucket)));
      return true;
    }
  };

  template <typename Key, typename Val, typename Bucket = impl::bucket<Key, Val>>
  class shared_map {

  };

}

#endif//AGATE_SUPPORT_MAP_HPP
