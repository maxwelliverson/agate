//
// Created by Maxwell on 2024-02-12.
//

#include "test_prolog.hpp"

#include "agate/init.h"
#include "agate/fiber.h"


#include <iostream>
#include <fstream>
#include <filesystem>

#undef INFO
#define INFO(...) std::cerr << __VA_ARGS__ << std::endl


struct user_data_base {
  uint64_t isInitial;
};

template <typename T>
struct user_data {
  uint64_t isInitial;
  T value;
};


// Some simple C++ wrappers to allow for lambda use


template <typename Fn> requires std::is_invocable_r_v<agt_fiber_param_t, Fn, agt_fiber_t, agt_fiber_param_t>
agt_fiber_param_t enter_fctx(agt_fiber_param_t initialParam, Fn&& fn) {
  using func_t = std::remove_cvref_t<Fn>;
  struct data_t {
    func_t func;
    agt_fiber_param_t result;
  };
  data_t data {
    std::forward<Fn>(fn),
    0
  };
  agt_fctx_desc_t desc{
    0,
    0,
    0,
    nullptr,
    [](agt_fiber_t fiber, agt_fiber_param_t param, void* userData) -> agt_fiber_param_t {
      auto pData = static_cast<data_t*>(userData);
      auto result = std::invoke_r<agt_fiber_param_t>(pData->func, fiber, param);
      pData->result = result;
      return result;
    },
    initialParam,
    &data
  };

  auto fctxResult = agt_enter_fctx(agt_ctx(), &desc, nullptr);
  return data.result;
}

template <typename Fn> requires std::is_invocable_r_v<agt_fiber_param_t, Fn, agt_fiber_t, agt_fiber_param_t>
std::pair<agt_status_t, agt_fiber_t> new_fiber(Fn&& fn) {
  using func_t = std::remove_cvref_t<Fn>;
  auto func = new func_t(std::forward<Fn>(fn));
  std::pair<agt_status_t, agt_fiber_t> results{ AGT_ERROR_UNKNOWN, nullptr };
  results.first = agt_new_fiber(&results.second,
    [](agt_fiber_t fiber, agt_fiber_param_t param, void* userData) -> agt_fiber_param_t {
      auto f = static_cast<func_t*>(userData);
      auto result = std::invoke_r<agt_fiber_param_t>(*f, fiber, param);
      // For now, just let the memory leak...
      return result;
    }, func);
  return results;
}

template <typename Fn> requires std::is_invocable_r_v<agt_fiber_param_t, Fn>
agt_fiber_transfer_t fiber_fork(Fn&& fn) {
  using func_t = std::remove_cvref_t<Fn>;
  func_t func {
    std::forward<Fn>(fn)
  };
  auto param = reinterpret_cast<agt_fiber_param_t>(&func);
  return agt_fiber_fork([](agt_fiber_t fiber, agt_fiber_param_t param, void* userData) -> agt_fiber_param_t {
    (void)fiber;
    (void)userData;
    auto f = reinterpret_cast<func_t*>(param);
    return std::invoke_r<agt_fiber_param_t>(*f);
  }, param, AGT_FIBER_SAVE_EXTENDED_STATE);
}

template <typename Fn> requires std::is_invocable_r_v<agt_fiber_param_t, Fn, agt_fiber_t, agt_fiber_param_t>
agt_fiber_param_t fiber_loop(agt_fiber_param_t initialParam, Fn&& fn) {
  using func_t = std::remove_cvref_t<Fn>;
  func_t func{std::forward<Fn>(fn)};

  void* oldUserData = agt_set_fiber_data(AGT_CURRENT_FIBER, &func);

  auto result = agt_fiber_loop([](agt_fiber_t fiber, agt_fiber_param_t param, void* userData) -> agt_fiber_param_t {
    auto f = static_cast<func_t*>(userData);
    return std::invoke_r<agt_fiber_param_t>(*f, fiber, param);
  }, initialParam, AGT_FIBER_SAVE_EXTENDED_STATE);

  agt_set_fiber_data(AGT_CURRENT_FIBER, oldUserData);

  return result;
}

agt_fiber_transfer_t fiber_switch(agt_fiber_t target, agt_fiber_param_t param) {
  return agt_fiber_switch(target, param, AGT_FIBER_SAVE_EXTENDED_STATE);
}

AGT_noreturn void fiber_jump(agt_fiber_t target, agt_fiber_param_t param) {
  agt_fiber_jump(target, param);
}

inline static agt_fiber_t this_fiber() {
  return agt_get_current_fiber(nullptr);
}

constexpr agt_fiber_param_t sequential_sum(agt_fiber_param_t initialValue, agt_fiber_param_t count) {
  // does a quick calculation of 'sum [initialValue..initialValue+count)'
  return (initialValue * count) + ((count * (count - 1)) / 2);
}



// fctx == fiber execution context

TEST_CASE("A normal thread may be converted into an fctx.", "[fibers]") {
  agt_ctx_t ctx;
  auto initResult = agt_default_init(&ctx);

  REQUIRE( initResult == AGT_SUCCESS );

  const agt_fiber_param_t initialParam = 0x7;

  void* userValue = nullptr;


  agt_fctx_desc_t desc {
    .flags = 0,
    .stackSize = 0,
    .maxFiberCount = 0,
    .parent = nullptr,
    .proc = [](agt_fiber_t fiber, agt_fiber_param_t param, void* userData) -> agt_fiber_param_t {
      REQUIRE( fiber != nullptr );
      REQUIRE( param == initialParam );
      REQUIRE( userData != nullptr );
      *static_cast<void**>(userData) = userData;
      auto currentFiber = agt_get_current_fiber(nullptr);
      REQUIRE( currentFiber != nullptr );
      REQUIRE( currentFiber == fiber );
      return initialParam;
    },
    .initialParam = initialParam,
    .userData = &userValue
  };

  auto fctxResult = agt_enter_fctx(ctx, &desc, nullptr);

  REQUIRE( fctxResult == AGT_SUCCESS );

  REQUIRE( userValue == &userValue );

  REQUIRE( agt_get_current_fiber(nullptr) == nullptr );

  agt_finalize(ctx);
}


TEST_CASE("Fibers in the same fctx can be switched between", "[fibers]") {
  agt_ctx_t ctx;
  agt_default_init(&ctx);

  constexpr static agt_fiber_param_t InitialParam = 0;
  constexpr static agt_fiber_param_t TotalSwitchCount = 0x4;
  constexpr static agt_fiber_param_t ExpectedSumResult = sequential_sum(InitialParam, TotalSwitchCount);

  auto result = enter_fctx(InitialParam, [](agt_fiber_t source, agt_fiber_param_t param) -> agt_fiber_param_t {

    REQUIRE( param == InitialParam );

    agt_fiber_param_t switchCount = param;
    agt_fiber_param_t sum = 0;

    const auto firstFiber = source;

    agt_status_t result;
    agt_fiber_t secondFiber;

    std::tie(result, secondFiber) = new_fiber([&](agt_fiber_t secondSource, agt_fiber_param_t param) mutable -> agt_fiber_param_t {

      REQUIRE( this_fiber() == secondFiber );
      REQUIRE( firstFiber == secondSource );
      REQUIRE( switchCount == param );

      sum += param;
      fiber_jump(firstFiber, param + 1);
    });

    REQUIRE ( result == AGT_SUCCESS );
    REQUIRE( secondFiber != nullptr );
    REQUIRE( firstFiber != secondFiber );

    while (switchCount < TotalSwitchCount) {
      auto [ switchSource, nextSwitchCount ] = fiber_switch(secondFiber, switchCount);

      REQUIRE( this_fiber() == firstFiber );
      REQUIRE( switchSource == secondFiber );
      REQUIRE( nextSwitchCount == switchCount + 1 );

      switchCount = nextSwitchCount;
    }

    auto destroyResult = agt_destroy_fiber(secondFiber);

    REQUIRE( destroyResult == AGT_SUCCESS );

    return sum;
  });

  REQUIRE( result == ExpectedSumResult );

  agt_finalize(ctx);
}


TEST_CASE("A fiber can fork with agt_fiber_fork", "[fibers]") {

  agt_ctx_t   ctx = nullptr;

  agt_default_init(&ctx);

  // INFO("Has initialized agate context");

  auto result = enter_fctx(1, [&](agt_fiber_t source, agt_fiber_param_t param) mutable -> agt_fiber_param_t {

    const auto firstFiber = this_fiber();
    bool hasForked = false;

    REQUIRE( firstFiber == source );
    REQUIRE( param == 1 );

    agt_status_t newFiberResult;
    agt_fiber_t otherFiber;

    std::tie(newFiberResult, otherFiber) = new_fiber([&](agt_fiber_t innerSrc, agt_fiber_param_t innerParam) -> agt_fiber_param_t {
      const auto secondFiber = this_fiber();

      REQUIRE( innerSrc == firstFiber );
      REQUIRE( secondFiber == otherFiber );
      REQUIRE( hasForked );
      REQUIRE( innerParam == 2 );

      auto transfer = fiber_switch(firstFiber, 3);

      REQUIRE( transfer.source == firstFiber );
      REQUIRE( transfer.param == 4 );

      fiber_jump(firstFiber, 5);
    });

    REQUIRE( newFiberResult == AGT_SUCCESS );

    // jump to other fiber
    auto transfer = fiber_fork([&]() mutable -> agt_fiber_param_t {
      hasForked = true;
      agt_fiber_jump(otherFiber, 2);
    });

    REQUIRE( transfer.source == otherFiber );
    REQUIRE( transfer.param == 3);

    // switch to other fiber and fallthrough
    transfer = fiber_fork([&]() mutable -> agt_fiber_param_t {

      auto forkTransfer = fiber_switch(otherFiber, 4);

      REQUIRE( forkTransfer.source == otherFiber );
      REQUIRE( forkTransfer.param == 5 );

      return 6;
    });

    REQUIRE( transfer.source == firstFiber );
    REQUIRE( transfer.param == 6);

    agt_destroy_fiber(otherFiber);

    return 0;
  });

  REQUIRE( result == AGT_SUCCESS );

  agt_finalize(ctx);

}