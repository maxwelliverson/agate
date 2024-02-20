//
// Created by Maxwell on 2024-02-18.
//

#ifndef AGATE_TEST_BENCHMARK_UTILS_HPP
#define AGATE_TEST_BENCHMARK_UTILS_HPP



#include "agate/core.h"

#include <atomic>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <ranges>
#include <type_traits>
#include <vector>



namespace bm {

  [[msvc::noinline]] void usePointer(const volatile char*);

  template <typename T>
  void doNotOptimize(T&& val) noexcept {
    usePointer(reinterpret_cast<const volatile char*>(std::addressof(val)));
    // std::atomic_signal_fence(std::memory_order_acq_rel);
  }

  void clobberMemory() noexcept;

  struct op_cycles {
    uint64_t mean;
    uint64_t min;
    uint64_t max;
    uint64_t stdDev;
    uint64_t tightMin;
    uint64_t tightMax;
    uint64_t minMean;
    uint64_t maxMean;
    uint64_t minMaxStdDev;
  };

  struct cycle_distribution {
    uint64_t mean;
    uint64_t stdDev;
    uint64_t min;
    uint64_t max;
  };

  using cycle_overhead = std::vector<std::pair<uint64_t, cycle_distribution>>;


  template <typename Rng>
  [[nodiscard]] cycle_distribution calculateCycleDistributionTrimming(Rng&& rng, size_t* pRealElementCount, double stdDevTrim, double origMean, double origStdDev) noexcept {
    size_t elementCount = 0;
    uint64_t sum = 0;
    uint64_t min = UINT64_MAX;
    uint64_t max = 0;

    const auto isWithinRange = [&](double val) {
      return (std::abs(origMean - val) / origStdDev) < stdDevTrim;
    };

    for (auto val : rng) {
      if (isWithinRange(static_cast<double>(val))) {
        ++elementCount;
        sum += val;
        if (val > max)
          max = val;
        if (val < min)
          min = val;
      }
    }

    cycle_distribution dist{};

    const auto realCount = static_cast<double>(elementCount);

    double mean = static_cast<double>(sum) / realCount; // do integer division with rounding

    double varianceSum = 0;

    for (auto val : rng) {
      const auto floatVal = static_cast<double>(val);
      if (isWithinRange(floatVal)) {
        const auto diffFromMean = mean - floatVal;
        varianceSum += (diffFromMean * diffFromMean);
      }
    }

    double stdDev = std::sqrt(varianceSum / realCount);


    dist.mean = static_cast<uint64_t>(std::round(mean));
    dist.stdDev = static_cast<uint64_t>(std::round(stdDev));
    dist.min = min;
    dist.max = max;


    if (pRealElementCount)
      *pRealElementCount = elementCount;

    return dist;
  }


  template <typename Rng>
  [[nodiscard]] cycle_distribution calculateCycleDistribution(Rng&& rng, size_t* pRealElementCount = nullptr, double stdDevTrim = 0) noexcept {
    size_t elementCount = 0;
    uint64_t sum = 0;
    uint64_t min = UINT64_MAX;
    uint64_t max = 0;
    for (auto val : rng) {
      ++elementCount;
      sum += val;
      if (val > max)
        max = val;
      if (val < min)
        min = val;
    }

    cycle_distribution dist{};

    if (elementCount > 0) {
      const auto realCount = static_cast<double>(elementCount);

      double mean = static_cast<double>(sum) / realCount; // do integer division with rounding

      double varianceSum = 0;

      for (auto val : rng) {
        const auto diffFromMean = mean - static_cast<double>(val);
        varianceSum += (diffFromMean * diffFromMean);
      }

      double stdDev = std::sqrt(varianceSum / realCount);

      if (stdDevTrim != 0)
        return calculateCycleDistributionTrimming(rng, pRealElementCount, stdDevTrim, mean, stdDev);


      dist.mean = static_cast<uint64_t>(std::round(mean));
      dist.stdDev = static_cast<uint64_t>(std::round(stdDev));
      dist.min = min;
      dist.max = max;
    }

    if (pRealElementCount)
      *pRealElementCount = elementCount;

    return dist;
  }


  class state {

    friend class thread;

    bool tryStart() noexcept {
      if (!m_hasStarted) {
        // m_iterations.reserve(m_iterCount);
        m_hasStarted = true;
        return true;
      }
      return false;
    }
  public:

    class iterator;

    class iteration {

      friend class iterator;

      using callback_t = void (*)(uint64_t cycles, uint32_t switchCount, uint32_t readCount, void* userData);

      callback_t m_callback;
      void*      m_userData;
      uint64_t   m_startCycles      = 0;
      // uint64_t   m_endCycles        = 0;
      uint32_t   m_startSwitchCount = 0;
      uint32_t   m_startReadCount = 0;
      // uint32_t   m_endSwitchCount   = 0;
      bool       m_isAuto           = false;

      explicit iteration(callback_t callback, void* userData) noexcept
        : m_callback(callback),
          m_userData(userData)
      { }

      void reset() noexcept {
        m_startCycles      = 0;
        // m_endCycles        = 0;
        m_startSwitchCount = 0;
        m_startReadCount = 0;
          // m_endSwitchCount   = 0;
      }

    public:

      iteration(const iteration& other) noexcept
        : m_callback(other.m_callback),
          m_userData(other.m_userData)
      {
        m_isAuto = true;
        start();
      }

      ~iteration() {
        if (m_isAuto)
          finish();
      }

      void start() noexcept;

      void finish() noexcept;

    private:

      /*[[nodiscard]] uint64_t getCycleCount() const noexcept {
        return ((m_endCycles != 0) ? m_endCycles : m_resetCycles) - m_startCycles;
      }*/

      /*[[nodiscard]] uint64_t getCycleCount() const noexcept {
        return (m_endCycles - m_startCycles) - thread::getExcessCycles();
      }*/
    };

    class sentinel { };

    class iterator {
      // std::vector<uint64_t>::iterator m_pos;
      // std::vector<uint64_t>::iterator m_end;
      size_t            m_index = 0;
      size_t            m_maxIndex;
      mutable iteration m_iteration;

    public:

      using difference_type = std::ptrdiff_t;
      using value_type = iteration;
      using pointer = iteration*;
      using reference = iteration&;
      using iterator_category = std::input_iterator_tag;

      explicit iterator(state& state) noexcept
        : m_maxIndex(state.m_iterCount),
          m_iteration([](uint64_t cycleCount, uint32_t switchCount, uint32_t retryCount, void* userData) {
            auto s = static_cast<bm::state*>(userData);
            if (s->m_discardIfSwitched && switchCount > 0)
              return;
            if (s->m_readDelimitedIterations.size() < (retryCount + 1))
              s->m_readDelimitedIterations.resize(retryCount + 1);
            s->m_readDelimitedIterations[retryCount].push_back(cycleCount);
          }, &state)
      { }


      iteration& operator*() const noexcept {
        return m_iteration;
      }

      iteration* operator->() const noexcept {
        return &m_iteration;
      }

      iterator& operator++() noexcept {
        ++m_index;
        m_iteration.reset();
        return *this;
      }

      friend bool operator==(const iterator& self, const sentinel&) noexcept {
        return self.m_index == self.m_maxIndex;
      }
    };


    void setDiscardIfSwitched(bool enabled) noexcept {
      m_discardIfSwitched = enabled;
    }

    void setIterationCount(size_t maxIter) noexcept {
      m_iterCount = maxIter;
    }

    [[nodiscard]] op_cycles getCycles() const noexcept;


    [[nodiscard]] iterator begin() noexcept {
      if (!tryStart())
        m_iterCount = 0;
      return iterator(*this);
    }

    [[nodiscard]] sentinel end() const noexcept {
      return {};
    }

  private:
    bool                  m_hasStarted = false;
    bool                  m_discardIfSwitched = false;
    size_t                m_iterCount   = 1000;
    std::vector<std::vector<uint64_t>> m_readDelimitedIterations;
  };

  class thread {
    using thread_func_t = void(*)(op_cycles& cycles, void* userData);

    static void executeOnNewThread(thread_func_t func, op_cycles& cycles, void* userData) noexcept;
  public:

    static void     init() noexcept;

    static uint64_t cycles() noexcept;

    template <std::invocable<state&> Fn>
    static op_cycles measure(Fn&& fn) noexcept {
      op_cycles cycles{};
      using func_t = std::remove_cvref_t<Fn>;
      auto func = new func_t(std::forward<Fn>(fn));
      executeOnNewThread([](op_cycles& cycles, void* userData) {
        state benchmarkState{};
        const auto overhead = getCyclesOverhead();
        doNotOptimize(overhead);
        const auto func = static_cast<func_t*>(userData);
        std::invoke(*func, benchmarkState);
        cycles = benchmarkState.getCycles();
        delete func;
      }, cycles, func);

      return cycles;
    }

    /// @returns the overhead distribution for paired calls to thread::cycles
    // static const cycle_distribution& getCyclesOverhead() noexcept;

    static const cycle_overhead& getCyclesOverhead() noexcept;
  };







  /*template <std::invocable<state&> Fn>
  op_cycles measureCycles(Fn&& fn) noexcept {
    state benchmarkState{};
    std::invoke(std::forward<Fn>(fn), benchmarkState);
    return benchmarkState.getCycles();
  }*/
}

#endif //AGATE_TEST_BENCHMARK_UTILS_HPP
