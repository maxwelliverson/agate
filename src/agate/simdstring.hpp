//
// Created by maxwe on 2023-01-12.
//

#ifndef AGATE_SIMDSTRING_HPP
#define AGATE_SIMDSTRING_HPP

#include "config.hpp"

#include "agate/align.hpp"

#include <memory>

#include <cstring>
#include <cstdlib>

#include <immintrin.h>

namespace agt {

  using simdchar = AGT_alignas(16) agt_char_t;

  class AGT_alignas(16) simdstring_chunk {
    inline constexpr static size_t TotalSize = 16;
    inline constexpr static size_t CharSize  = sizeof(agt_char_t);
    inline constexpr static size_t CharsPerChunk = TotalSize / CharSize;
    inline constexpr static size_t Alignment = 16;
  public:




  private:
    __m128i m_xmmReg;
    int     m_chunkLength;
  };

  class simdstring {
  public:

  private:

  };

  class simdstring_view {
  public:
    simdstring_view() = default;
    simdstring_view(const simdchar* ptr, size_t length) noexcept
        : m_ptr(ptr), m_length(length) { }



  private:
    const simdchar* m_ptr;
    size_t          m_length;
  };

  template <size_t N = 32>
  class simdstring_buffer {
    inline constexpr static size_t CharSize = sizeof(agt_char_t);
    inline constexpr static size_t Alignment = 16;

    inline static size_t _alloc_size(size_t length) noexcept {
      return align_to<Alignment>((length + 1) * CharSize);
    }

    static_assert(N % Alignment == 0, "For any instance of simdstring_buffer<N>, N must be a multiple of 16.");

  public:

    simdstring_buffer() = default;
    simdstring_buffer(const agt_char_t* str, size_t length) noexcept {
      assign(str, length);
    }
    ~simdstring_buffer() {
      _aligned_free(m_dynPtr);
    }

    void assign(const agt_char_t* str, size_t length) noexcept {
      if (length < N) {
        reset();
        std::memcpy(m_staticBuffer, str, length * CharSize);
        m_staticBuffer[length] = {};
        m_ptr = m_staticBuffer;
        m_length = length;
      }
      else {
        const size_t allocSize = _alloc_size(length);

        if (m_dynPtr) {
          if (allocSize > m_dynLength) {
            auto newPtr = (simdchar*) _aligned_realloc(m_dynPtr, allocSize, Alignment);
            m_dynPtr = newPtr;
            m_ptr = newPtr;
            m_dynLength = allocSize;
          }

          AGT_invariant( m_dynPtr == m_ptr );
          std::memcpy(m_dynPtr, str, length * CharSize);
          m_dynPtr[length] = {};
          m_length = length;
        }
        else {
          auto mem = (simdchar*)_aligned_malloc(allocSize, Alignment);
          std::memcpy(mem, str, length * CharSize);
          mem[length] = {};
          m_ptr = mem;
          m_length = length;
          m_dynPtr = mem;
          m_dynLength = allocSize;
        }
      }
    }

    [[nodiscard]] simdstring_view view() const noexcept {
      return { m_ptr, m_length };
    }

    void reset() noexcept {
      if (m_ptr) {
        m_ptr = nullptr;
        m_length = 0;
        _aligned_free(m_dynPtr);
        m_dynLength = 0;
      }
    }

  private:

    simdchar* m_ptr = nullptr;
    size_t    m_length = 0;
    AGT_alignas(16) agt_char_t m_staticBuffer[N] = {};
    simdchar* m_dynPtr = nullptr;
    size_t    m_dynLength = 0;
  };

}

#endif//AGATE_SIMDSTRING_HPP
