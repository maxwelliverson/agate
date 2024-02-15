//
// Created by maxwe on 2023-02-28.
//

#ifndef AGATE_INTERNAL_THREAD_HPP
#define AGATE_INTERNAL_THREAD_HPP

// A class wrapping a system thread, much in the same vein as std::thread.
// The main reason I wrote a custom implementation instead of using std::thread
// is so that threads may be created in a suspended state, which is necessary for
// implementing executors.

#include "config.hpp"
#include "thread_id.hpp"
#include "dllexport.hpp"

#include <tuple>
#include <atomic>

namespace agt {

  class AGT_dllexport thread {

#if AGT_system_windows
  public:
    using handle_type = void*;
  private:
    using proc_rettype = unsigned long;
    using proc_argtype = void*;
#else
  public:
    using handle_type  = int;
  private:
    using proc_rettype = void*;
    using proc_argtype = void*;
#endif
  private:



    template <typename Fn, typename ...Args>
    static proc_rettype proc(proc_argtype arg) noexcept {
      using info_type = start_info<Fn, Args...>;
      auto pInfo = static_cast<info_type*>(arg);
      set_thread_info(pInfo);
      invoke_callback(pInfo, std::make_index_sequence<sizeof...(Args)>{});
      if (!--pInfo->refCount)
        delete pInfo;
      return 0;
    }

    struct thread_info {
      std::atomic_bool     startedFlag = false;
      bool                 started = false;
      std::atomic_uint32_t refCount = 2;
      uintptr_t            tid = 0;
    };

    static void set_thread_info(thread_info* pInfo) noexcept;

    template <typename T>
    using arg_wrapper_t = std::conditional_t<std::is_lvalue_reference_v<T>, std::add_pointer_t<T>, T>;

    template <typename T>
    static T                     wrap_fwd(std::type_identity_t<T>&& arg) noexcept {
      return std::move(arg);
    }
    template <typename T>
    static std::add_pointer_t<T> wrap_fwd(std::remove_reference_t<T>& arg) noexcept {
      return std::addressof(arg);
    }

    template <typename T>
    static T unwrap_fwd(std::add_pointer_t<T> arg) noexcept {
      return *arg;
    }
    template <typename T>
    static T unwrap_fwd(std::type_identity_t<T>&& arg) noexcept {
      return std::move(arg);
    }

    template <typename Fn, typename ...Args>
    struct start_info : thread_info {
      std::remove_cvref_t<Fn> functor;
      std::tuple<arg_wrapper_t<Args>...> args;
    };

    template <typename Fn, typename ...Args, size_t ...I>
    static void invoke_callback(start_info<Fn, Args...>* info, std::index_sequence<I...>) noexcept {
      std::invoke(std::forward<Fn>(info->functor), unwrap_fwd<Args>(std::get<I>(info->args))...);
    }

    uintptr_t    m_id;
    handle_type  m_handle;
    thread_info* m_pInfo;

    template <typename Fn, typename ...Args>
    static thread_info* make_info(Fn&& fn, Args&& ...args) noexcept {
      using info_type = start_info<Fn, Args...>;
      return new info_type(std::forward<Fn>(fn), std::make_tuple(wrap_fwd<Args>(args)...));
    }


    void do_start(proc_rettype(* pProc)(proc_argtype), thread_info* threadInfo, bool startSuspended) noexcept;

  public:

    thread() = default;

    [[nodiscard]] bool started() const noexcept {
      if (!m_pInfo->started)
        m_pInfo->started = m_pInfo->startedFlag.load();
      return m_pInfo->started;
    }

    void resume() noexcept;



    template <typename Fn, typename ...Args>
    static thread start(Fn&& fn, Args&& ...args) noexcept {
      auto info = make_info(std::forward<Fn>(fn), std::forward<Args>(args)...);
      thread t;
      t.do_start(&proc<Fn, Args...>, info, false);
      return std::move(t);
    }
    template <typename Fn, typename ...Args>
    static thread start_suspended(Fn&& fn, Args&& ...args) noexcept {
      auto info = make_info(std::forward<Fn>(fn), std::forward<Args>(args)...);
      thread t;
      t.do_start(&proc<Fn, Args...>, info, true);
      return std::move(t);
    }

  };


}

#endif//AGATE_INTERNAL_THREAD_HPP
