//
// Created by maxwe on 2023-08-07.
//

#include "agate/naming.h"
#include "agate/uexec.h"

#include <type_traits>
#include <bit>
#include <concepts>
#include <expected>
#include <string_view>


template <typename T>
using result = std::expected<T, agt_status_t>;



/**
 * void            (* init)(void* userData, agt_equeue_t equeue);
agt_eagent_t    (* attachAgent)(void* userData, const agt_agent_info_t* agentInfo);
void            (* detachAgent)(void* userData, agt_eagent_t agentTag);
agt_eagent_t    (* getReadyAgent)(void* userData);
agt_bool_t      (* dispatchMessage)(void* userData, agt_ecmd_t cmd, agt_address_t paramA, agt_address_t paramB);
void            (* readyAgent)(void* userData, agt_eagent_t agent);
void            (* blockAgent)(void* userData, agt_eagent_t agent);
void            (* blockAgentUntil)(void* userData, agt_eagent_t agent, agt_timestamp_t timestamp);*/

class eagent {
  void*        reserved;
  agt_efiber_t fiber;
public:

};

struct custom_executor_info {
  agt_executor_proc_t proc;
  void*               userData;
  agt_receiver_t      receiver;
  agt_executor_kind_t kind;
  agt_u32_t           stackSize;
  agt_u32_t           maxBoundAgents;
  agt_u32_t           maxConcurrency;
};

class custom_executor {
protected:

  ~custom_executor() = default;
public:

  virtual void       init(agt_equeue_t equeue) noexcept = 0;

  virtual eagent*    attachAgent(const agt_agent_info_t& agentInfo) noexcept = 0;

  virtual void       detachAgent(eagent* agent) noexcept = 0;

  virtual eagent*    getReadyAgent() noexcept = 0;

  virtual agt_bool_t dispatchMessage(agt_ecmd_t ecmd, agt_address_t paramA, agt_address_t paramB) noexcept = 0;

  virtual void       readyAgent(eagent* agent) noexcept = 0;

  virtual void       blockAgent(eagent* agent) noexcept = 0;

  virtual void       blockAgentUntil(eagent* agent, agt_timestamp_t timestamp) noexcept = 0;
};


template <typename T, typename C>
auto extract_member_type(T C::*) -> std::type_identity<T>;
template <typename R, typename C, typename ...Args>
auto extract_member_type(R (C::* mptr)(Args...)) -> std::type_identity<R(C*, Args...)> requires(sizeof(mptr) == sizeof(void*));
template <typename R, typename C, typename ...Args>
auto extract_member_type(R (C::* mptr)(Args...) noexcept) -> std::type_identity<R(C*, Args...) noexcept> requires(sizeof(mptr) == sizeof(void*));
template <typename R, typename C, typename ...Args>
auto extract_member_type(R (C::* mptr)(Args...) const) -> std::type_identity<R(const C*, Args...)> requires(sizeof(mptr) == sizeof(void*));
template <typename R, typename C, typename ...Args>
auto extract_member_type(R (C::* mptr)(Args...) const noexcept) -> std::type_identity<R(const C*, Args...) noexcept> requires(sizeof(mptr) == sizeof(void*));



#define AGT_member_type(type_, member_) typename decltype(extract_member_type(&type_::member_))::type

#define AGT_executor_fn(type_, name_) .name_ = std::bit_cast<AGT_member_type(agt_executor_vtable_t, name_)>(&type_::name_)


template <typename T, typename U = std::remove_cvref_t<T>>
struct get_fwd_helper {
  using type = T;

  constexpr static inline std::remove_reference_t<T>&& fwd(std::remove_reference_t<T>&& value) noexcept {
    return std::move(value);
  }

  constexpr static inline std::remove_reference_t<T>&& fwd(std::remove_reference_t<T>& value) noexcept {
    return std::move(value);
  }
};

template <typename T, typename V>
struct get_fwd_helper<T, std::reference_wrapper<V>> {
  using type = V&;

  constexpr static inline V& fwd(T&& value) noexcept {
    return *value;
  }
};

template <size_t I, typename Tup>
decltype(auto) get_fwd(Tup* args) noexcept {
  return get_fwd_helper<std::tuple_element_t<I, Tup>>::fwd(std::get<I>(*args));
}

template <typename E, typename Tup, size_t ...I>
E* make_new_lazy_(Tup* args, std::index_sequence<I...>) noexcept {
  auto result = new E(get_fwd<I>(args)...);
  delete args;
  return result;
}

template <typename E, typename Tup>
E* make_new_lazy(Tup* args) noexcept {
  return make_new_lazy_<E>(args, std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Tup>>>{});
}

template <typename ...Args>
std::tuple<Args...>* make_args(Args&& ...args) noexcept {
  if constexpr (sizeof...(Args) == 0)
    return nullptr;
  else
    return new std::tuple<Args...>(std::forward<Args>(args)...);
}


template <typename E>
void executor_proc_fn(agt_ctx_t ctx, agt_equeue_t equeue, void* userData) noexcept {
  auto exec = new E();
  agt_equeue_bind_user_data(equeue, exec);
  agt_equeue_dispatch(equeue, AGT_WAIT);
}

template <typename E, typename ...Args> requires (sizeof...(Args) > 0)
void executor_proc_fn(agt_ctx_t ctx, agt_equeue_t equeue, void* userData) noexcept {
  auto exec = make_new_lazy<E>(static_cast<std::tuple<Args...>*>(userData));
  agt_equeue_bind_user_data(equeue, exec);
  agt_equeue_dispatch(equeue, AGT_WAIT);
}


template <typename T>
inline static const agt_executor_vtable_t* get_vtable() noexcept {
  const static agt_executor_vtable_t vtable {
      AGT_executor_fn(T, init),
      AGT_executor_fn(T, attachAgent),
      AGT_executor_fn(T, detachAgent),
      AGT_executor_fn(T, getReadyAgent),
      AGT_executor_fn(T, dispatchMessage),
      AGT_executor_fn(T, readyAgent),
      AGT_executor_fn(T, blockAgent),
      AGT_executor_fn(T, blockAgentUntil)
  };
  return &vtable;
}

template <std::derived_from<custom_executor> E, typename ...Args>
result<agt_executor_t> make_executor(Args&& ...args) noexcept {
  auto ctx = agt_ctx();
  agt_executor_t executor;
  agt_status_t status;
  auto argTuple = make_args(std::forward<Args>(args)...);

  custom_executor_info execInfo{
      .proc           = nullptr,
      .userData       = nullptr,
      .receiver       = nullptr,
      .kind           = AGT_DEFAULT_EXECUTOR_KIND,
      .stackSize      = 0,
      .maxBoundAgents = 0,
      .maxConcurrency = 0
  };

  E::getInfo(execInfo);

  bool isMultiThreaded = false;

  if (execInfo.kind == AGT_BUSY_EXECUTOR_KIND || execInfo.kind == AGT_PARALLEL_EXECUTOR_KIND)
    if (execInfo.maxConcurrency > 1)
      isMultiThreaded = true;

  agt_custom_executor_params_t customParams{
      .vtable         = get_vtable<E>(),
      .proc           = execInfo.proc,
      .userData       = execInfo.userData,
      .receiver       = execInfo.receiver
  };

  agt_executor_desc_t desc {
      .flags          = isMultiThreaded ? AGT_EXECUTOR_IS_THREAD_SAFE : 0u,
      .kind           = execInfo.kind,
      .name           = AGT_ANONYMOUS,
      .maxBoundAgents = execInfo.maxBoundAgents,
      .stackSize      = execInfo.stackSize,
      .maxConcurrency = execInfo.maxConcurrency,
      .params   = &customParams
  };


  status = agt_new_executor(ctx, &executor, &desc);
  if (status != AGT_SUCCESS) {
    delete argTuple;
    return std::unexpected(status);
  }

  return executor;
}

template <std::derived_from<custom_executor> E, typename ...Args>
result<agt_executor_t> make_named_executor(std::string_view name, Args&& ...args) noexcept {
  auto ctx = agt_ctx();
  agt_executor_t executor;
  auto obj = new E(std::forward<Args>(args)...);

  agt_name_t   nameTok;
  agt_status_t status;

  if (name.empty())
    nameTok = AGT_ANONYMOUS;
  else {
    agt_name_desc_t nameDesc{
        .async = nullptr,
        .name = {
            .data   = name.data(),
            .length = name.length()
        },
        .flags = 0
    };
    agt_name_result_t nameResult;
    status = agt_reserve_name(ctx, &nameDesc, &nameResult);
    if (status != AGT_SUCCESS)
      return std::unexpected(status);
    nameTok = nameResult.name;
  }

  agt_custom_executor_params_t customParams{
      .vtable         = get_vtable<E>(),
      .receiver       = AGT_NULL_HANDLE,
      .userData       = obj
  };

  agt_executor_desc_t desc{
      .flags          = 0,
      .kind           = AGT_CUSTOM_EXECUTOR_KIND,
      .name           = nameTok,
      .maxBoundAgents = obj->getMaximumBoundAgents(),
      .stackSize      = 0,
      .extraParams    = &customParams
  };

  status = agt_new_executor(ctx, &executor, &desc);

  if (status != AGT_SUCCESS) {
    if (nameTok != AGT_ANONYMOUS)
      agt_release_name(ctx, nameTok);
    delete obj;
    return std::unexpected(status);
  }

  return executor;
}



class fiber_executor : public custom_executor {
  agt_executor_t    executor;
  agt_equeue_t      equeue;
  agt_efiber_list_t idleList;


  static void fiber_proc(agt_ctx_t ctx, agt_efiber_proc_flags_t flags, agt_u64_t param, void* userData) noexcept {
    static_cast<fiber_executor*>(userData)->loop(flags, param, ctx);
  }
public:

  void init(agt_equeue_t equeue_) noexcept override {
    this->equeue = equeue_;
  }

  eagent* attachAgent(const agt_agent_info_t& agentInfo) noexcept override {

  }

  void detachAgent(eagent* agent) noexcept override {

  }



  void loop(agt_efiber_proc_flags_t flags, agt_u64_t param, agt_ctx_t ctx) noexcept {
    bool hasLooped = false;
    bool isBlocked = flags & AGT_EFIBER_PROC_IS_BLOCKED;
    do {
      if (hasLooped || isBlocked) {

      }

      hasLooped = true;
    } while(true);
  }

  static void start(agt_equeue_t executor) noexcept {
    auto ctx = agt_ctx();

    agt_efiber_flags_t flags;
    agt_u32_t          stackSize;
    agt_u32_t          maxFiberCount;
    agt_efiber_t       parentFiber;
    agt_efiber_proc_t  proc;
    agt_u64_t          initialParam;
    void*              userData;

    agt_efiber_desc_t fiberDesc{
        .flags         = 0,
        .stackSize     = 0,
        .maxFiberCount = 0,
        .parentFiber   = nullptr,
        .proc          = &fiber_executor::fiber_proc,
        .initialParam  = 0,
        .userData      = nullptr
    };
  }

};




int main() {

  auto vptr = &custom_executor::init;


  agt_executor_t executor;

  if (auto maybeExecutor = make_executor<>()) {

  }

}