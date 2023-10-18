//
// Created by maxwe on 2022-08-07.
//

#ifndef AGATE_SUPPORT_MAP_HPP
#define AGATE_SUPPORT_MAP_HPP

#include "allocator.hpp"
#include "iterator.hpp"

#include "epoch_tracker.hpp"

#include <bit>
#include <concepts>

namespace agt {

  template <typename T>
  struct nonnull;

  template <typename T>
  struct map_key_info;
  //static inline T get_empty_key();
  //static inline T get_tombstone_key();
  //static unsigned get_hash_value(const T &Val);
  //static bool     is_equal(const T &LHS, const T &RHS);

  template <typename T>
  struct map_key_info<T*> {

    inline static constexpr uintptr_t MaxAlignBits = 12;
    inline static constexpr uintptr_t MaxAlign     = (0x1ULL << MaxAlignBits);

    using key_type = T*;

    static inline key_type get_empty_key() noexcept {
      return reinterpret_cast<T*>(static_cast<uintptr_t>(-1) << MaxAlignBits);
    }
    static inline key_type get_tombstone_key() noexcept {
      return reinterpret_cast<T*>(static_cast<uintptr_t>(-2) << MaxAlignBits);
    }
    static unsigned        get_hash_value(const T* val) noexcept {
      return (unsigned((uintptr_t)val) >> 4) ^
             (unsigned((uintptr_t)val) >> 9);
    }
    static bool            is_equal(const T* a, const T* b) noexcept {
      return a == b;
    }
  };

  template <typename T>
  struct map_key_info<nonnull<T*>> {

    using key_type = T*;

    static inline T* get_empty_key()     noexcept {
      return nullptr;
    }
    static inline T* get_tombstone_key() noexcept {
      return reinterpret_cast<T*>(static_cast<uintptr_t>(-1));
    }
    static unsigned  get_hash_value(const T* val) noexcept {
      return map_key_info<T*>::get_hash_value(val);
    }
    static bool      is_equal(const T* a, const T* b) noexcept {
      return a == b;
    }
  };

  /*template <std::integral I>
  struct map_key_info<I> {
    //static inline T get_empty_key();
    //static inline T get_tombstone_key();
    //static unsigned get_hash_value(const T &Val);
    //static bool     is_equal(const T &LHS, const T &RHS);
  };

  template <typename T> requires std::is_enum_v<T>
  struct map_key_info<T> {

  };*/

  namespace impl {
    struct no_value{};

    template <typename InfoT, typename KeyT, typename = void>
    struct get_key {
      using type = KeyT;
    };
    template <typename InfoT, typename KeyT>
    struct get_key<InfoT, KeyT, std::void_t<typename InfoT::key_type>> {
      using type = typename InfoT::key_type;
    };

    template <typename InfoT, typename KeyT>
    using key_t = typename get_key<InfoT, KeyT>::type;

    template <std::unsigned_integral I>
    inline constexpr static I nextPowerOf2(I i) noexcept {
      return std::bit_ceil(i + 1);
    }

    template <std::unsigned_integral I>
    inline constexpr unsigned log2Ceil(I i) {
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

      using key_type   = Key;
      using value_type = Val;

      key_type   first;
      value_type second;

      key_type&         key()         noexcept { return first; }
      const key_type&   key()   const noexcept { return first; }
      value_type&       value()       noexcept { return second; }
      const value_type& value() const noexcept { return second; }
    };

    template <typename Key>
    struct bucket<Key, no_value> : no_value {

      using key_type   = Key;
      using value_type = no_value;

      Key first;

      key_type&         key()         noexcept { return first; }
      const key_type&   key()   const noexcept { return first; }
      value_type&       value()       noexcept { return *this; }
      const value_type& value() const noexcept { return *this; }
    };

    template <typename B>
    concept is_bucket_type = requires(B& b, const B& cb) {
      typename B::key_type;
      typename B::value_type;
      {  b.key()   } noexcept -> std::same_as<typename B::key_type&>;
      { cb.key()   } noexcept -> std::same_as<const typename B::key_type&>;
      {  b.value() } noexcept -> std::same_as<typename B::value_type&>;
      { cb.value() } noexcept -> std::same_as<const typename B::value_type&>;
    };

    template <typename T>
    concept has_member_bucket_type = requires {
      typename T::bucket_type;
    } && is_bucket_type<typename T::bucket_type>;


    template <typename Info, typename Key, typename Val>
    struct get_bucket {
      using type = bucket<key_t<Info, Key>, Val>;
    };
    template <has_member_bucket_type Info, typename Key, typename Val>
    struct get_bucket<Info, Key, Val> {
      using type = typename Info::bucket_type;
    };

    template <typename Info, typename Key, typename Val>
    using bucket_t = typename get_bucket<Info, Key, Val>::type;
  }



  template <typename Key,
            typename Val,
            typename Info    = map_key_info<Key>,
            bool     IsConst = false>
  class map_iterator;

  namespace impl {

    template <typename T>
    using const_ptr_or_ref_t = typename const_pointer_or_const_ref<T>::type;

    template <typename OKey, typename Info, typename Key>
    concept lookup_key_type = requires(const OKey& okey, typename const_pointer_or_const_ref<Key>::type key) {
      { Info::get_hash_value(okey) } noexcept -> std::unsigned_integral;
      { Info::is_equal(okey, key) } noexcept  -> std::same_as<bool>;
    };

    
    template <typename Derived,
              typename Key,
              typename Val,
              typename Info>
    class basic_map : public debug_epoch_base {
      template <typename T>
      using const_arg_type_t = typename const_pointer_or_const_ref<T>::type;

      using key_info = Info;
    public:

      using bucket_type = bucket_t<Info, Key, Val>;
      using size_type   = uint32_t;
      using key_type    = typename bucket_type::key_type;
      using mapped_type = typename bucket_type::value_type;
      using value_type  = bucket_type;

      using iterator       = map_iterator<Key, Val, Info, false>;
      using const_iterator = map_iterator<Key, Val, Info, true>;


      inline iterator       begin()       noexcept {
        // When the map is empty, avoid the overhead of advancing/retreating past
        // empty buckets.
        if (empty())
          return end();

        if constexpr (should_reverse_iterate<key_type>())
          return _make_iterator(_get_buckets_end() - 1);
        else
          return _make_iterator(_get_buckets());
      }
      inline iterator       end()         noexcept {
        if constexpr (should_reverse_iterate<key_type>())
          return _make_iterator(_get_buckets());
        else
          return _make_iterator(_get_buckets_end());
      }
      inline const_iterator begin() const noexcept {
        if (empty())
          return end();
        if constexpr (should_reverse_iterate<key_type>())
          return _make_const_iterator(_get_buckets_end() - 1);
        else
          return _make_const_iterator(_get_buckets());
      }
      inline const_iterator end()   const noexcept {
        if constexpr (should_reverse_iterate<key_type>())
          return _make_const_iterator(_get_buckets());
        else
          return _make_const_iterator(_get_buckets_end());
      }

      AGT_nodiscard bool      empty() const noexcept {
        return _get_entry_count() == 0;
      }
      AGT_nodiscard size_type size()  const noexcept {
        return _get_entry_count();
      }
      
      void reserve(size_type entryCount) noexcept {
        auto minBucketCount = get_min_bucket_to_reserve_for_entries(entryCount);
        increment_epoch();
        if (minBucketCount > _get_bucket_count())
          _grow(minBucketCount);
      }
      
      void clear() noexcept {
        increment_epoch();

        if (empty() && _get_tombstone_count() == 0)
          return;

        uint32_t entryCount  = size();
        uint32_t bucketCount = _get_bucket_count();

        // If the capacity of the array is huge, and the # elements used is small,
        // shrink the array.
        if ((entryCount * 4) < bucketCount && bucketCount > 64) {
          _shrink_and_clear();
          return;
        }

        const key_type empty     = empty_key();
        const key_type tombstone = tombstone_key();

        if constexpr (std::is_trivially_destructible_v<mapped_type>) {
          for (bucket_type* bucket = _get_buckets(), *end = _get_buckets_end(); bucket != end; ++bucket)
            bucket->key() = empty;
        }
        else {
          for (bucket_type* bucket = _get_buckets(), *end = _get_buckets_end(); bucket != end; ++bucket) {
            if (!key_info::is_equal(bucket->key(), empty)) {
              if (!key_info::is_equal(bucket->key(), tombstone)) {
                bucket->value().~mapped_type();
                --entryCount;
              }
              bucket->key() = empty;
            }
          }

          assert(entryCount == 0 && "Node count imbalance!");
        }

        _set_entry_count(0);
        _set_tombstone_count(0);
      }

      /// Return 1 if the specified key is in the map, 0 otherwise.
      AGT_nodiscard size_type count(const_arg_type_t<key_type> val) const noexcept {
        const bucket_type* bucket;
        return lookup_bucket_for(val, bucket) ? 1 : 0;
      }

      iterator       find(const_arg_type_t<key_type> val) noexcept {
        bucket_type* bucket;
        if (lookup_bucket_for(val, bucket))
          return _make_iterator(bucket, true);
        return end();
      }
      const_iterator find(const_arg_type_t<key_type> val) const noexcept {
        const bucket_type* bucket;
        if (lookup_bucket_for(val, bucket))
          return _make_const_iterator(bucket, true);
        return end();
      }

      /// Alternate version of find() which allows a different, and possibly
      /// less expensive, key type.
      /// The DenseMapInfo is responsible for supplying methods
      /// getHashValue(LookupKeyT) and isEqual(LookupKeyT, KeyT) for each key
      /// type used.
      template <lookup_key_type<Info, key_type> LookupKeyT>
      iterator       find_as(const LookupKeyT& val)       noexcept {
        bucket_type* bucket;
        if (lookup_bucket_for(val, bucket))
          return _make_iterator(bucket, true);
        return end();
      }
      template <lookup_key_type<Info, key_type> LookupKeyT>
      const_iterator find_as(const LookupKeyT& val) const noexcept {
        const bucket_type* bucket;
        if (lookup_bucket_for(val, bucket))
          return _make_const_iterator(bucket, true);
        return end();
      }

      /// lookup - Return the entry for the specified key, or a default
      /// constructed value if no such entry exists.
      mapped_type    lookup(const_arg_type_t<key_type> val) const noexcept {
        const bucket_type* bucket;
        if (lookup_bucket_for(val, bucket))
          return bucket->value();
        return mapped_type();
      }
      
      
      // Inserts key,value pair into the map if the key isn't already in the map.
      // If the key is already in the map, it returns false and doesn't update the
      // value.
      std::pair<iterator, bool> insert(const std::pair<key_type, mapped_type>& kv) noexcept {
        return try_emplace(kv.first, kv.second);
      }
      
      // Inserts key,value pair into the map if the key isn't already in the map.
      // If the key is already in the map, it returns false and doesn't update the
      // value.
      std::pair<iterator, bool> insert(std::pair<key_type, mapped_type>&& kv) noexcept {
        return try_emplace(std::move(kv.first), std::move(kv.second));
      }

      // Inserts key,value pair into the map if the key isn't already in the map.
      // The value is constructed in-place if the key is not in the map, otherwise
      // it is not moved.
      template <typename... T>
      std::pair<iterator, bool> try_emplace(key_type&& key, T&& ...args) noexcept {
        bucket_type* bucket;
        if (lookup_bucket_for(key, bucket))
          return std::make_pair(_make_iterator(bucket, true), false); // Already in map.

        // Otherwise, insert the new element.
        bucket = _insert_into_bucket(bucket, std::move(key), std::forward<T>(args)...);
        return std::make_pair(_make_iterator(bucket, true), true);
      }

      // Inserts key,value pair into the map if the key isn't already in the map.
      // The value is constructed in-place if the key is not in the map, otherwise
      // it is not moved.
      template <typename... T>
      std::pair<iterator, bool> try_emplace(const key_type& key, T&& ...args) noexcept {
        bucket_type* bucket;
        if (lookup_bucket_for(key, bucket))
          return std::make_pair(_make_iterator(bucket, true), false);

        bucket = _insert_into_bucket(bucket, key, std::forward<T>(args)...);
        return std::make_pair(_make_iterator(bucket, true), true);
      }

      /// Alternate version of insert() which allows a different, and possibly
      /// less expensive, key type.
      /// The DenseMapInfo is responsible for supplying methods
      /// getHashValue(LookupKeyT) and isEqual(LookupKeyT, KeyT) for each key
      /// type used.
      template <typename LookupKeyT>
      std::pair<iterator, bool> insert_as(std::pair<key_type, mapped_type>&& kv, const LookupKeyT& val) noexcept {

        bucket_type* bucket;
        if (lookup_bucket_for(val, bucket))
          return std::make_pair(_make_iterator(bucket, true), false);

        bucket = _insert_into_bucket_with_lookup(bucket, std::move(kv.first), std::move(kv.second), val);
        return std::make_pair(_make_iterator(bucket, true), true);
      }

      /// insert - Range insertion of pairs.
      template <typename InputIt, typename InputSent>
      void insert(InputIt I, InputSent E) noexcept {
        for (; I != E; ++I)
          insert(*I);
      }

      bool erase(const_arg_type_t<key_type> val) noexcept {
        bucket_type* bucket;
        if (!lookup_bucket_for(val, bucket))
          return false; // not in map.

        _erase_bucket(bucket);
        return true;
      }
      void erase(iterator iter) noexcept {
        _erase_bucket(&*iter);
      }

      value_type& find_and_construct(const key_type& key) noexcept {
        bucket_type* bucket;
        if (lookup_bucket_for(key, bucket))
          return *bucket;
        return *_insert_into_bucket(bucket, key);
      }

      mapped_type& operator[](const key_type& key) noexcept {
        return find_and_construct(key).value();
      }

      value_type& find_and_construct(key_type&& key) noexcept {
        bucket_type* bucket;
        if (lookup_bucket_for(key, bucket))
          return *bucket;

        return *_insert_into_bucket(bucket, std::move(key));
      }

      mapped_type& operator[](key_type&& key) noexcept {
        return find_and_construct(std::move(key)).value();
      }


      AGT_nodiscard bool is_pointer_into_buckets_array(const void* ptr) const noexcept {
        return _get_buckets() <= ptr && ptr < _get_buckets_end();
      }

      AGT_nodiscard const void* get_pointer_into_buckets_array() const noexcept {
        return _get_buckets();
      }

    protected:
      basic_map() = default;

      void destroy_all() noexcept {
        if constexpr (!std::is_trivially_destructible_v<bucket_type>) {

          if (_get_bucket_count() == 0) // Nothing to do.
            return;

          const key_type empty     = empty_key();
          const key_type tombstone = tombstone_key();
          for (bucket_type* pos = _get_buckets(), *end = _get_buckets_end(); pos != end; ++pos) {
            if constexpr (!std::is_trivially_destructible_v<mapped_type>) {
              if (!Info::is_equal(pos->key(), empty) && !Info::is_equal(pos->key(), tombstone))
                pos->value().~mapped_type();
            }
            pos->key().~key_type();
          }
        }
      }

      void init_empty() noexcept {
        _set_entry_count(0);
        _set_tombstone_count(0);

        assert(std::has_single_bit(_get_bucket_count())
               && "# initial buckets must be a power of two!");
        const key_type empty = empty_key();
        for (bucket_type* bucket = _get_buckets(), *end = _get_buckets_end(); bucket != end; ++bucket)
          ::new (&bucket->key()) key_type(empty);
      }

      /// Returns the minimum number of buckets required to ensure the map
      /// may accomodate up to #entries without needing to reallocate/rehash.
      unsigned get_min_bucket_to_reserve_for_entries(unsigned entries) noexcept {
        if (entries == 0)
          return 0;
        return nextPowerOf2(((entries * 4) / 3) + 1);
      }

      void move_from_old_buckets(bucket_type* oldBucketsBegin, bucket_type* oldBucketsEnd) noexcept {
        init_empty();

        // Insert all the old elements.
        const key_type empty     = empty_key();
        const key_type tombstone = tombstone_key();
        for (bucket_type* b = oldBucketsBegin, *end = oldBucketsEnd; b != end; ++b) {
          if (!(Info::is_equal(b->key(), empty) || Info::is_equal(b->key(), tombstone))) {
            bucket_type* dst;
            bool found = lookup_bucket_for(b->key(), dst);
            (void)found; // silence warning.
            assert(!found && "Key already in new map?");
            dst->key() = std::move(b->key());
            ::new (&dst->value()) mapped_type(std::move(b->value()));
            _increment_entry_count();

            // Free the value.
            if constexpr (!std::is_trivially_destructible_v<mapped_type>)
              b->value().~mapped_type();
          }
          if constexpr (!std::is_trivially_destructible_v<key_type>)
            b->key().~key_type();
        }
      }

      template <typename OtherBase>
      void copy_from(const basic_map<OtherBase, Key, Val, Info>& other) noexcept {
        assert(&other != this);
        assert(_get_bucket_count() == other._get_bucket_count());

        _set_entry_count(other._get_entry_count());
        _set_tombstone_count(other._get_tombstone_count());

        auto buckets      = _get_buckets();
        auto otherBuckets = other._get_buckets();

        if constexpr (std::is_trivially_copyable_v<bucket_type>) {
          std::memcpy(reinterpret_cast<void*>(buckets), other._get_buckets(), get_memory_size());
        } else {
          const key_type empty     = empty_key();
          const key_type tombstone = tombstone_key();

          for (size_t i = 0; i < _get_bucket_count(); ++i) {
            ::new (&buckets[i].key()) key_type(otherBuckets[i].key());
            if (!(Info::is_equal(buckets[i].key(), empty) || Info::is_equal(buckets[i].key(), tombstone)))
              ::new (&buckets[i].value()) mapped_type(otherBuckets[i].value());
          }
        }
      }

      static unsigned hash_value(const_arg_type_t<key_type> key) noexcept {
        return Info::get_hash_value(key);
      }

      template <typename LookupKeyT>
      static unsigned hash_value(const LookupKeyT& lookupKey) noexcept {
        return Info::get_hash_value(lookupKey);
      }

      static const key_type empty_key()     noexcept {
        static_assert(std::derived_from<Derived, basic_map>,
                      "Must pass the derived type to this template!");
        return Info::get_empty_key();
      }

      static const key_type tombstone_key() noexcept {
        return Info::get_tombstone_key();
      }

    private:

      void _erase_bucket(bucket_type* bucket) noexcept {
        if constexpr (!std::is_trivially_destructible_v<mapped_type>)
          bucket->value().~mapped_type();
        bucket->key() = tombstone_key();
        _decrement_entry_count();
        _increment_tombstone_count();
      }

      iterator       _make_iterator(bucket_type* pos, bool noAdvance = false) noexcept {
        if constexpr (should_reverse_iterate<key_type>())
          return iterator(pos + 1, _get_buckets(), *this, noAdvance);
        else
          return iterator(pos, _get_buckets_end(), *this, noAdvance);
      }

      const_iterator _make_const_iterator(const bucket_type* pos, bool noAdvance = false) const noexcept {
        if constexpr (should_reverse_iterate<key_type>())
          return const_iterator(pos + 1, _get_buckets(), *this, noAdvance);
        else
          return const_iterator(pos, _get_buckets_end(), *this, noAdvance);
      }

      AGT_nodiscard unsigned _get_entry_count() const noexcept {
        return static_cast<const Derived*>(this)->get_entry_count();
      }

      void _set_entry_count(uint32_t num) noexcept {
        static_cast<Derived*>(this)->set_entry_count(num);
      }

      void _increment_entry_count() noexcept {
        _set_entry_count(_get_entry_count() + 1);
      }

      void _decrement_entry_count() noexcept {
        _set_entry_count(_get_entry_count() - 1);
      }

      AGT_nodiscard uint32_t _get_tombstone_count() const noexcept {
        return static_cast<const Derived *>(this)->get_tombstone_count();
      }

      void _set_tombstone_count(uint32_t num) noexcept {
        static_cast<Derived*>(this)->set_tombstone_count(num);
      }

      void _increment_tombstone_count() noexcept {
        _set_tombstone_count(_get_tombstone_count() + 1);
      }

      void _decrement_tombstone_count() noexcept {
        _set_tombstone_count(_get_tombstone_count() - 1);
      }

      AGT_nodiscard const bucket_type* _get_buckets() const noexcept {
        return static_cast<const Derived *>(this)->get_buckets();
      }

      AGT_nodiscard       bucket_type* _get_buckets() noexcept {
        return static_cast<Derived*>(this)->get_buckets();
      }

      AGT_nodiscard unsigned _get_bucket_count() const noexcept {
        return static_cast<const Derived*>(this)->get_bucket_count();
      }

      bucket_type* _get_buckets_end() noexcept {
        return _get_buckets() + _get_bucket_count();
      }

      const bucket_type* _get_buckets_end() const noexcept {
        return _get_buckets() + _get_bucket_count();
      }

      void _grow(uint32_t atLeast) noexcept {
        static_cast<Derived*>(this)->grow(atLeast);
      }

      void _shrink_and_clear() noexcept {
        static_cast<Derived*>(this)->shrink_and_clear();
      }

      template <typename KeyArg, typename... ValueArgs>
      bucket_type* _insert_into_bucket(bucket_type* bucket, KeyArg&& key, ValueArgs&& ...values) noexcept {
        bucket = this->_insert_into_bucket_impl(/*key, */key, bucket);

        bucket->key() = std::forward<KeyArg>(key);
        ::new (&bucket->value()) mapped_type(std::forward<ValueArgs>(values)...);
        return bucket;
      }

      template <typename LookupKeyT>
      bucket_type* _insert_into_bucket_with_lookup(bucket_type* bucket, key_type&& key, mapped_type&& value, const LookupKeyT& lookup) noexcept {
        bucket = _insert_into_bucket_impl(/*key, */lookup, bucket);

        bucket->key() = std::move(key);
        ::new (&bucket->value()) mapped_type(std::move(value));
        return bucket;
      }

      template <typename LookupKeyT>
      bucket_type* _insert_into_bucket_impl(/*const key_type& key, */const LookupKeyT& lookup, bucket_type* bucket) noexcept {
        increment_epoch();

        uint32_t newEntryCount = _get_entry_count() + 1;
        uint32_t bucketCount   = _get_bucket_count();

        if (newEntryCount * 4 >= bucketCount * 3) [[unlikely]] {
          this->_grow(bucketCount * 2);
          lookup_bucket_for(lookup, bucket);
          bucketCount = _get_bucket_count();
        }
        else if (bucketCount - (newEntryCount + _get_tombstone_count()) <= (bucketCount / 8)) [[unlikely]] {
          this->_grow(bucketCount);
          lookup_bucket_for(lookup, bucket);
        }

        assert( bucket != nullptr );

        _increment_entry_count();

        if (!Info::is_equal(bucket->key(), empty_key()))
          _decrement_tombstone_count();

        return bucket;
      }


      /// lookup_bucket_for - Lookup the appropriate bucket for Val, returning it in
      /// FoundBucket.  If the bucket contains the key and a value, this returns
      /// true, otherwise it returns a bucket with an empty marker or tombstone and
      /// returns false.
      template <typename LookupKeyT>
      bool lookup_bucket_for(const LookupKeyT& val, const bucket_type*& foundBucket) const noexcept {
        const bucket_type* buckets     = _get_buckets();
        const uint32_t     bucketCount = _get_bucket_count();

        if (bucketCount == 0) {
          foundBucket = nullptr;
          return false;
        }

        // FoundTombstone - Keep track of whether we find a tombstone while probing.
        const bucket_type* foundTombstone = nullptr;
        const key_type empty     = empty_key();
        const key_type tombstone = tombstone_key();
        assert(!Info::is_equal(val, empty) &&
               !Info::is_equal(val, tombstone) &&
               "Empty/Tombstone value shouldn't be inserted into map!");

        uint32_t indexMask   = bucketCount - 1;
        uint32_t bucketIndex = hash_value(val) & indexMask;
        uint32_t probeOffset = 1;

        while (true) {
          const bucket_type* thisBucket = buckets + bucketIndex;

          // Found the right bucket?  If so, return it.
          if (Info::is_equal(val, thisBucket->key())) [[likely]] {
            foundBucket = thisBucket;
            return true;
          }

          // If we found an empty bucket, the key doesn't exist in the set.
          // return either the empty bucket in question, or, if probing had
          // previously come across any tombstones, return the first one found.
          if (Info::is_equal(thisBucket->key(), empty)) [[likely]] {
            foundBucket = foundTombstone ? foundTombstone : thisBucket;
            return false;
          }

          // Remember the first tombstone found.
          // If ultimately, the key is not found, we return the first tombstone we found
          if (Info::is_equal(thisBucket->key(), tombstone) && !foundTombstone)
            foundTombstone = thisBucket;

          // Otherwise, it's a hash collision or a tombstone, continue quadratic
          // probing.
          bucketIndex += probeOffset++;
          bucketIndex &= indexMask;
        }
      }

      template <typename LookupKeyT>
      bool lookup_bucket_for(const LookupKeyT& val, bucket_type*& foundBucket) noexcept {
        const bucket_type* constFoundBucket;
        bool result = const_cast<const basic_map*>(this)->lookup_bucket_for(val, constFoundBucket);
        foundBucket = const_cast<bucket_type*>(constFoundBucket);
        return result;
      }

    public:
      /// Return the approximate size (in bytes) of the actual map.
      /// This is just the raw memory used by DenseMap.
      /// If entries are pointers to objects, the size of the referenced objects
      /// are not included.
      AGT_nodiscard size_t get_memory_size() const noexcept {
        return _get_bucket_count() * sizeof(bucket_type);
      }
    };
  }

  template <typename Key,
            typename Val,
            typename KeyInfo = map_key_info<Key>>
  class map : public impl::basic_map<map<Key, Val, KeyInfo>, Key, Val, KeyInfo> {

    friend class impl::basic_map<map<Key, Val, KeyInfo>, Key, Val, KeyInfo>;

    using base_type = impl::basic_map<map<Key, Val, KeyInfo>, Key, Val, KeyInfo>;

    using bucket_type = typename base_type::bucket_type;

    bucket_type* buckets;
    uint32_t     entryCount;
    uint32_t     tombstoneCount;
    uint32_t     bucketCount;

  public:

    explicit map(unsigned initialReserve = 0) noexcept {
      init(initialReserve);
    }

    map(const map &other) : base_type() {
      init(0);
      copyFrom(other);
    }

    map(map&& other) noexcept : base_type() {
      init(0);
      swap(other);
    }

    template<typename InputIt, std::sentinel_for<InputIt> InputSent = InputIt>
    map(const InputIt& iter, const InputSent& sent) noexcept {
      init(std::ranges::distance(iter, sent));
      this->insert(iter, sent);
    }

    map(std::initializer_list<typename base_type::value_type> vals) {
      init(vals.size());
      this->insert(vals.begin(), vals.end());
    }

    ~map() {
      this->destroy_all();
      free_array(buckets, bucketCount, alignof(bucket_type));
    }

    void swap(map& other) {
      this->increment_epoch();
      other.increment_epoch();
      std::swap(buckets, other.buckets);
      std::swap(entryCount, other.entryCount);
      std::swap(tombstoneCount, other.tombstoneCount);
      std::swap(bucketCount, other.bucketCount);
    }

    map& operator=(const map& other) {
      if (this != &other)
        copy_from(other);
      return *this;
    }

    map& operator=(map &&other) noexcept {
      this->destroy_all();
      free_array(buckets, bucketCount, alignof(bucket_type));
      init(0);
      swap(other);
      return *this;
    }

    void copy_from(const map& other) noexcept {
      this->destroy_all();
      free_array(buckets, bucketCount, alignof(bucket_type));
      if (allocate_buckets(other.get_bucket_count())) {
        this->base_type::copy_from(other);
      } else {
        entryCount = 0;
        tombstoneCount = 0;
      }
    }

    void init(uint32_t initNumEntries) noexcept {
      auto initBucketCount = base_type::get_min_bucket_to_reserve_for_entries(initNumEntries);

      if (allocate_buckets(initBucketCount))
        this->init_empty();
      else {
        entryCount = 0;
        tombstoneCount = 0;
      }
    }

    void grow(uint32_t atLeast) noexcept {
      uint32_t oldBucketCount = bucketCount;
      bucket_type* oldBuckets = buckets;

      allocate_buckets(std::max<uint32_t>(64, static_cast<uint32_t>(impl::nextPowerOf2(atLeast - 1))));

      assert(buckets);

      if (!oldBuckets) {
        this->init_empty();
        return;
      }

      this->move_from_old_buckets(oldBuckets, oldBuckets + oldBucketCount);

      // Free the old table.
      free_array(oldBuckets, oldBucketCount, alignof(bucket_type));
    }

    void shrink_and_clear() noexcept {
      uint32_t oldBucketCount = bucketCount;
      uint32_t oldEntryCount  = entryCount;
      this->destroy_all();

      // Reduce the number of buckets.

      uint32_t newBucketCount = 0;
      if (oldEntryCount)
        newBucketCount = std::max(64, 1 << (impl::log2Ceil(oldEntryCount) + 1));

      if (newBucketCount == bucketCount) {
        this->init_empty();
        return;
      }

      free_array(buckets, oldBucketCount, alignof(bucket_type));
      init(newBucketCount);
    }

  private:

    AGT_nodiscard uint32_t get_entry_count() const noexcept {
      return entryCount;
    }

    void set_entry_count(uint32_t count) noexcept {
      entryCount = count;
    }

    AGT_nodiscard uint32_t get_tombstone_count() const noexcept {
      return tombstoneCount;
    }

    void set_tombstone_count(uint32_t count) noexcept {
      tombstoneCount = count;
    }

    AGT_nodiscard uint32_t get_bucket_count() const noexcept {
      return bucketCount;
    }

    AGT_nodiscard bucket_type* get_buckets() const noexcept {
      return buckets;
    }

    bool allocate_buckets(uint32_t num) noexcept {
      bucketCount = num;
      if (bucketCount == 0) {
        buckets = nullptr;
        return false;
      }



      buckets = alloc_array<bucket_type>(bucketCount, alignof(bucket_type));

      // buckets = static_cast<Bucket*>(allocate_buffer(sizeof(Bucket) * bucketCount, alignof(Bucket)));
      return true;
    }
  };


  template <typename KeyT, typename ValT, typename InfoT, bool IsConst>
  class map_iterator : public debug_handle_base {
    friend class map_iterator<KeyT, ValT, InfoT, true>;
    friend class map_iterator<KeyT, ValT, InfoT, false>;

    using const_iterator = map_iterator<KeyT, ValT, InfoT, true>;


  public:

    using bucket_type       = impl::bucket_t<InfoT, KeyT, ValT>;
    using difference_type   = ptrdiff_t;
    using value_type        = std::conditional_t<IsConst, const bucket_type, bucket_type>;
    using pointer           = value_type*;
    using reference         = value_type&;
    using iterator_category = std::forward_iterator_tag;

    using key_type    = typename bucket_type::key_type;
    using mapped_type = typename bucket_type::value_type;
    /*using key_type    = std::conditional_t<IsConst, const KeyT, KeyT>;
    using mapped_type = std::conditional_t<IsConst, const ValT, ValT>;*/
    using key_info_type = InfoT;

    map_iterator() = default;

    map_iterator(pointer pos, pointer end, const debug_epoch_base& epoch, bool noAdvance = false)
        : debug_handle_base(&epoch),
          m_ptr(pos),
          m_end(end) {
      assert(is_handle_in_sync() && "invalid construction!");

      if (noAdvance)
        return;

      if constexpr (should_reverse_iterate<key_type>())
        retreat_past_empty();
      else
        advance_past_empty();
    }


    template <bool IsConstSrc> requires (!IsConstSrc && IsConst)
    map_iterator(const map_iterator<KeyT, ValT, InfoT, IsConstSrc>& iter)
        : debug_handle_base(iter),
          m_ptr(iter.m_ptr),
          m_end(iter.m_end)
    { }


    reference operator*()  const noexcept {
      assert(is_handle_in_sync() && "invalid iterator access!");
      assert(m_ptr != m_end && "dereferencing end() iterator");
      if constexpr (should_reverse_iterate<key_type>())
        return m_ptr[-1];
      else
        return *m_ptr;
    }
    pointer   operator->() const noexcept {
      assert(is_handle_in_sync() && "invalid iterator access!");
      assert(m_ptr != m_end && "dereferencing end() iterator");
      if constexpr (should_reverse_iterate<key_type>())
        return &m_ptr[-1];
      else
        return m_ptr;
    }


    inline map_iterator& operator++() noexcept {  // Preincrement
      assert(is_handle_in_sync() && "invalid iterator access!");
      assert(m_ptr != m_end && "incrementing end() iterator");
      if constexpr (should_reverse_iterate<key_type>()) {
        --m_ptr;
        retreat_past_empty();
      }
      else {
        ++m_ptr;
        advance_past_empty();
      }
      return *this;
    }
    map_iterator operator++(int) noexcept {  // Postincrement
      assert(is_handle_in_sync() && "invalid iterator access!");
      map_iterator tmp = *this;
      ++*this;
      return tmp;
    }




    AGT_nodiscard bool operator==(const const_iterator& other) const noexcept {
      assert((!m_ptr || is_handle_in_sync()) && "handle not in sync!");
      assert((!other.m_ptr || other.is_handle_in_sync()) && "handle not in sync!");
      assert(get_epoch_address() == other.get_epoch_address() && "comparing incomparable iterators!");
      return m_ptr == other.m_ptr;
    }


  private:

    void advance_past_empty() noexcept {
      assert(m_ptr <= m_end);
      std::add_const_t<key_type> empty     = key_info_type::get_empty_key();
      std::add_const_t<key_type> tombstone = key_info_type::get_tombstone_key();

      while (m_ptr != m_end &&
             (key_info_type::is_equal(m_ptr->first, empty) ||
              key_info_type::is_equal(m_ptr->first, tombstone)))
        ++m_ptr;
    }

    void retreat_past_empty() noexcept {
      assert(m_ptr <= m_end);
      std::add_const_t<key_type> empty     = key_info_type::get_empty_key();
      std::add_const_t<key_type> tombstone = key_info_type::get_tombstone_key();

      while (m_ptr != m_end &&
             (key_info_type::is_equal(m_ptr[-1].first, empty) ||
              key_info_type::is_equal(m_ptr[-1].first, tombstone)))
        --m_ptr;
    }

    pointer m_ptr = nullptr;
    pointer m_end = nullptr;
  };

  template <typename Key, typename Val, typename Bucket = impl::bucket<Key, Val>>
  class shared_map {

  };

}

#endif//AGATE_SUPPORT_MAP_HPP
