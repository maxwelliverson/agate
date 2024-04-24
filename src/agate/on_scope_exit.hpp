//
// Created by maxwe on 2024-04-22.
//

#ifndef AGATE_ON_SCOPE_EXIT_HPP
#define AGATE_ON_SCOPE_EXIT_HPP

#include "config.hpp"

#include <concepts>

namespace agt::impl {

  template <std::invocable Fn>
  class on_scope_exit_st {
    Fn m_callback;
  public:
    template <typename FnTy>
    on_scope_exit_st(FnTy&& fn) noexcept : m_callback(std::forward<FnTy>(fn)) { }


    ~on_scope_exit_st() {
      std::invoke(m_callback);
    }
  };

  template <typename FnTy>
  on_scope_exit_st(FnTy&&) -> on_scope_exit_st<std::remove_cvref_t<FnTy>>;
}

#define on_scope_exit ::agt::impl::on_scope_exit_st _scope_exit_callback = [&]() mutable

#endif//AGATE_ON_SCOPE_EXIT_HPP
