//
// Created by Maxwell on 2024-02-07.
//

#define _CRT_RAND_S

#include <cstdio>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <random>
#include <thread>

#include <Windows.h>

__declspec(noinline) size_t currentStackSize() noexcept {
  volatile void* ptr;
  auto tib = reinterpret_cast<NT_TIB*>(NtCurrentTeb());
  return (static_cast<std::byte*>(tib->StackBase) - reinterpret_cast<std::byte*>(&ptr)) - (2*sizeof(void*));
}

static size_t initialStackSize = 0;
static size_t maxStackSize = 0;


class random_array {
  uint32_t* values = nullptr;
  uint32_t  size = 0;

  __declspec(noinline) static uint32_t* selectLeastElement(uint32_t least, uint32_t* leastPos, uint32_t* pos, uint32_t* end) noexcept {
    size_t stackSize = currentStackSize();
    if (maxStackSize < stackSize)
      maxStackSize = stackSize;
    if (pos == end)
      return leastPos;
    if (*pos < least)
      return selectLeastElement(*pos, pos, pos + 1, end);
    return selectLeastElement(least, leastPos, pos + 1, end);
  }

  __declspec(noinline) static void sortLeastElement(uint32_t* begin, uint32_t* end) noexcept {
    if (begin == end)
      return;
    const volatile auto next = begin + 1;
    auto leastElement = selectLeastElement(*begin, begin, next, end);
    if (leastElement != begin)
      std::swap(*leastElement, *begin);
    sortLeastElement(next, end);
  }

public:

  explicit random_array(uint32_t sz) noexcept {
    if (sz > 0) {
      values = new uint32_t[sz];
      size = sz;
      std::random_device randDev;
      std::mt19937 rand{randDev()};
      for (uint32_t i = 0; i < sz; ++i)
        values[i] = rand();
    }
  }

  random_array(const random_array&) = delete;

  ~random_array() {
    if (size > 0)
      delete[] values;
  }


  __declspec(noinline) void sort() noexcept {
    sortLeastElement(values, values + size);
  }

  [[nodiscard]] bool isSorted() const noexcept {
    if (size > 0) {
      uint32_t prevValue = *values;
      for (uint32_t i = 1; i < size; ++i) {
        const uint32_t val = values[i];
        if (val < prevValue)
          return false;
        prevValue = val;
      }
    }
    return true;
  }
};

__declspec(noinline) uint64_t factorial(uint64_t val) noexcept {
  return val ? 1 : val * factorial(val - 1);
}

template <typename T>
T read(void* addr, size_t offset) noexcept {
  T val;
  std::memcpy(&val, reinterpret_cast<std::byte*>(addr) + offset, sizeof(T));
  return val;
}


void printThreadInfo() {
  auto teb = NtCurrentTeb();
  auto tib = reinterpret_cast<NT_TIB*>(teb);


  auto deallocationStack    = read<PVOID>(tib, 0x1478);
  auto guaranteedStackBytes = read<ULONG>(tib, 0x1748);

  std::printf("Stack Base:         %p\n"
                    "Stack Limit:        %p\n"
                    "Deallocation Stack: %p\n"
                    "Guaranteed Bytes:   %lu\n\n",
                    tib->StackBase,
                    tib->StackLimit,
                    deallocationStack,
                    guaranteedStackBytes);
}


int main(int argc, char** argv) {

  uint32_t val = 0;

  if (argc == 2) {
    val = atoi(argv[1]);
    // printf("!%d = %llu", val, factorial(val));
  }

  std::thread worker{[](uint32_t val) {

    printThreadInfo();

    random_array array{val};

    initialStackSize = currentStackSize();
    maxStackSize = initialStackSize;

    array.sort();

    std::printf("array[%d] is sorted = %s\n\n", val, array.isSorted() ? "true" : "false");


    printThreadInfo();

  }, val};
  worker.join();

  std::printf("Initial Stack Size: %llu\n"
                    "Maximum Stack Size: %llu\n\n",
                    initialStackSize,
                    maxStackSize);

  return 0;
}