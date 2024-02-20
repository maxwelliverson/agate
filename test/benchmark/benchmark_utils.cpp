//
// Created by Maxwell on 2024-02-18.
//

#include "benchmark_utils.hpp"


#include <source_location>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// #include <realtimeapiset.h>


namespace {



  [[msvc::noinline]] void printWin32Error(DWORD err = GetLastError(), std::source_location loc = std::source_location::current()) noexcept {
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    fprintf_s(stderr, "Error at %s:%d:%d\n%s failed with error %#lX: %s", loc.file_name(), loc.line(), loc.column(), loc.function_name(), err, static_cast<const char*>(lpMsgBuf));
    LocalFree(lpMsgBuf);
    ExitProcess(err);
  }


  struct thread_profiler {
    HANDLE handle = nullptr;

    thread_profiler() {
      if (auto err = EnableThreadProfiling(GetCurrentThread(), THREAD_PROFILING_FLAG_DISPATCH, 0, &handle))
        printWin32Error(err);
    }

    ~thread_profiler() {
      if (auto err = DisableThreadProfiling(handle))
        printWin32Error(err);
    }

    void profile(PERFORMANCE_DATA& perfData) const noexcept {
      if (auto err = ReadThreadProfilingData(handle, READ_THREAD_PROFILING_FLAG_DISPATCHING, &perfData))
        printWin32Error(err);
    }
  };

  void profile(PERFORMANCE_DATA& perfData) noexcept {
    const thread_local thread_profiler profiler = {};
    perfData.Size = WORD(sizeof(PERFORMANCE_DATA));
    perfData.Version = PERFORMANCE_DATA_VERSION;
    profiler.profile(perfData);
  }
}


void bm::clobberMemory() noexcept {
  _ReadWriteBarrier();
}


void bm::thread::init() noexcept {

}



uint64_t bm::thread::cycles() noexcept {
  // ULONG64 cycles;
  FILETIME times[4];
  BOOL result = GetThreadTimes(GetCurrentThread(), times + 0, times + 1, times + 2, times + 3);

  // BOOL result = QueryThreadCycleTime(GetCurrentThread(), &cycles);
  if (!result) [[unlikely]]
    printWin32Error();

  return static_cast<uint64_t>(times[3].dwHighDateTime) << 32 | static_cast<uint64_t>(times[3].dwLowDateTime);
  // return cycles;
}

const bm::cycle_overhead& bm::thread::getCyclesOverhead() noexcept {
  thread_local cycle_overhead overhead = [] {
    size_t warmupIterCount = 1000;
    size_t iterCount = 10000000;
    // auto pValues = new uint64_t[iterCount];
    // std::span values{ pValues, iterCount };

    bool hasWarmedUp = false;
    do {
      bm::state state{};

      if (!hasWarmedUp)
        state.setIterationCount(warmupIterCount);
      else
        state.setIterationCount(iterCount);
      state.setDiscardIfSwitched(true);

      for (auto& iter : state) {
        char c;
        iter.start();
        doNotOptimize(c);
        iter.finish();
      }
      if (hasWarmedUp) {
        cycle_overhead overhead{};
        overhead.resize(state.m_readDelimitedIterations.size());
        for (size_t i = 0; i < state.m_readDelimitedIterations.size(); ++i) {
          auto& [sampleCount, dist] = overhead[i];
          dist = calculateCycleDistribution(state.m_readDelimitedIterations[i], &sampleCount, 8.0);
        }
        return overhead;
      }

      hasWarmedUp = true;
    } while(true);
  }();

  return overhead;
}



void bm::thread::executeOnNewThread(thread_func_t func, op_cycles& cycles, void* userData) noexcept {
  SECURITY_ATTRIBUTES secAttr{};
  secAttr.nLength = DWORD(sizeof(SECURITY_ATTRIBUTES));
  secAttr.bInheritHandle = FALSE;
  secAttr.lpSecurityDescriptor = nullptr;

  struct data_t {
    thread_func_t threadFunc;
    op_cycles*    pCycles;
    void*         funcUserData;
  };

  auto data = new data_t{};
  data->threadFunc = func;
  data->pCycles = &cycles;
  data->funcUserData = userData;

  HANDLE threadHandle = CreateThread(&secAttr, 0, [](PVOID userData) -> DWORD {
    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
      printWin32Error();
    const auto data = static_cast<data_t*>(userData);
    data->threadFunc(*data->pCycles, data->funcUserData);
    delete data;
    return 0;
  }, data, 0, nullptr);

  auto result = WaitForSingleObject(threadHandle, INFINITE);
  if (result == WAIT_FAILED)
    printWin32Error();
}



bm::op_cycles bm::state::getCycles() const noexcept  {
  auto overhead = thread::getCyclesOverhead()[0].second;

  auto cyclesDist = calculateCycleDistribution(m_readDelimitedIterations[0], nullptr, 7.0);

  op_cycles cycles{};

  cycles.mean         = std::max(cyclesDist.mean, overhead.mean) - overhead.mean;
  cycles.min          = std::max(cyclesDist.min, overhead.max) - overhead.max;
  cycles.tightMin     = std::max(cyclesDist.min, overhead.min) - overhead.min;
  cycles.tightMax     = std::max(cyclesDist.max, overhead.max) - overhead.max;
  cycles.max          = std::max(cyclesDist.max, overhead.min) - overhead.min;
  cycles.stdDev       = cyclesDist.stdDev + overhead.stdDev;
  cycles.maxMean      = std::max(cyclesDist.max, overhead.mean) - overhead.mean;
  cycles.minMean      = std::max(cyclesDist.min, overhead.mean) - overhead.mean;
  cycles.minMaxStdDev = overhead.stdDev;

  return cycles;

  /*auto overhead = thread::getCyclesOverhead();

  auto cyclesDist = calculateCycleDistribution(m_iterations);

  op_cycles cycles{};

  cycles.mean     = std::max(cyclesDist.mean, overhead.mean) - overhead.mean;
  cycles.min      = std::max(cyclesDist.min, overhead.max) - overhead.max;
  cycles.tightMin = std::max(cyclesDist.min, overhead.min) - overhead.min;
  cycles.tightMax = std::max(cyclesDist.max, overhead.max) - overhead.max;
  cycles.max      = std::max(cycles.max, overhead.min) - overhead.min;
  cycles.stdDev   = cyclesDist.stdDev + overhead.stdDev;
  cycles.maxMean  = std::max(cyclesDist.max, overhead.mean) - overhead.mean;
  cycles.minMean  = std::max(cyclesDist.min, overhead.mean) - overhead.mean;
  cycles.minMaxStdDev = overhead.stdDev;

  return cycles;*/
  // return {};
}


void bm::state::iteration::start() noexcept {
  PERFORMANCE_DATA perfData{};
  profile(perfData);
  m_startCycles = perfData.CycleTime;
  m_startSwitchCount = perfData.ContextSwitchCount;
  m_startReadCount = perfData.RetryCount;
}

void bm::state::iteration::finish() noexcept {
  PERFORMANCE_DATA perfData{};
  profile(perfData);
  const auto endCycles = perfData.CycleTime;
  const auto endSwitchCount = perfData.ContextSwitchCount;

  m_callback(endCycles - m_startCycles, endSwitchCount - m_startSwitchCount, m_startReadCount + perfData.RetryCount, m_userData);
}





void bm::usePointer(const volatile char *) { }



