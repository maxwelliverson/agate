//
// Created by maxwe on 2022-08-07.
//

#ifndef AGATE_SUPPORT_SET_HPP
#define AGATE_SUPPORT_SET_HPP

#include "map.hpp"

namespace agt {
  
  namespace impl {
    
    template <std::unsigned_integral I>
    inline static I powerOf2Ceil(I i) noexcept {
      if (!i)
        return static_cast<I>(0);
      return std::bit_ceil(i);
    }
    
    
    template <typename ValueType, typename MapType, typename ValueTypeInfo>
    class basic_set {
      
      using map_type = MapType;
      
      static_assert(sizeof(typename map_type::value_type) == sizeof(ValueType),
                    "DenseMap buckets unexpectedly large!");
      map_type mapInst;
      
      template <typename T>
      using const_arg_type_t = typename const_pointer_or_const_ref<T>::type;

    public:
      using key_type   = ValueType;
      using value_type = ValueType;
      using size_type  = uint32_t;

      explicit basic_set(unsigned InitialReserve = 0) : mapInst(InitialReserve) {}

      template <typename InputIt>
      basic_set(const InputIt &I, const InputIt &E) noexcept
          : basic_set(powerOf2Ceil(std::distance(I, E))) {
        insert(I, E);
      }

      basic_set(std::initializer_list<ValueType> Elems) noexcept
          : basic_set(powerOf2Ceil(Elems.size())) {
        insert(Elems.begin(), Elems.end());
      }

      AGT_nodiscard bool empty() const noexcept { return mapInst.empty(); }
      AGT_nodiscard size_type size() const noexcept { return mapInst.size(); }
      AGT_nodiscard size_t getMemorySize() const noexcept { return mapInst.getMemorySize(); }

      /// Grow the DenseSet so that it has at least Size buckets. Will not shrink
      /// the Size of the set.
      void resize(size_t Size) noexcept { mapInst.resize(Size); }

      /// Grow the DenseSet so that it can contain at least \p NumEntries items
      /// before resizing again.
      void reserve(size_t Size) noexcept { mapInst.reserve(Size); }

      void clear() noexcept {
        mapInst.clear();
      }

      /// Return 1 if the specified key is in the set, 0 otherwise.
      AGT_nodiscard size_type count(const_arg_type_t<ValueType> V) const noexcept {
        return mapInst.count(V);
      }

      bool erase(const ValueType& V) noexcept {
        return mapInst.erase(V);
      }

      void swap(basic_set& RHS) noexcept { mapInst.swap(RHS.mapInst); }

      // Iterators.

      class ConstIterator;

      class Iterator {
        typename map_type::iterator I;
        friend class basic_set;
        friend class ConstIterator;

      public:
        using difference_type = typename map_type::iterator::difference_type;
        using value_type = ValueType;
        using pointer = value_type *;
        using reference = value_type &;
        using iterator_category = std::forward_iterator_tag;

        constexpr Iterator() = default;
        constexpr Iterator(const typename map_type::iterator &i) noexcept : I(i) {}

        ValueType& operator*() noexcept { return I->getFirst(); }
        const ValueType& operator*() const noexcept { return I->getFirst(); }
        ValueType* operator->() noexcept { return &I->getFirst(); }
        const ValueType* operator->() const noexcept { return &I->getFirst(); }

        Iterator& operator++() noexcept { ++I; return *this; }
        Iterator operator++(int) noexcept { auto T = *this; ++I; return T; }

        bool operator==(const ConstIterator& X) const noexcept { return I == X.I; }
        bool operator!=(const ConstIterator& X) const noexcept { return I != X.I; }
      };

      class ConstIterator {
        typename map_type::const_iterator I;
        friend class basic_set;
        friend class Iterator;

      public:
        using difference_type = typename map_type::const_iterator::difference_type;
        using value_type = ValueType;
        using pointer = const value_type *;
        using reference = const value_type &;
        using iterator_category = std::forward_iterator_tag;

        constexpr ConstIterator() = default;
        constexpr ConstIterator(const Iterator &B) noexcept : I(B.I) {}
        constexpr ConstIterator(const typename map_type::const_iterator &i) noexcept : I(i) {}

        const ValueType &operator*() const noexcept { return I->getFirst(); }
        const ValueType *operator->() const noexcept { return &I->getFirst(); }

        ConstIterator& operator++() noexcept { ++I; return *this; }
        ConstIterator operator++(int) noexcept { auto T = *this; ++I; return T; }
        bool operator==(const ConstIterator& X) const noexcept { return I == X.I; }
        bool operator!=(const ConstIterator& X) const noexcept { return I != X.I; }
      };

      using iterator = Iterator;
      using const_iterator = ConstIterator;

      AGT_nodiscard iterator       begin() noexcept { return Iterator(mapInst.begin()); }
      AGT_nodiscard const_iterator begin() const noexcept { return ConstIterator(mapInst.begin()); }

      AGT_nodiscard iterator       end() noexcept { return Iterator(mapInst.end()); }
      AGT_nodiscard const_iterator end() const noexcept { return ConstIterator(mapInst.end()); }

      AGT_nodiscard iterator find(const_arg_type_t<ValueType> V) noexcept { return Iterator(mapInst.find(V)); }
      AGT_nodiscard const_iterator find(const_arg_type_t<ValueType> V) const noexcept {
        return ConstIterator(mapInst.find(V));
      }

      /// Alternative version of find() which allows a different, and possibly less
      /// expensive, key type.
      /// The DenseMapInfo is responsible for supplying methods
      /// getHashValue(LookupKeyT) and isEqual(LookupKeyT, KeyT) for each key type
      /// used.
      template <class LookupKeyT>
      AGT_nodiscard iterator find_as(const LookupKeyT &Val) noexcept {
        return Iterator(mapInst.find_as(Val));
      }
      template <class LookupKeyT>
      AGT_nodiscard const_iterator find_as(const LookupKeyT &Val) const noexcept {
        return ConstIterator(mapInst.find_as(Val));
      }

      void erase(Iterator I) noexcept { return mapInst.erase(I.I); }
      void erase(ConstIterator CI) noexcept { return mapInst.erase(CI.I); }

      std::pair<iterator, bool> insert(const ValueType& V) noexcept {
        no_value emptyVal;
        return mapInst.try_emplace(V, emptyVal);
      }

      std::pair<iterator, bool> insert(ValueType &&V) noexcept {
        no_value emptyVal;
        return mapInst.try_emplace(std::move(V), emptyVal);
      }

      /// Alternative version of insert that uses a different (and possibly less
      /// expensive) key type.
      template <typename LookupKeyT>
      std::pair<iterator, bool> insert_as(const ValueType &V, const LookupKeyT &LookupKey) noexcept {
        return mapInst.insert_as({V, no_value{}}, LookupKey);
      }
      template <typename LookupKeyT>
      std::pair<iterator, bool> insert_as(ValueType &&V, const LookupKeyT &LookupKey) noexcept {
        return mapInst.insert_as({std::move(V), no_value{}}, LookupKey);
      }

      // Range insertion of values.
      template<typename InputIt>
      void insert(InputIt I, InputIt E) noexcept {
        for (; I != E; ++I)
          insert(*I);
      }
    };
  }

  template <typename T, typename KeyInfo = map_key_info<T>>
  class set : public impl::basic_set<T, map<T, impl::no_value, KeyInfo>, KeyInfo> {
    using map_type  = map<T, impl::no_value, KeyInfo>;
    using base_type = impl::basic_set<T, map_type, KeyInfo>;
  public:
    using base_type::base_type;
  };

  template <typename T>
  class shared_set {

  };

}

#endif//AGATE_SUPPORT_SET_HPP
