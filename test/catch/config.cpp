//
// Created by Maxwell on 2024-02-03.
//

#include "test_prolog.hpp"

#include "agate/init.h"
#include "dll_interface.hpp"

#include <mutex>
#include <thread>


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


class execution_path {
  std::mutex m_mutex;
public:
  void start() noexcept {
    m_mutex.lock();
  }

  void yield() noexcept {
    m_mutex.unlock();
    m_mutex.lock();
  }

  void finish() noexcept {
    m_mutex.unlock();
  }
};

static bool moduleIsLoaded(const char* name) noexcept {
  const auto module = GetModuleHandle(name);
  return module != nullptr;
  // return GetModuleHandle(name) != nullptr;
}



AGT_register_enum_strings(agt_status_t);

AGT_register_enum_strings(agt_value_type_t);

AGT_register_enum_strings(agt_object_type_t);


struct init_info_t {
  agt_ctx_t ctx = nullptr;
  bool      initializedByCallback = false;
};

static void init_callback(agt_ctx_t ctx, void* userData) noexcept {
  auto pInfo = static_cast<init_info_t*>(userData);
  pInfo->ctx = ctx;
  pInfo->initializedByCallback = true;
}

TEST_CASE("Can be default-initialized", "[init]") {

  init_info_t info;

  agt_status_t result;

  SECTION("Using agt_default_init") {
    result = agt_default_init(&info.ctx);

    REQUIRE( not info.initializedByCallback );
  }

  SECTION("Using agt_default_init_with_callback") {
    result = agt_default_init_with_callback(init_callback, &info);

    REQUIRE( info.initializedByCallback );
  }

  REQUIRE(result == AGT_SUCCESS);
  REQUIRE(info.ctx != nullptr);

  agt_status_t finalizeResult = agt_finalize(info.ctx);
  REQUIRE(finalizeResult == AGT_SUCCESS);
}

TEST_CASE("Can be config initialized", "[init]") {

  agt_config_t config = agt_get_config(AGT_ROOT_CONFIG);

  init_info_t info;

  agt_status_t result;

  SECTION("Using agt_init") {
    result = agt_init(&info.ctx, config);

    REQUIRE( not info.initializedByCallback );
  }

  SECTION("Using agt_init_with_callback") {
    result = agt_init_with_callback(config, init_callback, &info);

    REQUIRE( info.initializedByCallback );
  }

  REQUIRE(result == AGT_SUCCESS);
  REQUIRE(info.ctx != nullptr);

  agt_status_t finalizeResult = agt_finalize(info.ctx);
  REQUIRE(finalizeResult == AGT_SUCCESS);
}

TEST_CASE("Client can retrieve pointers to API functions", "[init]") {
  /*agt_ctx_t    context = nullptr;



  agt_status_t result;
  agt_proc_t   pfn;

  pfn = agt_get_proc_address();*/
}

TEST_CASE("Client can initialize optional modules", "[init]") {
  constexpr static const char* ValidModules[] = { "log" };
  constexpr static const char* InvalidModules[] = { " ### " };

  agt_ctx_t    context = nullptr;
  agt_config_t config = agt_get_config(AGT_ROOT_CONFIG);
  agt_status_t result;
  agt_proc_t   pfnLog;

  SECTION("Requesting a valid module name results in success") {
    agt_config_init_modules(config, AGT_INIT_REQUIRED, std::size(ValidModules), ValidModules);
    result = agt_init(&context, config);
    REQUIRE( result == AGT_SUCCESS );
    REQUIRE( context != nullptr );

    pfnLog = agt_get_proc_address("agt_log");
    REQUIRE( pfnLog != nullptr );
  }

  SECTION("Requiring an invalid module name results in failure") {
    agt_config_init_modules(config, AGT_INIT_REQUIRED, std::size(InvalidModules), InvalidModules);
    result = agt_init(&context, config);

    REQUIRE( result == AGT_ERROR_MODULE_NOT_FOUND );
    REQUIRE( context == nullptr );

    // note that agt_get_proc_address is a part of the loader API, and as such may still be called without a valid context.
    pfnLog = agt_get_proc_address("agt_log");
    REQUIRE( pfnLog == nullptr );
  }

  SECTION("Optionally requesting an invalid module name results in success") {
    agt_config_init_modules(config, AGT_INIT_OPTIONAL, std::size(InvalidModules), InvalidModules);
    result = agt_init(&context, config);

    REQUIRE( result == AGT_SUCCESS );
    REQUIRE( context != nullptr );

    pfnLog = agt_get_proc_address("agt_log"); // The agt-log module wasn't loaded, so this should return null
    REQUIRE( pfnLog == nullptr );
  }

  if (context)
    agt_finalize(context);
  pfnLog = agt_get_proc_address("agt_log");
  REQUIRE( pfnLog == nullptr ); // should always be null after the library is destroyed
}

/*TEST_CASE("Linked libraries are unloaded when instance is destroyed manually", "[init]") {
  execution_path executionPath;
  executionPath.start();
  std::thread worker{[&executionPath]() mutable {
    executionPath.start();
    agt_ctx_t ctx;
    agt_default_init(&ctx);
    executionPath.yield();
    agt_finalize(ctx);
    executionPath.yield();
    executionPath.finish();
  }};
  executionPath.yield();

  REQUIRE( moduleIsLoaded("agate-core") );

  executionPath.yield();

  REQUIRE_FALSE( moduleIsLoaded("agate-core") );

  executionPath.finish();

  worker.join();
}

TEST_CASE("Linked libraries are unloaded when instance is destroyed automatically", "[init]") {
  execution_path executionPath;
  executionPath.start();
  std::thread worker{[&executionPath]() mutable {
    executionPath.start();
    agt_ctx_t ctx;
    agt_default_init(&ctx);
    executionPath.yield();
    executionPath.finish();
  }};
  executionPath.yield();

  REQUIRE( moduleIsLoaded("agate-core.dll") );

  executionPath.yield();
  executionPath.finish();
  worker.join();

  REQUIRE_FALSE( moduleIsLoaded("agate-core.dll") );
}*/

TEST_CASE("Client may load agate-linked DLLs", "[init]") {
  agt_config_t config = agt_get_config(AGT_ROOT_CONFIG);

  agt_ctx_t   ctx = nullptr;
  agt_ctx_t   firstCtx = nullptr;
  agt_ctx_t   secondCtx = nullptr;
  agt_ctx_t   thirdCtx = nullptr;

  agt_status_t result;

  SECTION("Loading a single DLL") {
    result = dll::initSecondLibrary(config, secondCtx);

    REQUIRE( result == AGT_DEFERRED );
    REQUIRE( secondCtx == nullptr );

    result = agt_init(&ctx, config);

    REQUIRE( result == AGT_SUCCESS );
    REQUIRE( ctx != nullptr );
    REQUIRE( secondCtx != nullptr );
  }

  SECTION("Chain loading DLLs") {
    result = dll::initFirstLibrary(config, false, firstCtx, thirdCtx);

    REQUIRE( result == AGT_DEFERRED );
    REQUIRE( firstCtx == nullptr );
    REQUIRE( thirdCtx == nullptr );

    result = agt_init(&ctx, config);

    REQUIRE( result == AGT_SUCCESS );
    REQUIRE( ctx != nullptr );
    REQUIRE( firstCtx != nullptr );
    REQUIRE( thirdCtx != nullptr );
  }

  SECTION("DLL loaded multiple times") {

  }


  if (ctx)
    REQUIRE( agt_finalize(ctx) == AGT_SUCCESS );

  if (firstCtx && firstCtx != ctx) // eventually we want to be able to get rid of the second clauses here, but for now, a ctx can only be finalized once.
    REQUIRE( dll::closeFirstLibrary(firstCtx) == AGT_SUCCESS );

  if (secondCtx && secondCtx != ctx)
    REQUIRE( dll::closeSecondLibrary(secondCtx) == AGT_SUCCESS );

  if (thirdCtx && thirdCtx != ctx)
    REQUIRE( dll::closeThirdLibrary(thirdCtx) == AGT_SUCCESS );
}

