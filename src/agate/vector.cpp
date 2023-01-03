//
// Created by maxwe on 2021-09-24.
//

#include "vector.hpp"

using namespace agt;

#include <string>
#include <iostream>


namespace {
  struct struct_16bytes {
    alignas(16) void * x;
  };
  struct struct_32bytes {
    alignas(32) void * x;
  };
}

static_assert(sizeof(vector<void *, 0>) == sizeof(unsigned) * 2 + sizeof(void *),
              "wasted space in small_array size 0");
static_assert(alignof(vector<struct_16bytes, 0>) >= alignof(struct_16bytes),
              "wrong alignment for 16-byte aligned T");
static_assert(alignof(vector<struct_32bytes, 0>) >= alignof(struct_32bytes),
              "wrong alignment for 32-byte aligned T");
static_assert(sizeof(vector<struct_16bytes, 0>) >= alignof(struct_16bytes),
              "missing padding for 16-byte aligned T");
static_assert(sizeof(vector<struct_32bytes, 0>) >= alignof(struct_32bytes),
              "missing padding for 32-byte aligned T");
static_assert(sizeof(vector<void *, 1>) == sizeof(unsigned) * 2 + sizeof(void *) * 2,
              "wasted space in small_array size 1");

static_assert(sizeof(vector<char, 0>) == sizeof(void *) * 2 + sizeof(void *),
              "1 byte elements have word-sized type for size and capacity");

/// Report that minimum_size doesn't fit into this vector's size type. Throws
/// std::length_error or calls report_fatal_error.
AGT_noreturn static void report_size_overflow(agt_u64_t minimum_size, agt_u64_t maximum_size)  {
  std::string Reason = "valkyrie::small_vector unable to grow. Requested capacity (" +
                       std::to_string(minimum_size) +
                       ") is larger than maximum value for size type (" +
                       std::to_string(maximum_size) + ")";
#ifdef AGT_exceptions_enabled
  throw std::length_error(Reason);
#else
  /*std::u8string reason = "valkyrie::small_vector unable to grow. Requested capacity (" +

                         panic();*/
  std::cerr << Reason;
  abort();
#endif
}

/// Report that this vector is already at maximum capacity. Throws
/// std::length_error or calls report_fatal_error.
AGT_noreturn static void report_at_maximum_capacity(agt_u64_t maximum_size)  {
  std::string Reason =
    "valkyrie::small_vector capacity unable to grow. Already at maximum size " +
    std::to_string(maximum_size);
#ifdef AGT_exceptions_enabled
  throw std::length_error(Reason);
#else
  //report_fatal_error(Reason);
  std::cerr << Reason;
  abort();
#endif
}


template <class Size_T>
void* impl::vector_base<Size_T>::malloc_for_grow(agt_u64_t minimum_size, agt_u64_t type_size, agt_u64_t& new_capacity, agt_u64_t alignment) {
  new_capacity = get_new_capacity(minimum_size, this->capacity());
  return _aligned_malloc(new_capacity * type_size, alignment);
}
template <class Size_T>
void  impl::vector_base<Size_T>::grow_pod(void *first_element, agt_u64_t minimum_capacity, agt_u64_t type_size, agt_u64_t alignment) {
  agt_u64_t new_capacity = get_new_capacity(minimum_capacity, this->capacity());
  void* new_elements;
  if (begin_ == first_element) {
    new_elements = _aligned_malloc(new_capacity * type_size, alignment);
    std::memcpy(new_elements, this->begin_, size() * type_size);
  }
  else {
    new_elements = _aligned_realloc(begin_,
                                    new_capacity * type_size,
                                    /*capacity_ * type_size,*/
                                    alignment);
  }

  begin_ = new_elements;
  capacity_ = new_capacity;
}


template <class Size_T>
agt_u64_t impl::vector_base<Size_T>::get_new_capacity(agt_u64_t minimum_size, agt_u64_t old_capacity) noexcept {
  constexpr agt_u64_t maximum_size = std::numeric_limits<Size_T>::max();

  // Ensure we can fit the new capacity.
  // This is only going to be applicable when the capacity is 32 bit.
  if (minimum_size > maximum_size) [[unlikely]]
    report_size_overflow(minimum_size, maximum_size);

  // Ensure we can meet the guarantee of space for at least one more element.
  // The above check alone will not catch the case where grow is called with a
  // default minimum_size of 0, but the current capacity cannot be increased.
  // This is only going to be applicable when the capacity is 32 bit.
  if (old_capacity == maximum_size) [[unlikely]]
    report_at_maximum_capacity(maximum_size);

  // In theory 2*capacity can overflow if the capacity is 64 bit, but the
  // original capacity would never be large enough for this to be a problem.
  const agt_u64_t new_capacity = 2 * old_capacity + 1; // Always grow.
  return std::min(std::max(new_capacity, minimum_size), maximum_size);
}

template class impl::vector_base<agt_u32_t>;
template class impl::vector_base<agt_u64_t>;