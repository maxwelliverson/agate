//
// Created by maxwe on 2024-04-23.
//

#ifndef AGATE_DEBUG_HPP
#define AGATE_DEBUG_HPP

#include "config.hpp"

#include <syncstream>
#include <iostream>

#if defined(__cpp_lib_debugging) && (__cpp_lib_debugging >= 202311L)
#include <debugging>
# define AGT_has_debugging true
# define AGT_breakpoint() std::breakpoint_if_debugging()
#else
# define AGT_has_debugging false
# define AGT_breakpoint() (void)0
#endif

#if defined(__cpp_lib_stacktrace) && (__cpp_lib_stacktrace >= 202011L)
#include <stacktrace>
# define AGT_has_stacktrace true
#else
# define AGT_has_stacktrace false
#endif

#if defined(__cpp_lib_source_location) && (__cpp_lib_source_location >= 201907L)
#include <source_location>
# define AGT_has_source_location true
#else
# define AGT_has_source_location false
#endif


#if defined(NDEBUG)
# if AGT_has_source_location
#  define AGT_not_implemented() ::agt::debug::break_on_unimplemented()
# else
#  define AGT_not_implemented() ::agt::debug::break_on_unimplemented(__FUNCSIG__)
# endif
#else
# if AGT_has_source_location
#  define AGT_not_implemented() ::agt::debug::break_on_unimplemented()
# else
#  define AGT_not_implemented() ::agt::debug::break_on_unimplemented(__FUNCSIG__)
# endif
// # define AGT_return_not_implemented() AGT_not_implemented()
#endif
#define AGT_return_not_implemented() do { AGT_breakpoint(); return AGT_ERROR_NOT_YET_IMPLEMENTED; } while(false)

namespace agt::debug {
#if AGT_has_source_location
  [[noreturn]] inline void break_on_unimplemented(std::source_location loc = std::source_location::current()) noexcept {
    AGT_breakpoint();
    {
      auto& out = std::cerr;
      // std::osyncstream out{std::cerr};
      out << "Function '" << loc.function_name() << "' is not yet implemented.\n\n";
#if AGT_has_stacktrace
      auto stacktrace = std::stacktrace::current(2);
      out << stacktrace << std::endl;
#endif
      // out.emit();
    }
    exit(-1);
  }
#else
  [[noreturn]] inline void break_on_unimplemented(const char* func) noexcept {
    AGT_breakpoint();
    {
      auto& out = std::cerr;
      // std::osyncstream out{std::cerr};
      out << "Function '" << func << "' is not yet implemented.\n\n";
#if AGT_has_stacktrace
      auto stacktrace = std::stacktrace::current(2);
      out << stacktrace << std::endl;
#endif
      // out.emit();
    }
    std::cerr.flush();
    exit(-1);
  }
#endif
}

#endif//AGATE_DEBUG_HPP
