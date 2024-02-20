//
// Created by Maxwell on 2024-02-18.
//

#include "benchmark_utils.hpp"


#include "agate/init.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <iostream>

using json = nlohmann::json;

struct benchmark {
  std::string   testName;
  std::string   testArgs;
  std::vector<std::pair<std::string, bm::op_cycles>> cycles;
};

namespace bm {

  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(cycle_distribution, mean, stdDev, min, max);

  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(op_cycles, mean, stdDev, min, max, tightMin, tightMax, minMean, maxMean, minMaxStdDev);
}



std::ostream& operator<<(std::ostream& os, const bm::op_cycles& cycles) {
  const json j = cycles;
  return os << j.dump(2);
}

std::ostream& operator<<(std::ostream& os, const bm::cycle_distribution& cycles) {
  const json j = cycles;
  return os << j.dump(2);
}

std::ostream& operator<<(std::ostream& os, const benchmark& b)  {
  json j;

  j["name"] = b.testName;
  j["args"] = b.testArgs;
  j["cycles"] = b.cycles;

  os << j.dump(2);

  return os;
}


#pragma optimize("", off)
[[msvc::noinline]] void math_op(double value) { (void)std::riemann_zeta(value); }
#pragma optimize("", on)

extern "C" size_t bm_nop_op(size_t count);

extern "C" void   bm_nopx4_op(size_t count);



template <std::invocable<int> Fn>
benchmark doBenchmarkFiberSwitch(std::string_view name, std::initializer_list<int> counts, Fn&& fn) {
  agt_ctx_t ctx;
  agt_default_init(&ctx);

  benchmark fiberSwitchBenchmark{};
  fiberSwitchBenchmark.testName = std::string(name);
  std::stringstream argString;
  json argArray{ json::value_t::array };
  for (auto c : counts)
    argArray.push_back(c);
  argString << argArray;
  fiberSwitchBenchmark.testArgs = argString.str();


}


int main() {

  benchmark mathBenchmark{
    .testName = "math",
    .testArgs = "[10.5, 20.287, 45.1, 221.0, -2.387]",
  };

  benchmark nopBenchmark{
    .testName = "nop",
    .testArgs = "[1000, 500, 5000, 50, 10000, 25000]",
  };



  auto cyclesOverhead = bm::thread::getCyclesOverhead();



  std::thread nopThread{};

  for (auto arg : { 10.5, 20.287, 45.1, 221.0, -2.387 }) {
    auto& [argName, cycles ] = mathBenchmark.cycles.emplace_back();
    argName = std::to_string(arg);
    cycles = bm::thread::measure([arg](bm::state& state) {
      state.setIterationCount(1000000);
      for (auto _ : state)
        math_op(arg);
    });
  }


  for (auto arg : { 1000, 500, 5000, 50, 10000, 25000 }) {
    auto& [argName, cycles ] = nopBenchmark.cycles.emplace_back();
    argName = std::to_string(arg);
    cycles = bm::thread::measure([arg]( bm::state& state) {
      state.setIterationCount(10000);
      const size_t argVal = arg;
      for (auto& iter : state) {
        iter.start();
        // auto result = bm_nop_op(arg);
        bm_nopx4_op(argVal);
        bm::clobberMemory();
        iter.finish();
        // bm::doNotOptimize(result);
      }
    });
  }



  std::cout << mathBenchmark << std::endl;
  std::cout << nopBenchmark << std::endl;
  std::cout << "overhead: " << json(cyclesOverhead).dump(2) << std::endl;
}