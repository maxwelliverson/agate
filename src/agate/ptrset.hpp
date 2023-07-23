//
// Created by maxwe on 2023-03-01.
//

#ifndef AGATE_INTERNAL_PTRSET_HPP
#define AGATE_INTERNAL_PTRSET_HPP

#include "config.hpp"

#include <bit>
#include <optional>

namespace agt {
  namespace impl {

    template <bool IsConst = false>
    class ptrset_iter {

      using value_type = std::conditional_t<IsConst, const uintptr_t, uintptr_t>;

      value_type* front;
      value_type* pos;
      value_type* end;
      uintptr_t   minValue;
    public:

      template <bool OtherIsConst>
      explicit(!IsConst && OtherIsConst) ptrset_iter(const ptrset_iter<OtherIsConst>& other) noexcept
          : front(const_cast<value_type*>(other.front)),
            pos(const_cast<value_type*>(other.pos)),
            end(const_cast<value_type*>(other.end)),
            minValue(other.minValue)
      {}

      ptrset_iter(value_type* front, uint32_t size, uint32_t pos, bool advanceToValid = true) noexcept
          : front(front),
            pos(front + pos),
            end(front + size),
            minValue(size)
      {
        if (advanceToValid) {
          if (!isDone() && value() <= minValue)
            next();
        }
      }



      bool next() noexcept {
        assert( !isDone() );
        do {
          ++pos;
          if (isDone())
            return false;
          if (value() > minValue)
            return true;
        } while(true);
      }

      [[nodiscard]] uint32_t position() const noexcept {
        return static_cast<uint32_t>(pos - front);
      }

      value_type& value() const noexcept {
        return *pos;
      }

      [[nodiscard]] bool isDone() const noexcept {
        return pos == end;
      }
    };

    class ptrset {

      void initialize_freelist(uint32_t from = 0) noexcept {
        auto table = m_table;
        while ( from < m_capacity ) {
          uint32_t next = from + 1;
          table[from] = next;
          from = next;
        }
      }

      [[nodiscard]] uintptr_t* ptr() noexcept {
        return m_isLarge ? m_table : &m_value;
      }
      [[nodiscard]] const uintptr_t* ptr() const noexcept {
        return m_isLarge ? m_table : &m_value;
      }

    public:

      // Note: while this data structure is intended for use as a set,
      // (ie. unordered, unique membership), uniqueness is not enforced,
      // but is rather assumed. Care must be taken in use to ensure that
      // there are no duplicate entries.

      void init(uint32_t initialCapacity = 1) noexcept {
        assert( initialCapacity > 0 );
        auto trueCapacity = std::bit_ceil(initialCapacity);
        m_nextFreeSlot = 0;
        m_capacity       = trueCapacity;
        m_availableCount = trueCapacity;
        if (initialCapacity > 1) {
          m_isLarge = true;
          m_table = (uintptr_t*)malloc(trueCapacity * sizeof(uintptr_t));
          initialize_freelist();
        }
        else {
          m_isLarge  = false;
        }
      }

      // returns position, which may be cached and later used for O(1) removal.
      uint32_t insert(uintptr_t value) noexcept {
        uint32_t pos;
        if (m_availableCount) {
          if (m_isLarge) {
            pos = m_nextFreeSlot;
            uint32_t next = m_table[pos];
            m_table[pos] = value;
            m_nextFreeSlot = next;
          }
          else {
            pos = 0;
            m_value = value;
          }
          --m_availableCount;
        }
        else {
          uint32_t newCapacity;
          if (m_isLarge) {
            newCapacity = m_capacity * 2;
            m_table = (uintptr_t*)realloc(m_table, newCapacity * sizeof(uintptr_t));
            pos = m_capacity;
            m_availableCount = m_capacity - 1;
          }
          else {
            m_isLarge = true;
            auto oldValue = m_value;
            newCapacity = 4; // Generally, if an agent has opened more than one handle, it's likely they'll open at least a couple more.
            auto table = (uintptr_t*)malloc(newCapacity * sizeof(uintptr_t));
            table[0] = oldValue;
            m_table = table;
            m_availableCount = newCapacity - 2;
            pos = 1;
          }
          m_capacity = newCapacity;
          uint32_t from = pos + 1;
          m_nextFreeSlot = from;
          m_table[pos] = value;
          initialize_freelist(from);
        }
        return pos;
      }

      // returns whether or not the value was actually found (and subsequently removed)
      bool remove(uintptr_t value) noexcept {
        if (auto pos = find(value)) {
          removeAt(pos.value());
          return true;
        }
        return false;
      }

      // returns the position if the value was found, or nothing if not.
      [[nodiscard]] std::optional<uint32_t> find(uintptr_t value) const noexcept {
        if (m_availableCount < m_capacity) {
          if (m_isLarge) [[likely]] {
            for (auto iter = makeIter(0); !iter.isDone(); iter.next()) {
              if (iter.value() == value)
                return iter.position();
            }
          }
          else if (m_value == value)
            return 0;
        }

        return std::nullopt;
      }

      uintptr_t at(uint32_t position) const noexcept {
        assert( position < m_capacity );
        return ptr()[position];
      }

      void removeAt(uint32_t position) noexcept {
        assert( position < m_capacity );
        assert( m_capacity < at(position) );
        ptr()[position] = m_nextFreeSlot;
        m_nextFreeSlot = position;
        ++m_availableCount;
        if (m_isLarge && m_availableCount == m_capacity && m_capacity > 4) {
          free(m_table);
          m_value = 0;
          m_capacity = 1;
          m_isLarge = false;
        }
      }

      void destroy() noexcept {
        if (m_capacity > 1)
          delete[] m_table;
      }

      ptrset_iter<false> makeIter(uint32_t position, bool advanceToValid = true) noexcept {
        assert( position <= m_capacity );
        return ptrset_iter<false>(ptr(), m_capacity, position, advanceToValid);
      }

      ptrset_iter<true>  makeIter(uint32_t position, bool advanceToValid = true) const noexcept {
        assert( position <= m_capacity );
        return ptrset_iter<true>(ptr(), m_capacity, position, advanceToValid);
      }

    private:
      uint32_t m_availableCount;
      uint32_t m_capacity;
      uint32_t m_nextFreeSlot;
      bool     m_isLarge;
      union {
        uintptr_t  m_value;
        uintptr_t* m_table;
      };
    };
  }

  template <typename T>
  class ptrset : impl::ptrset {
  public:

  };
}

#endif//AGATE_INTERNAL_PTRSET_HPP
