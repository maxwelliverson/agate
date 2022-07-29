//
// Created by maxwe on 2022-05-31.
//

// #include <wrl/async.h>

#include "agate2.h"

#if defined(__clang__) && defined(__CLION_IDE__)
#define __cpp_lib_coroutine
#endif

#include <coroutine>
#include <optional>
#include <concepts>
#include <string_view>


inline constinit static agt_ctx_t g_agateCtx = nullptr;


class agent;


class agent {
protected:
  agent() {

  }
public:




  void set_handle(agt_agent_t handle) noexcept {
    this->handle = handle;
  }

private:
  agt_agent_t handle;
};


template <typename T>
class agent_ptr {
public:

  agent_ptr(T* p) noexcept;

  T& operator*()  const noexcept {
    return *ptr;
  }
  T* operator->() const noexcept {
    return *ptr;
  }



private:
  T* ptr;
};


template <typename T>
class message {
public:

};

template <typename, typename>
struct get_typename_impl_HELPER;
struct get_typename_impl_END;

template <typename T>
std::string_view get_typename_impl() noexcept {
  return __FUNCSIG__;
}

template <typename T>
std::string_view get_typename() noexcept {
  auto fullname = get_typename_impl<get_typename_impl_HELPER<T, get_typename_impl_END>>();
  const std::string_view helperPrefix = "get_typename_impl_HELPER<";
  const std::string_view helperSuffix = "get_typename_impl_END";
  fullname.remove_prefix(fullname.find(helperPrefix) + helperPrefix.size());
  return fullname.substr(0, fullname.find(helperSuffix) - 2);
}


template <typename Agt>
agt_type_id_t register_agent_type() noexcept {
  agt_agent_type_registration_info_t typeInfo;
  agt_type_id_t typeID;
  auto name = get_typename<Agt>();
  typeInfo.name           = name.data();
  typeInfo.nameLength     = name.size();
  typeInfo.dispatchKind   = AGT_NO_DISPATCH;
  typeInfo.flags          = 0;
  typeInfo.constructor    = nullptr;
  typeInfo.destructor     = [](void* p){
    delete static_cast<Agt*>(p);
  };
  typeInfo.noDispatchProc = [](void* pAgent, void* msg, const agt_agent_message_info_t* msgInfo) {
    // auto agent = static_cast<Agt*>(pAgent);
    std::coroutine_handle<> coroHandle = *static_cast<std::coroutine_handle<>*>(msg);
    coroHandle.resume();
    return coroHandle.done() ? AGT_ACTION_SUCCESS : AGT_ACTION_INCOMPLETE;
  };
  auto result = agt_register_type(g_agateCtx, &typeInfo, &typeID);
  assert(result == AGT_SUCCESS);
  return typeID;
}

template <typename Agt>
inline constinit static agt_type_id_t agent_type_id = {};


template <typename Agt, typename ...Args>
agent_ptr<Agt> make(std::string_view name, Args&& ...args) noexcept {
  if (!agent_type_id<Agt>)
    agent_type_id<Agt> = register_agent_type<Agt>();
  auto agentObject = new Agt(std::forward<Args>(args)...);

  agt_agent_create_info_t createInfo;
  createInfo.fixedMessageSize = 0; // Allow for dynamic message sizes
  createInfo.userData   = agentObject;
  createInfo.name       = name.data();
  createInfo.nameLength = name.length();
  createInfo.flags      = AGT_AGENT_CREATE_ALWAYS;
  createInfo.agency     = nullptr;
  createInfo.group      = nullptr;

  agt_agent_t handle;

  auto result = agt_create_agent_by_typeid(g_agateCtx, agent_type_id<Agt>, &createInfo, &handle);
  assert(result == AGT_SUCCESS);

  agentObject->set_handle(handle);

  return agent_ptr{agentObject};
}




template <typename T>
class async {

public:

  struct promise_type;

  /*async(const async& other)
      : coroHandle(other.coroHandle),
        agateAsync()
  {
    agt_copy_async(&other.agateAsync, &agateAsync);
    coroHandle.promise().set_async(agateAsync);
  }*/
  async(async&& other) noexcept
      : coroHandle(std::move(other.coroHandle)),
        agateAsync()
  {
    agt_move_async(&other.agateAsync, &agateAsync);
    coroHandle.promise().set_async(agateAsync);
  }

  ~async() {
    agt_destroy_async(&agateAsync);
  }


  auto operator co_await() noexcept;


private:

  explicit async(promise_type& promise) noexcept
      : coroHandle(std::coroutine_handle<promise_type>::from_promise(promise)),
        agateAsync()
  {
    agt_new_async(g_agateCtx, &agateAsync, 0);
    coroHandle.promise().set_async(agateAsync);
  }

  std::coroutine_handle<promise_type> coroHandle;
  agt_async_t                         agateAsync;
};

template <typename T>
struct async<T>::promise_type {

  async               get_return_object() { return async(*this); }
  std::suspend_always initial_suspend() noexcept { return {}; }
  std::suspend_never  final_suspend() noexcept { return {}; }
  template <std::convertible_to<T> U>
  void return_value(U&& val) noexcept {
    m_value.emplace(std::forward<U>(val));
  }
  void unhandled_exception() {}

  void set_async(agt_async_t& a) noexcept {
    pAsync = &a;
  }

  T&        value()       &  noexcept {
    return m_value;
  }
  const T&  value() const &  noexcept {
    return m_value;
  }
  T&&       value()       && noexcept {
    return std::move(m_value);
  }
  const T&& value() const && noexcept {
    return std::move(m_value);
  }

private:
  std::optional<T> m_value;
  agt_async_t*     pAsync;
};

template <typename T>
auto async<T>::operator co_await() noexcept  {
  struct awaitable {

    bool await_ready() noexcept {
      return agt_async_status(pAsync) != AGT_NOT_READY;
    }

    void await_suspend(std::coroutine_handle<> h) noexcept {

    }

    T await_resume() noexcept {
      return std::move(*promiseType).value();
    }

    promise_type* promiseType;
    agt_async_t*  pAsync;
  };
  return awaitable{ &coroHandle.promise(), &agateAsync };
}

template <>
struct async<void>::promise_type {
  async               get_return_object() { return async(*this); }
  std::suspend_always initial_suspend() noexcept { return {}; }
  std::suspend_never  final_suspend() noexcept { return {}; }
  void                return_void() noexcept { }
  void                unhandled_exception() { }

  void set_async(agt_async_t& a) noexcept {
    pAsync = &a;
  }

private:
  agt_async_t* pAsync;
};

template <>
auto async<void>::operator co_await() noexcept  {
  struct awaitable {

    bool await_ready() const noexcept {
      return agt_async_status(pAsync) != AGT_NOT_READY;
    }

    void await_suspend(std::coroutine_handle<> h) noexcept { }

    void await_resume() noexcept { }

    promise_type* promiseType;
    agt_async_t*  pAsync;
  };
  return awaitable{ &coroHandle.promise(), &agateAsync };
}


template <typename T, std::derived_from<agent> Agent, typename ...Args>
struct std::coroutine_traits<async<T>, Agent, Args...> {

};



async<int> user_input() noexcept {

}



int main() {

  if (agt_init(&g_agateCtx) != AGT_SUCCESS)
    return -1;






  return 0;
}