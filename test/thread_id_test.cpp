//
// Created by maxwe on 2022-12-27.
//

#include <thread>
#include <atomic>
#include <span>
#include <random>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <ranges>
#include <algorithm>
#include <numeric>

#include <immintrin.h>

struct thread_info {
  size_t          index;
  size_t          infoIndex;
  std::thread::id stdId;
  uintptr_t       gsId;
};

inline constexpr static uint32_t SleepTimeSeed = 0;
inline constexpr static uint32_t MaxSleepTime = 2000;
inline constexpr static uint32_t MinSleepTime = 10;

void init_sleep_times(std::span<uint32_t> sleepTimes, uint32_t seed, uint32_t minSleep, uint32_t maxSleep) noexcept {
  if (seed == 0)
    seed = std::random_device()();

  std::mt19937 rand{seed};
  std::uniform_int_distribution<uint32_t> dist{minSleep, maxSleep};

  for (auto& time : sleepTimes)
    time = dist(rand);
}

void init_threads(size_t offset, size_t count, std::span<std::thread> threads, std::span<thread_info> infoArray, std::span<const uint32_t> sleepTimes) {
  for (size_t i = 0; i < count; ++i) {
    size_t realIndex = i + offset;
    threads[realIndex] = std::thread([](size_t index, thread_info* infoArray, const uint32_t* sleepTimes){

      static constinit std::atomic_size_t infoArrayIndex = 0;

      auto sleepTime = sleepTimes[index];
      std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
      size_t infoIndex = infoArrayIndex++;
      auto& info = infoArray[infoIndex];
      info.index = index;
      info.infoIndex = infoIndex;
      info.stdId = std::this_thread::get_id();
      info.gsId  = _readgsbase_u64();
    }, realIndex, infoArray.data(), sleepTimes.data());
  }
}

void join_threads(size_t offset, size_t count, std::span<std::thread> threads) {
  for (size_t i = 0; i < count; ++i)
    threads[offset + i].join();
}

void run_threads(std::span<const size_t> batchSizes, std::span<std::thread> threads, std::span<thread_info> infoArray, std::span<const uint32_t> sleepTimes) {
  size_t offset = 0;
  for (auto batchSize : batchSizes) {
    init_threads(offset, batchSize, threads, infoArray, sleepTimes);
    join_threads(offset, batchSize, threads);
    offset += batchSize;
  }
}

size_t total_thread_count(std::span<const size_t> batchSizes) {
  size_t count = 0;
  for (auto partialCount : batchSizes)
    count += partialCount;
  return count;
}


std::map<uintptr_t, std::set<size_t>> map_indices_to_gsid(std::span<const thread_info> infoArray) noexcept {
  std::map<uintptr_t, std::set<size_t>> m;

  for (auto&& info : infoArray)
    m[info.gsId].insert(info.index);

  // return std::move(m);

  return m;
}


int main(int argc, char** argv) {

  const uint32_t seed = 0;
  const uint32_t minSleepPeriod = 10;
  const uint32_t maxSleepPeriod = 200;

  std::vector<size_t> batchSizes;

  if (argc == 1) {
    batchSizes = { 1, 1, 1, 5, 1, 10, 20, 10, 5, 100, 20 };
  }
  else {
    for (int i = 1; i < argc; ++i)
      batchSizes.push_back(std::atoll(argv[i]));
  }


  const size_t threadCount = total_thread_count(batchSizes);

  std::vector<std::thread> threads;
  std::vector<thread_info> infos;
  std::vector<uint32_t>    sleepTimes;

  threads.resize(threadCount);
  infos.resize(threadCount);
  sleepTimes.resize(threadCount);


  init_sleep_times(sleepTimes, seed, minSleepPeriod, maxSleepPeriod);

  run_threads(batchSizes, threads, infos, sleepTimes);


  for (size_t i = 0; i < threadCount; ++i) {
    printf("%llu :: %ums\n", i, sleepTimes[i]);
  }

  printf("\n\n\n");


  for (auto&& entry : map_indices_to_gsid(infos)) {
    std::stringstream sstr;

    bool needsComma = false;
    // sstr << std::hex;
    for (auto index : entry.second) {
      if (needsComma)
        sstr << ",";
      sstr << " " << index;
      needsComma = true;
    }
    auto str = std::move(sstr).str();
    printf("%llx :: {%s }\n", entry.first, str.c_str());
  }




}