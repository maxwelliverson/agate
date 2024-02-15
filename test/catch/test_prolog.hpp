//
// Created by Maxwell on 2024-02-09.
//

#ifndef AGATE_CATCH_TEST_PROLOG_HPP
#define AGATE_CATCH_TEST_PROLOG_HPP

#include <catch2/catch_tostring.hpp>
#include <catch2/catch_test_macros.hpp>
#include <magic_enum.hpp>

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

#endif //AGATE_CATCH_TEST_PROLOG_HPP
