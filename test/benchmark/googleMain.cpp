//
// Created by Maxwell on 2024-02-19.
//

#include "benchmark_utils.hpp"


#include "agate/fiber.h"
#include "agate/init.h"

#include "agate/integer_division.hpp"


#include <benchmark/benchmark.h>
#include <chrono>
#include <thread>
#include <semaphore>
#include <mutex>
#include <condition_variable>
#include <random>


#define NOMINMAX
#include <Windows.h>


#pragma optimize("", off)
[[msvc::noinline]] void math_op(double value) { (void)std::riemann_zeta(value); }
#pragma optimize("", on)

extern "C" void   bm_nop_op(size_t count);

extern "C" void   bm_nopx4_op(size_t count);


static void BM_nop_op(benchmark::State& state) {
  for (auto _ : state) {
    bm_nop_op(state.range());
    benchmark::ClobberMemory();
  }
  state.SetComplexityN(state.range());
}


// BENCHMARK(BM_nop_op)->RangeMultiplier(2)->Range(0x1<<7, 0x1<<14)->Complexity(benchmark::oN)->Repetitions(20);

#define BENCHMARK_LOOP(state_) for (auto _ : *static_cast<benchmark::State*>(state_))


namespace {
  uint64_t get_tsc_hz() noexcept {
    DWORD data;
    DWORD dataSize = sizeof(DWORD);
    if (RegGetValue(HKEY_LOCAL_MACHINE,
                    "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                    "~MHz", RRF_RT_DWORD, nullptr, &data, &dataSize) == ERROR_SUCCESS) {
      return static_cast<uint64_t>(data) * 1000000;
    }
    return 0;
  }

  AGT_forceinline uint64_t get_100ns_timestamp() noexcept {
    LARGE_INTEGER lrgInt;
    QueryPerformanceCounter(&lrgInt);
    return lrgInt.QuadPart;
  }

  template <typename T>
  [[msvc::noinline, clang::noinline]] void do_not_optimize(T&&) noexcept { }

  std::pair<uint32_t, uint32_t> get_tsc_to_ns_ratio() noexcept {
    if (auto tscHz = get_tsc_hz()) {
      auto billion = 1000000000ull;
      const auto gcd = agt::impl::calculate_gcd(tscHz, billion);
      if (gcd != 1) {
        tscHz /= gcd;
        billion /= gcd;
      }
      if (tscHz > UINT_MAX) {
        std::terminate(); // :(
      }

      return { static_cast<uint32_t>(billion), static_cast<uint32_t>(tscHz) };
    }


    constexpr static uint64_t EstimateTimeout_ms = 500;


    const auto start100ns = get_100ns_timestamp();
    const auto start      = __rdtsc();


    const uint64_t   target100ns = start100ns + (EstimateTimeout_ms * 10000);


    std::minstd_rand rng{};
    uint64_t state = 0;
    uint64_t curr100ns = get_100ns_timestamp();

    while (curr100ns < target100ns) {
      constexpr static auto BatchSize = 10000;
      rng.discard(BatchSize);
      state += rng();
      curr100ns = get_100ns_timestamp();
    }

    do_not_optimize(state);

    const auto end100ns = get_100ns_timestamp();
    const auto end = __rdtsc();

    return { static_cast<uint32_t>((end100ns - start100ns) * 100), static_cast<uint32_t>(end - start) };
  }
}


const static agt::ratio_multiplier g_tscToNsRatio = []{
  agt::ratio_multiplier ratio;
  auto [num, den] = get_tsc_to_ns_ratio();
  agt::precompile_ratio(ratio, num, den);
  return ratio;
}();



__forceinline agt_fiber_param_t now() noexcept {
  return __rdtsc();
  // const auto time = std::chrono::high_resolution_clock::now();
  // return reinterpret_cast<const agt_fiber_param_t&>(time);
}

void setManualTime(void* state_, agt_fiber_param_t start, agt_fiber_param_t end) noexcept {
  auto& state = *static_cast<benchmark::State*>(state_);
  const auto count = end - start;
  const auto nsCount = agt::multiply(count, g_tscToNsRatio);
  state.SetIterationTime(static_cast<double>(nsCount) / 1000000000.0);
  /*const static long long PerfFrequency = []{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return freq.QuadPart;
  }();



  using std::chrono::duration, std::chrono::duration_cast;

  duration<double> iterationDuration;

  if (PerfFrequency == 10000000) {
    iterationDuration = duration_cast<duration<double>>(duration<uint64_t, std::nano>(count));
  }
  else {
    iterationDuration =
  }*/





  // const auto start = std::bit_cast<std::chrono::high_resolution_clock::time_point>(start_);
  // const auto end = std::bit_cast<std::chrono::high_resolution_clock::time_point>(end_);

  // state.SetIterationTime(std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count());
}

__forceinline agt_fiber_flags_t getFiberFlags(void* state, size_t pos = 0) noexcept {
  return static_cast<agt_fiber_flags_t>(static_cast<benchmark::State*>(state)->range(pos));
}


std::optional<agt_fiber_t> newFiber(void* state, agt_fiber_proc_t proc, void* userData = nullptr) noexcept {
  agt_fiber_t fiber;
  agt_status_t result = agt_new_fiber(&fiber, proc, userData);
  if (result != AGT_SUCCESS) {
    static_cast<benchmark::State*>(state)->SkipWithError("agt_new_fiber failed with error" + std::to_string(result));
    return std::nullopt;
  }
  return fiber;
}


static agt_fiber_param_t fiber_switch_to_loop_proc(agt_fiber_t, agt_fiber_param_t, void* state) {

  agt_fiber_t fiber;

  if (auto maybeFiber = newFiber(state, [](agt_fiber_t fromFiber, agt_fiber_param_t, void*) -> agt_fiber_param_t {
    agt_fiber_jump(fromFiber, now());
  }))
    fiber = *maybeFiber;
  else
    return 1;


  BENCHMARK_LOOP(state) {
    const auto flags = getFiberFlags(state);
    const auto start = now();
    const auto end = agt_fiber_switch(fiber, 0, flags).param;
    setManualTime(state, start, end);
  }

  agt_destroy_fiber(fiber);

  return 0;
}

static agt_fiber_param_t fiber_jump_to_loop_proc(agt_fiber_t self, agt_fiber_param_t, void* state) {
  agt_fiber_t fiber;

  if (auto maybeFiber = newFiber(state, [](agt_fiber_t fromFiber, agt_fiber_param_t, void*) -> agt_fiber_param_t {
    agt_fiber_jump(fromFiber, now());
  }))
    fiber = *maybeFiber;
  else
    return 1;

  struct data_t {
    void*       state;
    agt_fiber_t self;
    agt_fiber_t fiber;
  } data{ state, self, fiber };

  agt_set_fiber_data(AGT_CURRENT_FIBER, &data);

  BENCHMARK_LOOP(state) {
    agt_fiber_loop([](agt_fiber_t fromFiber, agt_fiber_param_t start, void* userData) -> agt_fiber_param_t {
      const auto end = now();
      auto& data = *static_cast<data_t*>(userData);
      if (fromFiber == data.self)
        agt_fiber_jump(data.fiber, 0);
      setManualTime(data.state, start, end);
      return 0;
    }, 0, getFiberFlags(state));
  }

  agt_destroy_fiber(fiber);

  return 0;
}

static agt_fiber_param_t fiber_enter_loop_proc(agt_fiber_t, agt_fiber_param_t, void* state) {
  BENCHMARK_LOOP(state) {
    const auto flags = getFiberFlags(state);
    const auto start = now();
    const auto end = agt_fiber_loop([](agt_fiber_t, agt_fiber_param_t, void*) -> agt_fiber_param_t {
      return now();
    }, 0, flags);
    setManualTime(state, start, end);
  }
  return 0;
}

static agt_fiber_param_t fiber_exit_loop_proc(agt_fiber_t, agt_fiber_param_t, void* state) {
  BENCHMARK_LOOP(state) {
    const auto flags = getFiberFlags(state);
    const auto start = agt_fiber_loop([](agt_fiber_t, agt_fiber_param_t, void*) -> agt_fiber_param_t {
      return now();
    }, 0, flags);
    const auto end = now();
    setManualTime(state, start, end);
  }
  return 0;
}


static agt_fiber_param_t fiber_enter_fork_proc(agt_fiber_t, agt_fiber_param_t, void* state) {
  BENCHMARK_LOOP(state) {
    const auto flags = getFiberFlags(state);
    const auto start = now();
    const auto end = agt_fiber_fork([](agt_fiber_t, agt_fiber_param_t, void*) -> agt_fiber_param_t {
      return now();
    }, 0, flags).param;
    setManualTime(state, start, end);
  }
  return 0;
}

static agt_fiber_param_t fiber_leave_fork_proc(agt_fiber_t, agt_fiber_param_t, void* state) {
  BENCHMARK_LOOP(state) {
    const auto flags = getFiberFlags(state);
    const auto start = agt_fiber_fork([](agt_fiber_t, agt_fiber_param_t, void*) -> agt_fiber_param_t {
      return now();
    }, 0, flags).param;
    const auto end = now();
    setManualTime(state, start, end);
  }
  return 0;
}

static agt_fiber_param_t fiber_jump_to_fork_proc(agt_fiber_t, agt_fiber_param_t, void* state) {
  agt_fiber_t fiber;

  if (auto maybeFiber = newFiber(state, [](agt_fiber_t fromFiber, agt_fiber_param_t, void*) -> agt_fiber_param_t {
    agt_fiber_jump(fromFiber, now());
  }))
    fiber = *maybeFiber;
  else
    return 1;

  agt_set_fiber_data(AGT_CURRENT_FIBER, fiber);

  BENCHMARK_LOOP(state) {
    const auto start = agt_fiber_fork([](agt_fiber_t, agt_fiber_param_t, void* userData) -> agt_fiber_param_t {
      agt_fiber_jump(static_cast<agt_fiber_t>(userData), 0);
    }, 0, getFiberFlags(state)).param;
    const auto end = now();
    setManualTime(state, start, end);
  }

  agt_destroy_fiber(fiber);

  return 0;
}


static agt_fiber_param_t fiber_switch_to_fork_proc(agt_fiber_t, agt_fiber_param_t, void* state) {
  agt_fiber_t fiber;

  if (auto maybeFiber = newFiber(state, [](agt_fiber_t fromFiber, agt_fiber_param_t, void* state) -> agt_fiber_param_t {
    while (true) {
      const auto flags = getFiberFlags(state, 1);
      agt_fiber_switch(fromFiber, now(), flags);
    }
  }, state))
    fiber = *maybeFiber;
  else
    return 1;

  agt_set_fiber_data(AGT_CURRENT_FIBER, fiber);

  BENCHMARK_LOOP(state) {
    const auto start = agt_fiber_fork([](agt_fiber_t, agt_fiber_param_t, void* userData) -> agt_fiber_param_t {
      agt_fiber_jump(static_cast<agt_fiber_t>(userData), 0);
    }, 0, getFiberFlags(state)).param;
    const auto end = now();
    setManualTime(state, start, end);
  }

  agt_destroy_fiber(fiber);

  return 0;
}

static agt_fiber_param_t fiber_create_proc(agt_fiber_t, agt_fiber_param_t, void* state) {
  agt_fiber_t fiber;

  BENCHMARK_LOOP(state) {
    const auto start = now();
    agt_new_fiber(&fiber, [](agt_fiber_t, agt_fiber_param_t, void* state) -> agt_fiber_param_t { return 0; }, nullptr);
    const auto end = now();
    setManualTime(state, start, end);
    agt_destroy_fiber(fiber);
  }

  return 0;
}

static void BM_fibers(benchmark::State& state, agt_fiber_proc_t proc) {
  agt_ctx_t ctx;
  auto initResult = agt_default_init(&ctx);

  if (initResult != AGT_SUCCESS) {
    state.SkipWithError("agt_default_init failed with error code " + std::to_string(initResult));
    return;
  }

  agt_fctx_desc_t fctxDesc{
    .proc = proc,
    .userData = &state,
  };

  agt_status_t fctxResult = agt_enter_fctx(ctx, &fctxDesc, nullptr);

  if (fctxResult != AGT_SUCCESS)
    state.SkipWithError("agt_enter_fctx failed with error code " + std::to_string(fctxResult));

  agt_finalize(ctx);
}


static void BM_threads(benchmark::State& state) {
  std::mutex mtx;
  std::condition_variable secondThreadMayExecute;
  std::condition_variable mainLoopMayExecute;
  bool isDone = false;
  agt_fiber_param_t startTime = 0;

  std::unique_lock outerLock{mtx};

  std::thread otherThread{[&]() mutable {
    std::unique_lock lock{mtx};
    do {
      mainLoopMayExecute.notify_one();
      startTime = now();
      secondThreadMayExecute.wait(lock);
    } while(!isDone);
  }};

  BENCHMARK_LOOP(&state) {
    mainLoopMayExecute.wait(outerLock);
    const auto endTime = now();
    setManualTime(&state, startTime, endTime);
    secondThreadMayExecute.notify_one();
  }

  isDone = true;
  outerLock.unlock();
  otherThread.join();
}

static void BM_createThread(benchmark::State& state) {
  BENCHMARK_LOOP(&state) {
    const auto startTime = now();
    std::thread t{[]{  }};
    const auto endTime = now();
    setManualTime(&state, startTime, endTime);
    t.join();
  }
}


#define BENCHMARK_FIBER_FUNC(_func, _name) BENCHMARK_CAPTURE(BM_fibers, _func, _func##_proc)->Name(_name)->UseManualTime()
#define BENCHMARK_SAVE_FIBER_FUNC(_func, _name) BENCHMARK_FIBER_FUNC(_func, _name)->Arg(0)->Arg(AGT_FIBER_SAVE_EXTENDED_STATE)
#define BENCHMARK_MULTISAVE_FIBER_FUNC(_func, _name) BENCHMARK_FIBER_FUNC(_func, _name)->ArgsProduct({{ 0, AGT_FIBER_SAVE_EXTENDED_STATE }, { 0, AGT_FIBER_SAVE_EXTENDED_STATE }})


BENCHMARK_SAVE_FIBER_FUNC(fiber_switch_to_loop, "switch-to-loop");
BENCHMARK_SAVE_FIBER_FUNC(fiber_jump_to_loop,   "jump-to-loop");
BENCHMARK_SAVE_FIBER_FUNC(fiber_enter_loop,     "enter-loop");
BENCHMARK_SAVE_FIBER_FUNC(fiber_exit_loop,      "exit-loop");
BENCHMARK_SAVE_FIBER_FUNC(fiber_enter_fork,     "enter-fork");
BENCHMARK_SAVE_FIBER_FUNC(fiber_leave_fork,     "leave-fork");
BENCHMARK_SAVE_FIBER_FUNC(fiber_jump_to_fork,   "jump-to-fork");
BENCHMARK_MULTISAVE_FIBER_FUNC(fiber_switch_to_fork, "switch-to-fork");
BENCHMARK_FIBER_FUNC(fiber_create,              "new-fiber");

BENCHMARK(BM_threads)->Name("thread-switch")->UseManualTime();
BENCHMARK(BM_createThread)->Name("thread-create")->UseManualTime();




static void BM_nopx4_op(benchmark::State& state) {
  for (auto _ : state) {
    bm_nopx4_op(state.range());
    benchmark::ClobberMemory();
  }
  state.SetComplexityN(state.range());
}

// BENCHMARK(BM_nopx4_op)->RangeMultiplier(2)->Range(0x1<<7, 0x1<<14)->Complexity(benchmark::oN);


BENCHMARK_MAIN();