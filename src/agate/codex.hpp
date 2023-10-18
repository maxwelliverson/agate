//
// Created by maxwe on 2023-08-16.
//

#ifndef AGATE_SUPPORT_CODEX_HPP
#define AGATE_SUPPORT_CODEX_HPP

#include "agate.h"

#include "align.hpp"
#include "iterator.hpp"
#include "allocator.hpp"

#include <string_view>
#include <memory>
#include <bit>

// See <dictionary.hpp> for intended usage.

// Essentially a dictionary optimized for string values.

namespace agt {
  namespace impl {
    inline constexpr static size_t OptimalStringAlign = 16;

    class codex;

    class codex_entry_base {

      friend class impl::codex;

      uint64_t m_keyLength;
      uint32_t m_valueLength;
      uint32_t m_valueOffset;
    protected:

      inline static size_t _get_key_offset() noexcept {
        return align_to<OptimalStringAlign>(sizeof(codex_entry_base));
      }

      inline static size_t _get_value_offset(size_t minKeyLength) noexcept {
        return align_to<OptimalStringAlign>(_get_key_offset() + minKeyLength);
      }

      inline static size_t _get_alloc_size(size_t minKeyLength, size_t minValueLength) noexcept {
        return align_to<OptimalStringAlign>(_get_value_offset(minKeyLength) + minValueLength);
      }

      [[nodiscard]] inline const void* _get_key_data() const noexcept {
        return std::assume_aligned<OptimalStringAlign>(reinterpret_cast<const char*>(this) + _get_key_offset());
      }

      [[nodiscard]] inline const void* _get_value_data() const noexcept {
        return std::assume_aligned<OptimalStringAlign>(reinterpret_cast<const char*>(this) + m_valueOffset);
      }


      void _set_value_length(size_t len) noexcept {
        m_valueLength = static_cast<uint32_t>(len);
      }

    public:
      codex_entry_base(size_t keyLength, size_t valueLength, size_t minKeyLength)
          : m_keyLength(keyLength),
            m_valueLength(valueLength),
            m_valueOffset(_get_value_offset(minKeyLength))
      {}

      codex_entry_base(codex_entry_base &e) = delete;


      AGT_nodiscard inline size_t get_value_length() const noexcept {
        return m_valueLength;
      }

      AGT_nodiscard inline size_t get_key_length() const noexcept {
        return m_keyLength;
      }
    };

    class codex {

      inline bool _key_matches_entry(const void* keyData, size_t keyLength, codex_entry_base* entry) const noexcept;

    protected:

      // Array of NumBuckets pointers to entries, null pointers are holes.
      // TheTable[NumBuckets] contains a sentinel value for easy iteration. Followed
      // by an array of the actual hash values as unsigned integers.
      codex_entry_base ** m_table          = nullptr;
      agt_u32_t         m_bucketCount    = 0;
      agt_u32_t         m_entryCount     = 0;
      agt_u32_t         m_tombstoneCount = 0;
      agt_u32_t         m_keyCharSize    = 0;

    protected:
      explicit codex(size_t keyCharSize) noexcept : m_keyCharSize(static_cast<uint32_t>(keyCharSize)) { }
      codex(codex && other) noexcept
          : m_table(std::exchange(other.m_table, nullptr)),
            m_bucketCount(std::exchange(other.m_bucketCount, 0)),
            m_entryCount(std::exchange(other.m_entryCount, 0)),
            m_tombstoneCount(std::exchange(other.m_tombstoneCount, 0)),
            m_keyCharSize(other.m_keyCharSize)
      { }

      /// Allocate the table with the specified number of buckets and otherwise
      /// setup the map as empty.
      void _init(agt_u32_t size) noexcept;
      void _explicit_init(agt_u32_t size) noexcept;

      agt_u32_t _rehash_table(agt_u32_t bucketIndex) noexcept;

      /// lookup_bucket_for - Look up the bucket that the specified string should end
      /// up in.  If it already exists as a key in the map, the Item pointer for the
      /// specified bucket will be non-null.  Otherwise, it will be null.  In either
      /// case, the FullHashValue field of the bucket will be set to the hash value
      /// of the string.
      agt_u32_t _lookup_bucket_for(const void* keyData, size_t keyLength) noexcept;

      /// find_key - Look up the bucket that contains the specified key. If it exists
      /// in the map, return the bucket number of the key.  Otherwise return -1.
      /// This does not modify the map.
      AGT_nodiscard agt_i32_t _find_key(const void* keyData, size_t keyLength) const noexcept;

      /// remove_key - Remove the specified dictionary_entry from the table, but do not
      /// delete it.  This aborts if the value isn't in the table.
      void _remove_key(codex_entry_base * v) noexcept;

      /// remove_key - Remove the dictionary_entry for the specified key from the
      /// table, returning it.  If the key is not in the table, this returns null.
      codex_entry_base * _remove_key(const void* keyData, size_t keyLength) noexcept;


      void _dealloc_table() noexcept;


    public:
      inline static codex_entry_base * get_tombstone_val() noexcept {
        constexpr static agt_u32_t free_low_bits = std::countr_zero(OptimalStringAlign);
        auto Val = static_cast<uintptr_t>(-1);
        Val <<= free_low_bits;
        return reinterpret_cast<codex_entry_base *>(Val);
      }

      AGT_nodiscard agt_u32_t  get_bucket_count() const noexcept { return m_bucketCount; }
      AGT_nodiscard agt_u32_t  get_entry_count() const noexcept { return m_entryCount; }

      AGT_nodiscard bool empty() const noexcept { return m_entryCount == 0; }
      AGT_nodiscard agt_u32_t  size() const noexcept { return m_entryCount; }

      void swap(codex & other) noexcept {
        std::swap(m_table, other.m_table);
        std::swap(m_bucketCount, other.m_bucketCount);
        std::swap(m_entryCount, other.m_entryCount);
        std::swap(m_tombstoneCount, other.m_tombstoneCount);
      }
    };

    template <typename DerivedTy, typename T>
    class codex_iter_base : public iterator_facade_base<DerivedTy, std::forward_iterator_tag, T> {
    protected:
      impl::codex_entry_base** m_ptr = nullptr;

      using derived_type = DerivedTy;

    public:
      codex_iter_base() = default;

      explicit codex_iter_base(impl::codex_entry_base** bucket, bool noAdvance = false)
          : m_ptr(bucket) {
        if (!noAdvance)
          advance_past_empty();
      }

      derived_type& operator=(const derived_type& other) {
        m_ptr = other.m_ptr;
        return static_cast<derived_type &>(*this);
      }



      derived_type& operator++() noexcept {// Preincrement
        ++m_ptr;
        advance_past_empty();
        return static_cast<derived_type &>(*this);
      }

      derived_type operator++(int) noexcept {// Post-increment
        derived_type tmp(m_ptr);
        ++*this;
        return tmp;
      }


      friend bool operator==(const derived_type& a, const derived_type& b) noexcept {
        return a.m_ptr == b.m_ptr;
      }

    private:



      void advance_past_empty() {
        while (*m_ptr == nullptr || *m_ptr == impl::codex::get_tombstone_val())
          ++m_ptr;
      }
    };
  }

  template <typename K, typename V = K>
  class basic_codex_const_iterator;
  template <typename K, typename V = K>
  class basic_codex_iterator;
  template <typename K, typename V = K>
  class basic_codex_key_iterator;



  template <typename K, typename V = K>
  class alignas(impl::OptimalStringAlign) basic_codex_entry final : public impl::codex_entry_base {
    static size_t allocSize(size_t keySize, size_t valueSize) noexcept {
      return impl::codex_entry_base::_get_alloc_size((keySize + 1) * sizeof(K), (valueSize + 1) * sizeof(V));
    }
    static size_t allocAlign() noexcept {
      return alignof(basic_codex_entry);
    }
  public:

    using key_char_type = K;
    using mapped_char_type = V;
    using key_type    = std::basic_string_view<K>;
    using mapped_type = std::basic_string_view<V>;

    basic_codex_entry(size_t keyLength, size_t valueLength) noexcept
        : impl::codex_entry_base(keyLength, valueLength, (keyLength + 1) * sizeof(K)) { }


    AGT_nodiscard key_type key() const noexcept {
      return key_type(get_key_data(), this->get_key_length());
    }

    AGT_nodiscard const key_char_type* get_key_data() const noexcept {
      return static_cast<const key_char_type*>(this->_get_key_data());
    }

    AGT_nodiscard mapped_type value() const noexcept {
      return { get_value_data(), this->get_value_length() };
    }

    AGT_nodiscard const mapped_char_type* get_value_data() const noexcept {
      return static_cast<const mapped_char_type*>(this->_get_value_data());
    }


    AGT_nodiscard size_t get_entry_size() const noexcept {
      return allocSize(get_key_length(), get_value_length());
    }


    static basic_codex_entry * create(key_type key, mapped_type value) noexcept {
      size_t keyLength = key.length();
      size_t valueLength = value.length();

      size_t reqAllocSize = allocSize(keyLength, valueLength);

      auto newMemory = alloc_aligned(reqAllocSize, allocAlign());



      assert(newMemory && "Unhandled 'out of memory' error");

      auto* newEntry = new (newMemory) basic_codex_entry(keyLength, valueLength);

      auto  keyBuffer = const_cast<K*>(newEntry->get_key_data());
      auto  valBuffer = const_cast<V*>(newEntry->get_value_data());

      if (keyLength > 0)
        std::memcpy(keyBuffer, key.data(), keyLength * sizeof(K));
      if (valueLength > 0)
        std::memcpy(valBuffer, value.data(), valueLength * sizeof(V));
      keyBuffer[keyLength] = 0;
      valBuffer[valueLength] = 0;
      return newEntry;
    }

    static basic_codex_entry * create(key_type key, size_t valLength = 0) noexcept {
      size_t keyLength = key.length();

      size_t reqAllocSize = allocSize(keyLength, valLength);

      auto newMemory = alloc_aligned(reqAllocSize, allocAlign());



      assert(newMemory && "Unhandled 'out of memory' error");

      auto* newEntry = new (newMemory) basic_codex_entry(keyLength, valLength);

      auto  keyBuffer = const_cast<K*>(newEntry->get_key_data());

      if (keyLength > 0)
        std::memcpy(keyBuffer, key.data(), keyLength * sizeof(K));
      keyBuffer[keyLength] = 0;
      return newEntry;
    }

    basic_codex_entry *        assign_value(mapped_type value) noexcept {
      size_t maxValueStringMemorySize = align_to<impl::OptimalStringAlign>((get_value_length() + 1) * sizeof(V));
      size_t reqSize = (value.length() + 1) * sizeof(V);

      basic_codex_entry * entry = this;

      if (reqSize > maxValueStringMemorySize) {
        size_t oldAllocSize = get_entry_size();
        size_t newAllocSize = allocSize(get_key_length(), value.length());
        entry = static_cast<basic_codex_entry *>(realloc_aligned(entry, newAllocSize, oldAllocSize, allocAlign()));
      }

      entry->_set_value_length(value.length());
      auto* valBuffer = const_cast<V*>(entry->get_value_data());

      if (value.length() > 0) [[likely]]
        std::memcpy(valBuffer, value.data(), value.length() * sizeof(V));
      valBuffer[value.size()] = 0;
      return entry;
    }

    void destroy() noexcept {
      free_aligned(this, allocSize(get_key_length(), get_value_length()), allocAlign());
    }
  };


  template <typename KeyChar, typename ValChar = KeyChar>
  class basic_codex : public impl::codex {
    class reference_proxy;
  public:

    using key_char_type = KeyChar;
    using mapped_char_type = ValChar;
    using key_type = std::basic_string_view<KeyChar>;
    using mapped_type = std::basic_string_view<ValChar>;
    using entry_type = basic_codex_entry<KeyChar, ValChar>;
    using size_type = uint32_t;

    using reference = reference_proxy;

    using const_iterator = basic_codex_const_iterator<KeyChar, ValChar>;
    using iterator = basic_codex_iterator<KeyChar, ValChar>;
    using key_iterator = basic_codex_key_iterator<KeyChar, ValChar>;

  public:




    basic_codex() noexcept
        : impl::codex(sizeof(KeyChar)) { }


    explicit basic_codex(agt_u32_t initialSize)
        : impl::codex(sizeof(KeyChar)) {
      this->_explicit_init(initialSize);
    }

    basic_codex(std::initializer_list<std::pair<key_type, mapped_type>> list)
        : impl::codex(sizeof(KeyChar)) {
      _explicit_init(list.size());
      for (const auto& entry : list)
        insert(entry);
    }

    basic_codex(basic_codex&& other) noexcept
        : impl::codex(std::move(other)) { }

    basic_codex(const basic_codex& other)
        : impl::codex(sizeof(KeyChar)) {

      if (other.empty())
        return;

      this->_init(other.m_bucketCount);
      auto* hashTable = (uint32_t*)(m_table + m_bucketCount + 1);
      auto* otherHashTable = (uint32_t*)(other.m_table + m_bucketCount + 1);

      m_entryCount     = other.m_entryCount;
      m_tombstoneCount = other.m_tombstoneCount;

      const uint32_t end = m_bucketCount;
      for (uint32_t i = 0; i != end; ++i) {
        auto bucket = other.m_table[i];
        if (!bucket || bucket == get_tombstone_val()) {
          m_table[i] = bucket;
          continue;
        }

        auto entry = static_cast<entry_type*>(bucket);
        m_table[i] = entry_type::create(entry->key(), entry->value());
        hashTable[i] = otherHashTable[i];
      }
    }

    basic_codex& operator=(basic_codex&& other) noexcept {
      impl::codex::swap(other);
      return *this;
    }

    ~basic_codex() {
      if (!empty()) {
        uint32_t entryCount = m_entryCount;
        const auto end = m_bucketCount;
        for (uint32_t i = 0; i < end; ++i) {
          auto bucket = m_table[i];
          if (bucket && bucket != get_tombstone_val()) {
            static_cast<entry_type*>(bucket)->destroy();
            if (--entryCount == 0)
              break;
          }
        }
        assert( entryCount == 0 && "Codex entries unaccounted for?");
      }
      this->_dealloc_table();
    }





    iterator begin() noexcept { return iterator(m_table, m_bucketCount == 0); }
    iterator end() noexcept { return iterator(m_table + m_bucketCount, true); }
    const_iterator begin() const noexcept {
      return const_iterator(m_table, m_bucketCount == 0);
    }
    const_iterator end() const noexcept {
      return const_iterator(m_table + m_bucketCount, true);
    }

    range_view<key_iterator> keys() const noexcept {
      return { key_iterator(begin()), key_iterator(end()) };
    }

    iterator       find(key_type key) noexcept {
      int bucketIndex = this->_find_key(key.data(), key.length());
      if (bucketIndex == -1)
        return end();
      return iterator(m_table + bucketIndex, true);
    }

    const_iterator find(key_type key) const noexcept {
      int bucketIndex = this->_find_key(key.data(), key.length());
      if (bucketIndex == -1)
        return end();
      return const_iterator(m_table + bucketIndex, true);
    }

    /// lookup - Return the entry for the specified key, or a default
    /// constructed value if no such entry exists.
    mapped_type lookup(key_type key) const noexcept {
      int bucketIndex = this->_find_key(key.data(), key.length());
      if (bucketIndex == -1)
        return {};
      return static_cast<entry_type*>(m_table[bucketIndex])->value();
    }

    /// Lookup the ValueTy for the \p Key, or create a default constructed value
    /// if the key is not in the map.
    reference operator[](key_type key) noexcept {
      unsigned bucketIndex = _lookup_bucket_for(key.data(), key.length());
      impl::codex_entry_base*& bucket = m_table[bucketIndex];
      if (bucket && bucket != get_tombstone_val())
        return reference_proxy(&bucket);

      if (bucket == get_tombstone_val())
        --m_tombstoneCount;

      bucket = entry_type::create(key);
      ++m_entryCount;
      assert(m_entryCount + m_tombstoneCount <= m_bucketCount);

      bucketIndex = _rehash_table(bucketIndex);

      return reference_proxy(m_table + bucketIndex);
    }

    /// count - Return 1 if the element is in the map, 0 otherwise.
    AGT_nodiscard size_type count(key_type key) const noexcept {
      return (_find_key(key.data(), key.size()) == -1) ? 0 : 1;
    }



    /*template <typename InputTy>
    AGT_nodiscard size_type count(const dictionary_entry<InputTy>& entry) const {
      return count(entry.key());
    }*/

    AGT_nodiscard bool contains(key_type key) const noexcept {
      return _find_key(key.data(), key.length()) != -1;
    }

    /// equal - check whether both of the containers are equal.
    friend bool operator==(const basic_codex& a, const basic_codex& b) noexcept {
      if (a.size() != b.size())
        return false;

      for (const auto& kv : a) {
        int bIndex = b._find_key(kv.key().data(), kv.key().length());
        if (bIndex == -1)
          return false;

        if (kv.value() != static_cast<entry_type*>(b.m_table[bIndex])->value())
          return false;
      }

      return true;
    }

    /// insert - Insert the specified key/value pair into the map.  If the key
    /// already exists in the map, return false and ignore the request, otherwise
    /// insert it and return true.
    bool insert(entry_type* entry) noexcept {
      uint32_t bucketIndex = _lookup_bucket_for(entry->key().data(), entry->key().length());
      impl::codex_entry_base*& bucket = m_table[bucketIndex];

      if (bucket && bucket != get_tombstone_val())
        return false; // already in codex

      if (bucket == get_tombstone_val())
        --m_tombstoneCount;

      bucket = entry;
      ++m_entryCount;

      assert(m_entryCount + m_tombstoneCount <= m_bucketCount);

      _rehash_table();
      return true;
    }

    /// insert - Inserts the specified key/value pair into the map if the key
    /// isn't already in the map. The bool component of the returned pair is true
    /// if and only if the insertion takes place, and the iterator component of
    /// the pair points to the element with key equivalent to the key of the pair.
    std::pair<iterator, bool> insert(const std::pair<key_type, mapped_type>& kv) noexcept {
      return this->try_emplace(kv.first, kv.second);
    }

    /// Inserts an element or assigns to the current element if the key already
    /// exists. The return type is the same as try_emplace.
    template <std::convertible_to<mapped_type> V>
    std::pair<iterator, bool> insert_or_assign(key_type key, V&& value) noexcept {
      auto ret = try_emplace(key, std::forward<V>(value));
      if (!ret.second)
        ret.first.assign(mapped_type(std::forward<V>(value)));
      return ret;
    }

    /// Emplace a new element for the specified key into the map if the key isn't
    /// already in the map. The bool component of the returned pair is true
    /// if and only if the insertion takes place, and the iterator component of
    /// the pair points to the element with key equivalent to the key of the pair.
    template <typename... Args> requires std::constructible_from<mapped_type, Args...>
    std::pair<iterator, bool> try_emplace(key_type key, Args&& ...args) noexcept {
      unsigned bucketIndex = _lookup_bucket_for(key.data(), key.length());
      impl::codex_entry_base*& bucket = m_table[bucketIndex];
      if (bucket && bucket != get_tombstone_val())
        return std::make_pair(iterator(m_table + bucketIndex, false), false);

      if (bucket == get_tombstone_val())
        --m_tombstoneCount;

      bucket = entry_type::create(key, mapped_type(std::forward<Args>(args)...));
      ++m_entryCount;
      assert(m_entryCount + m_tombstoneCount <= m_bucketCount);

      bucketIndex = _rehash_table(bucketIndex);
      return std::make_pair(iterator(m_table + bucketIndex, false), true);
    }

    // clear - Empties out the dictionary
    void clear() noexcept {
      if (empty())
        return;

      const auto end = m_bucketCount;
      for (uint32_t i = 0; i < end; ++i) {
        auto& bucket = m_table[i];
        if (bucket && bucket != get_tombstone_val())
          static_cast<entry_type*>(bucket)->destroy();
        bucket = nullptr;
      }

      m_entryCount = 0;
      m_tombstoneCount = 0;
    }

    /// remove - Remove the specified key/value pair from the map, but do not
    /// erase it.  This aborts if the key is not in the map.
    void remove(entry_type* entry) noexcept {
      this->_remove_key(entry);
    }

    void erase(iterator i) noexcept {
      entry_type& entry = *i;
      remove(&entry);
      entry.destroy();
    }

    bool erase(key_type key) noexcept {
      int bucketIndex = this->_find_key(key.data(), key.length());
      if (bucketIndex == -1)
        return false;
      auto entry = *(entry_type**)(m_table + bucketIndex);
      this->remove(entry);
      entry->destroy();
      return true;
    }

  private:
    class reference_proxy {

      friend class basic_codex;

      entry_type*& m_entryPos;

      reference_proxy(impl::codex_entry_base** pos) noexcept
          : m_entryPos(*(entry_type**)pos) { }

    public:

      reference_proxy(const reference_proxy&) = delete;
      reference_proxy(reference_proxy&&) noexcept = delete;

      reference_proxy& operator=(mapped_type value) noexcept {
        m_entryPos = m_entryPos->assign_value(value);
        return *this;
      }

      operator mapped_type() const noexcept {
        return m_entryPos->value();
      }
    };
  };



  template <typename K, typename V>
  class basic_codex_const_iterator : public impl::codex_iter_base<basic_codex_const_iterator<K, V>, const basic_codex_entry<K, V>> {
  public:
    using entry_type = basic_codex_entry<K, V>;
  private:
    using base = impl::codex_iter_base<basic_codex_const_iterator, const entry_type>;

  public:
    using base::base;

    const entry_type& operator*() const noexcept {
      return *static_cast<const entry_type*>(*this->m_ptr);
    }
  };

  template <typename K, typename V>
  class basic_codex_iterator : public impl::codex_iter_base<basic_codex_iterator<K, V>, basic_codex_entry<K, V>> {
  public:
    using entry_type = basic_codex_entry<K, V>;
  private:
    using base = impl::codex_iter_base<basic_codex_iterator, entry_type>;

  public:
    using base::base;

    entry_type& operator*() const noexcept {
      return *static_cast<entry_type*>(*this->m_ptr);
    }

    void assign(std::basic_string_view<V> value) noexcept {
      *this->m_ptr = static_cast<entry_type*>(*this->m_ptr)->assign_value(value);
    }

    operator basic_codex_const_iterator<K, V>() const noexcept {
      return basic_codex_const_iterator<K, V>(this->m_ptr, true);
    }
  };

  template <typename K, typename V>
  class basic_codex_key_iterator : public iterator_adaptor_base<basic_codex_key_iterator<K, V>, basic_codex_const_iterator<K, V>, std::forward_iterator_tag, std::basic_string_view<K>> {
    using base = iterator_adaptor_base<basic_codex_key_iterator<K, V>, basic_codex_const_iterator<K, V>, std::forward_iterator_tag, std::basic_string_view<K>>;
  public:

    using key_type = std::basic_string_view<K>;

    basic_codex_key_iterator() = default;
    explicit basic_codex_key_iterator(basic_codex_const_iterator<K, V> iter) noexcept : base(std::move(iter)){}


    key_type& operator*() const noexcept {
      m_cachedKey = this->wrapped()->key();
      return m_cachedKey;
    }

  private:
    mutable key_type m_cachedKey;
  };





  using codex   = basic_codex<char>;
  using wcodex  = basic_codex<wchar_t>;
  using cwcodex = basic_codex<char, wchar_t>;
  using wccodex = basic_codex<wchar_t, char>;

  using codex_entry   = basic_codex_entry<char>;
  using wcodex_entry  = basic_codex_entry<wchar_t>;
  using cwcodex_entry = basic_codex_entry<char, wchar_t>;
  using wccodex_entry = basic_codex_entry<wchar_t, char>;


  using codex_iterator = basic_codex_iterator<char>;
  using codex_const_iterator = basic_codex_const_iterator<char>;
  using codex_key_iterator = basic_codex_key_iterator<char>;

  using wcodex_iterator = basic_codex_iterator<wchar_t>;
  using wcodex_const_iterator = basic_codex_const_iterator<wchar_t>;
  using wcodex_key_iterator = basic_codex_key_iterator<wchar_t>;

  using cwcodex_iterator = basic_codex_iterator<char, wchar_t>;
  using cwcodex_const_iterator = basic_codex_const_iterator<char, wchar_t>;
  using cwcodex_key_iterator = basic_codex_key_iterator<char, wchar_t>;

  using wccodex_iterator = basic_codex_iterator<wchar_t, char>;
  using wccodex_const_iterator = basic_codex_const_iterator<wchar_t, char>;
  using wccodex_key_iterator = basic_codex_key_iterator<wchar_t, char>;
}

#endif//AGATE_SUPPORT_CODEX_HPP
