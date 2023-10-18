//
// Created by maxwe on 2021-09-18.
//

#ifndef JEMSYS_INTERNAL_DICTIONARY_HPP
#define JEMSYS_INTERNAL_DICTIONARY_HPP


#include "agate.h"

#include "align.hpp"
#include "iterator.hpp"
#include "allocator.hpp"

#include <string_view>
#include <memory>
#include <bit>


// Implementation of a "dictionary" type, which in this context, is a typed string map.
// String keys are mapped to values of the specified type.
// Functions more or less equivalently to an agt::map type, but is optimized for string key types.
// For string => string maps, use a codex instead, which is very similar, but is further optimized by storing
// all text (both keys and values) inline in single buckets to avoid extraneous allocation.

namespace agt {

  struct nothing_t{};
  inline constexpr static nothing_t nothing{};


  namespace impl{
    
    template <typename T>
    inline constexpr static agt_size_t ptr_free_low_bits = std::countr_zero(alignof(T));

    inline constexpr static size_t OptimalStringAlign = 16;
    
    class dictionary_entry_base {
      agt_u64_t m_keyLength;

    public:
      explicit dictionary_entry_base(agt_u64_t keyLength) : m_keyLength(keyLength) {}

      AGT_nodiscard agt_u64_t get_key_length() const noexcept { return m_keyLength; }
    };

    template <typename Val>
    class dictionary_entry_storage : public dictionary_entry_base{
      Val second;
    public:


      explicit dictionary_entry_storage(size_t keyLength)
          : dictionary_entry_base(keyLength), second() {}
      template<typename... InitTy>
      dictionary_entry_storage(size_t keyLength, InitTy &&...initVals)
          : dictionary_entry_base(keyLength),
            second(std::forward<InitTy>(initVals)...) {}
      dictionary_entry_storage(dictionary_entry_storage &e) = delete;

      const Val &get() const { return second; }
      Val &get() { return second; }

      void set(const Val &V) { second = V; }
    };

    template<>
    class dictionary_entry_storage<nothing_t> : public dictionary_entry_base {
    public:
      explicit dictionary_entry_storage(size_t keyLength, nothing_t none = nothing)
          : dictionary_entry_base(keyLength) {}
      dictionary_entry_storage(dictionary_entry_storage &entry) = delete;

      AGT_nodiscard nothing_t get() const { return nothing; }
    };

    template <typename T>
    class dictionary_entry_array_storage : public dictionary_entry_base {
      uint32_t m_arrayLength;
      uint32_t m_keyOffset;
      T        m_array[1];
    protected:
      [[nodiscard]] const char* _get_key() const noexcept {
        return std::assume_aligned<OptimalStringAlign>(reinterpret_cast<const char*>(this) + m_keyOffset);
      }
    public:
      dictionary_entry_array_storage(size_t keyLength, size_t arrayLength)
          : dictionary_entry_base(keyLength),
            m_arrayLength(arrayLength),
            m_keyOffset(align_to<OptimalStringAlign>(offsetof(dictionary_entry_array_storage, m_array) + (sizeof(T) * arrayLength)))
      {}

      dictionary_entry_array_storage(dictionary_entry_array_storage&e) = delete;


      std::span<T>       get()       noexcept { return { &m_array[0], m_arrayLength }; }
      std::span<const T> get() const noexcept { return { &m_array[0], m_arrayLength }; }

      void set(std::span<const T> elements) {
        assert( m_arrayLength == elements.size() );
        std::ranges::copy(elements, &m_array[0]);
      }
    };




    class dictionary {

      
    protected:

      // Array of NumBuckets pointers to entries, null pointers are holes.
      // TheTable[NumBuckets] contains a sentinel value for easy iteration. Followed
      // by an array of the actual hash values as unsigned integers.
      dictionary_entry_base** TheTable      = nullptr;
      agt_u32_t               NumBuckets    = 0;
      agt_u32_t               NumItems      = 0;
      agt_u32_t               NumTombstones = 0;
      agt_u32_t               ItemSize;

    protected:
      explicit dictionary(unsigned itemSize) noexcept : ItemSize(itemSize) {}
      dictionary(dictionary &&RHS) noexcept
          : TheTable(RHS.TheTable),
            NumBuckets(RHS.NumBuckets),
            NumItems(RHS.NumItems),
            NumTombstones(RHS.NumTombstones),
            ItemSize(RHS.ItemSize) {
        RHS.TheTable = nullptr;
        RHS.NumBuckets = 0;
        RHS.NumItems = 0;
        RHS.NumTombstones = 0;
      }

      /// Allocate the table with the specified number of buckets and otherwise
      /// setup the map as empty.
      void init(agt_u32_t Size) noexcept;
      void explicit_init(agt_u32_t size) noexcept;

      agt_u32_t  rehash_table(agt_u32_t BucketNo) noexcept;

      /// lookup_bucket_for - Look up the bucket that the specified string should end
      /// up in.  If it already exists as a key in the map, the Item pointer for the
      /// specified bucket will be non-null.  Otherwise, it will be null.  In either
      /// case, the FullHashValue field of the bucket will be set to the hash value
      /// of the string.
      agt_u32_t lookup_bucket_for(std::string_view Key) noexcept;

      /// find_key - Look up the bucket that contains the specified key. If it exists
      /// in the map, return the bucket number of the key.  Otherwise return -1.
      /// This does not modify the map.
      AGT_nodiscard agt_i32_t find_key(std::string_view Key) const noexcept;

      /// remove_key - Remove the specified dictionary_entry from the table, but do not
      /// delete it.  This aborts if the value isn't in the table.
      void remove_key(dictionary_entry_base *V) noexcept;

      /// remove_key - Remove the dictionary_entry for the specified key from the
      /// table, returning it.  If the key is not in the table, this returns null.
      dictionary_entry_base *remove_key(std::string_view Key) noexcept;


      void dealloc_table() noexcept;


    public:
      inline static dictionary_entry_base* get_tombstone_val() noexcept {
        constexpr static agt_u32_t free_low_bits = ptr_free_low_bits<dictionary_entry_base*>;
        auto Val = static_cast<uintptr_t>(-1);
        Val <<= free_low_bits;
        return reinterpret_cast<dictionary_entry_base *>(Val);
      }

      AGT_nodiscard agt_u32_t  get_num_buckets() const noexcept { return NumBuckets; }
      AGT_nodiscard agt_u32_t  get_num_items() const noexcept { return NumItems; }

      AGT_nodiscard bool empty() const noexcept { return NumItems == 0; }
      AGT_nodiscard agt_u32_t  size() const noexcept { return NumItems; }

      void swap(dictionary& other) noexcept {
        std::swap(TheTable, other.TheTable);
        std::swap(NumBuckets, other.NumBuckets);
        std::swap(NumItems, other.NumItems);
        std::swap(NumTombstones, other.NumTombstones);
      }
    };



    template<typename DerivedTy, typename T>
    class dictionary_iter_base : public iterator_facade_base<DerivedTy, std::forward_iterator_tag, T> {
    protected:
      impl::dictionary_entry_base** Ptr = nullptr;

      using derived_type = DerivedTy;

    public:
      dictionary_iter_base() = default;

      explicit dictionary_iter_base(impl::dictionary_entry_base** Bucket, bool NoAdvance = false)
          : Ptr(Bucket) {
        if (!NoAdvance)
          AdvancePastEmptyBuckets();
      }

      derived_type& operator=(const derived_type& Other) {
        Ptr = Other.Ptr;
        return static_cast<derived_type &>(*this);
      }



      derived_type &operator++() {// Preincrement
        ++Ptr;
        AdvancePastEmptyBuckets();
        return static_cast<derived_type &>(*this);
      }

      derived_type operator++(int) {// Post-increment
        derived_type Tmp(Ptr);
        ++*this;
        return Tmp;
      }


      friend bool operator==(const derived_type& LHS, const derived_type &RHS) noexcept {
        return LHS.Ptr == RHS.Ptr;
      }

    private:
      void AdvancePastEmptyBuckets() {
        while (*Ptr == nullptr || *Ptr == impl::dictionary::get_tombstone_val())
          ++Ptr;
      }
    };

  }

  template<typename ValueTy>
  class dictionary_const_iterator;
  template<typename ValueTy>
  class dictionary_iterator;
  template<typename ValueTy>
  class dictionary_key_iterator;


  class string_pool;


  template <typename Val>
  class dictionary_entry final : public impl::dictionary_entry_storage<Val> {

    static size_t allocSize(size_t keySize) noexcept {
      return sizeof(dictionary_entry) + keySize + 1;
    }
    static size_t allocAlign() noexcept {
      return alignof(dictionary_entry);
    }

  public:
    using impl::dictionary_entry_storage<Val>::dictionary_entry_storage;

    AGT_nodiscard std::string_view key() const {
      return std::string_view(get_key_data(), this->get_key_length());
    }

    /// get_key_data - Return the start of the string data that is the key for this
    /// value.  The string data is always stored immediately after the
    /// dictionary_entry object.
    AGT_nodiscard const char* get_key_data() const {
      return reinterpret_cast<const char*>(this + 1);
    }

    /// Create a dictionary_entry for the specified key construct the value using
    /// \p InitiVals.
    template <typename Alloc, typename... Args>
    static dictionary_entry* create(Alloc& allocator, std::string_view key, Args&& ...args) {
      size_t keyLength = key.length();
      

      auto newMemory = allocator.allocate(allocSize(keyLength), allocAlign());
      
      assert(newMemory && "Unhandled \"out of memory\" error");

      // Construct the value.
      auto *newItem = new (newMemory) dictionary_entry(keyLength, std::forward<Args>(args)...);

      // Copy the string information.
      auto strBuffer = const_cast<char*>(newItem->get_key_data());
      if (keyLength > 0)
        std::memcpy(strBuffer, key.data(), keyLength);
      strBuffer[keyLength] = 0;// Null terminate for convenience of clients.
      return newItem;
    }

    /// Getdictionary_entryFromKeyData - Given key data that is known to be embedded
    /// into a dictionary_entry, return the dictionary_entry itself.
    static dictionary_entry& get_dictionary_entry_from_key_data(const char* keyData) {
      auto ptr = const_cast<char *>(keyData) - sizeof(dictionary_entry<Val>);
      return *reinterpret_cast<dictionary_entry *>(ptr);
    }

    /// Destroy - Destroy this dictionary_entry, releasing memory back to the
    /// specified allocator.
    template <typename Alloc>
    void destroy(Alloc& allocator) {
      size_t size = allocSize(this->get_key_length());
      // Free memory referenced by the item.
      this->~dictionary_entry();
      allocator.deallocate(this, size, allocAlign());
    }
  };


  template <typename T, typename EntryAllocator = default_allocator>
  class dictionary : public impl::dictionary, private EntryAllocator {

    using entry_allocator_t = EntryAllocator;

    friend class string_pool;

    entry_allocator_t& get_allocator() const noexcept {
      return const_cast<entry_allocator_t&>(static_cast<const entry_allocator_t&>(*this));
    }

  public:
    using entry_type = dictionary_entry<T>;

    dictionary() noexcept
        : impl::dictionary(static_cast<agt_u32_t>(sizeof(entry_type))),
          entry_allocator_t() {}

    explicit dictionary(agt_u32_t InitialSize)
        : impl::dictionary(static_cast<agt_u32_t>(sizeof(entry_type))),
          entry_allocator_t()  {
      explicit_init(InitialSize);
    }

    dictionary(std::initializer_list<std::pair<std::string_view, T>> List)
        : impl::dictionary(static_cast<agt_u32_t>(sizeof(entry_type))),
          entry_allocator_t()  {
      explicit_init(List.size());
      for (const auto &P : List) {
        insert(P);
      }
    }

    dictionary(dictionary &&RHS) noexcept
        : impl::dictionary(std::move(RHS)),
          entry_allocator_t(std::move(RHS.get_allocator()))  {}

    dictionary(const dictionary &RHS)
        : impl::dictionary(static_cast<agt_u32_t>(sizeof(entry_type))),
          entry_allocator_t(RHS.get_allocator()) {
      if (RHS.empty())
        return;

      // Allocate TheTable of the same size as RHS's TheTable, and set the
      // sentinel appropriately (and NumBuckets).
      init(RHS.NumBuckets);
      auto* HashTable    = (agt_u32_t*)(TheTable + NumBuckets + 1);
      auto* RHSHashTable = (agt_u32_t*)(RHS.TheTable + NumBuckets + 1);

      NumItems = RHS.NumItems;
      NumTombstones = RHS.NumTombstones;

      for (agt_u32_t I = 0, E = NumBuckets; I != E; ++I) {
        impl::dictionary_entry_base *Bucket = RHS.TheTable[I];
        if (!Bucket || Bucket == get_tombstone_val()) {
          TheTable[I] = Bucket;
          continue;
        }

        TheTable[I] = entry_type::create(
          get_allocator(),
          static_cast<entry_type *>(Bucket)->key(),
          static_cast<entry_type *>(Bucket)->getValue());
        HashTable[I] = RHSHashTable[I];
      }
    }

    dictionary& operator=(dictionary&& RHS) noexcept {
      impl::dictionary::swap(RHS);
      std::swap(get_allocator(), RHS.get_allocator());
      return *this;
    }

    ~dictionary() {
      // Delete all the elements in the map, but don't reset the elements
      // to default values.  This is a copy of clear(), but avoids unnecessary
      // work not required in the destructor.
      if (!empty()) {
        for (unsigned I = 0, E = NumBuckets; I != E; ++I) {
          impl::dictionary_entry_base *Bucket = TheTable[I];
          if (Bucket && Bucket != get_tombstone_val()) {
            static_cast<entry_type *>(Bucket)->destroy(get_allocator());
          }
        }
      }
      this->dealloc_table();
    }

    using mapped_type = T;
    using value_type = dictionary_entry<T>;
    using size_type = size_t;

    using const_iterator = dictionary_const_iterator<T>;
    using iterator = dictionary_iterator<T>;

    iterator begin() noexcept { return iterator(TheTable, NumBuckets == 0); }
    iterator end() noexcept { return iterator(TheTable + NumBuckets, true); }
    const_iterator begin() const noexcept {
      return const_iterator(TheTable, !NumBuckets);
    }
    const_iterator end() const noexcept {
      return const_iterator(TheTable + NumBuckets, true);
    }

    range_view<dictionary_key_iterator<T>> keys() const noexcept {
      return { dictionary_key_iterator<T>(begin()),
              dictionary_key_iterator<T>(end()) };
    }

    iterator find(std::string_view Key) {
      int Bucket = find_key(Key);
      if (Bucket == -1)
        return end();
      return iterator(TheTable + Bucket, true);
    }

    const_iterator find(std::string_view Key) const {
      int Bucket = find_key(Key);
      if (Bucket == -1)
        return end();
      return const_iterator(TheTable + Bucket, true);
    }

    /// lookup - Return the entry for the specified key, or a default
    /// constructed value if no such entry exists.
    T lookup(std::string_view Key) const {
      const_iterator it = find(Key);
      if (it != end())
        return it->get();
      return T();
    }

    /// Lookup the ValueTy for the \p Key, or create a default constructed value
    /// if the key is not in the map.
    T& operator[](std::string_view Key) noexcept { return try_emplace(Key).first->get(); }

    /// count - Return 1 if the element is in the map, 0 otherwise.
    AGT_nodiscard size_type count(std::string_view Key) const { return find(Key) == end() ? 0 : 1; }



    template<typename InputTy>
    AGT_nodiscard size_type count(const dictionary_entry<InputTy>& entry) const {
      return count(entry.key());
    }

    AGT_nodiscard bool contains(std::string_view key) const noexcept {
      return find(key) != end();
    }

    /// equal - check whether both of the containers are equal.
    bool operator==(const dictionary &RHS) const {
      if (size() != RHS.size())
        return false;

      for (const auto &KeyValue : *this) {
        auto FindInRHS = RHS.find(KeyValue.key());

        if (FindInRHS == RHS.end())
          return false;

        if (KeyValue.get() != FindInRHS->get())
          return false;
      }

      return true;
    }

    /// insert - Insert the specified key/value pair into the map.  If the key
    /// already exists in the map, return false and ignore the request, otherwise
    /// insert it and return true.
    bool insert(entry_type *KeyValue) {
      unsigned BucketNo = lookup_bucket_for(KeyValue->key());
      impl::dictionary_entry_base *&Bucket = TheTable[BucketNo];
      if (Bucket && Bucket != get_tombstone_val())
        return false;// Already exists in map.

      if (Bucket == get_tombstone_val())
        --NumTombstones;
      Bucket = KeyValue;
      ++NumItems;
      assert(NumItems + NumTombstones <= NumBuckets);

      rehash_table();
      return true;
    }

    /// insert - Inserts the specified key/value pair into the map if the key
    /// isn't already in the map. The bool component of the returned pair is true
    /// if and only if the insertion takes place, and the iterator component of
    /// the pair points to the element with key equivalent to the key of the pair.
    std::pair<iterator, bool> insert(std::pair<std::string_view, T> KV) {
      return try_emplace(KV.first, std::move(KV.second));
    }

    /// Inserts an element or assigns to the current element if the key already
    /// exists. The return type is the same as try_emplace.
    template<typename V>
    std::pair<iterator, bool> insert_or_assign(std::string_view Key, V &&Val) {
      auto Ret = try_emplace(Key, std::forward<V>(Val));
      if (!Ret.second)
        Ret.first->get() = std::forward<V>(Val);
      return Ret;
    }

    /// Emplace a new element for the specified key into the map if the key isn't
    /// already in the map. The bool component of the returned pair is true
    /// if and only if the insertion takes place, and the iterator component of
    /// the pair points to the element with key equivalent to the key of the pair.
    template<typename... ArgsTy>
    std::pair<iterator, bool> try_emplace(std::string_view Key, ArgsTy &&...Args) {
      unsigned BucketNo = lookup_bucket_for(Key);
      impl::dictionary_entry_base *&Bucket = TheTable[BucketNo];
      if (Bucket && Bucket != get_tombstone_val())
        return std::make_pair(iterator(TheTable + BucketNo, false),
                              false);// Already exists in map.

      if (Bucket == get_tombstone_val())
        --NumTombstones;
      Bucket = entry_type::create(get_allocator(), Key, std::forward<ArgsTy>(Args)...);
      ++NumItems;
      assert(NumItems + NumTombstones <= NumBuckets);

      BucketNo = rehash_table(BucketNo);
      return std::make_pair(iterator(TheTable + BucketNo, false), true);
    }

    // clear - Empties out the dictionary
    void clear() {
      if (empty())
        return;

      // Zap all values, resetting the keys back to non-present (not tombstone),
      // which is safe because we're removing all elements.
      for (unsigned I = 0, E = NumBuckets; I != E; ++I) {
        impl::dictionary_entry_base *&Bucket = TheTable[I];
        if (Bucket && Bucket != get_tombstone_val()) {
          static_cast<entry_type *>(Bucket)->destroy(get_allocator());
        }
        Bucket = nullptr;
      }

      NumItems = 0;
      NumTombstones = 0;
    }

    /// remove - Remove the specified key/value pair from the map, but do not
    /// erase it.  This aborts if the key is not in the map.
    void remove(entry_type *KeyValue) { remove_key(KeyValue); }

    void erase(iterator I) {
      entry_type &V = *I;
      remove(&V);
      V.destroy(get_allocator());
    }

    bool erase(std::string_view Key) {
      iterator I = find(Key);
      if (I == end())
        return false;
      erase(I);
      return true;
    }
  };


  template<typename T>
  class dictionary_const_iterator : public impl::dictionary_iter_base<
                                      dictionary_const_iterator<T>,
                                      const dictionary_entry<T>> {
    using base = impl::dictionary_iter_base<dictionary_const_iterator<T>, const dictionary_entry<T>>;

  public:
    using base::base;

    const dictionary_entry<T>& operator*() const noexcept {
      return *static_cast<const dictionary_entry<T>*>(*this->Ptr);
    }
  };

  template<typename T>
  class dictionary_iterator : public impl::dictionary_iter_base<
                                dictionary_iterator<T>,
                                dictionary_entry<T>> {
    using base = impl::dictionary_iter_base<dictionary_iterator<T>, dictionary_entry<T>>;

  public:
    using base::base;

    dictionary_entry<T>& operator*() const noexcept {
      return *static_cast<dictionary_entry<T>*>(*this->Ptr);
    }

    operator dictionary_const_iterator<T>() const noexcept {
      return dictionary_const_iterator<T>(this->Ptr, true);
    }
  };

  template<typename T>
  class dictionary_key_iterator : public iterator_adaptor_base<
                                    dictionary_key_iterator<T>,
                                    dictionary_const_iterator<T>,
                                    std::forward_iterator_tag,
                                    std::string_view>{
    using base = iterator_adaptor_base<dictionary_key_iterator, dictionary_const_iterator<T>, std::forward_iterator_tag, std::string_view>;
  public:

    dictionary_key_iterator() = default;
    explicit dictionary_key_iterator(dictionary_const_iterator<T> iter) noexcept : base(std::move(iter)){}


    std::string_view& operator*() const noexcept {
      cache_key = this->wrapped()->key();
      return cache_key;
    }

  private:
    mutable std::string_view cache_key;
  };

}

#endif//JEMSYS_INTERNAL_DICTIONARY_HPP
