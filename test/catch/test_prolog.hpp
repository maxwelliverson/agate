//
// Created by Maxwell on 2024-02-09.
//

#ifndef AGATE_CATCH_TEST_PROLOG_HPP
#define AGATE_CATCH_TEST_PROLOG_HPP

#include <catch2/catch_tostring.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <magic_enum.hpp>

#include "agate/init.h"

#define AGT_register_enum_strings(type_) \
  template <> \
  struct Catch::StringMaker<type_> { \
    static std::string convert(type_ val_) { return std::string(magic_enum::enum_name(val_)); } \
  }


#define AGT_register_bitflag_strings(type_) \
  template <> \
  struct Catch::StringMaker<type_> { \
    static std::string convert(type_ val_) { return std::string(magic_enum::enum_name<magic_enum::as_flags<true>>(val_)); } \
  }


// Contain the context in a class so that agt_finalize can be called by the destructor.
// This ensures that the context is finalized regardless of how the test case exits
// (if agt_finalize were simply called at the end of the test case, any failed tests would fail to
// close the context).
class context {
  agt_ctx_t m_ctx = nullptr;
public:
  context() {
    auto status = agt_default_init(&m_ctx);
    REQUIRE( status == AGT_SUCCESS );
  }

  ~context() {
    auto status = agt_finalize(m_ctx);
    REQUIRE( status == AGT_SUCCESS );
  }

  operator agt_ctx_t() const noexcept {
    return m_ctx;
  }
};

#endif //AGATE_CATCH_TEST_PROLOG_HPP
