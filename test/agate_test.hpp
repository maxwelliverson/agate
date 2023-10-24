//
// Created by maxwe on 2023-01-21.
//

#ifndef AGATE_AGATE_TEST_HPP
#define AGATE_AGATE_TEST_HPP

#include "agate.h"

#include <vector>
#include <iostream>
#include <span>
#include <chrono>

#include <thread> // to get hardware concurrency


namespace {

  namespace dtl {
    template <typename Agt>
    concept agent = requires(Agt* a, agt_self_t self, const void* buffer, size_t size) {
      (*a)(self, buffer, size);
    };

    template <typename Agt>
    concept stateless_agent = std::is_empty_v<Agt> &&
                              std::default_initializable<Agt>;

    template <typename Agt>
    concept has_static_message_size = requires {
      { Agt::static_message_size } -> std::convertible_to<size_t>;
    };

    template <typename Agt>
    concept has_init_function = requires(Agt& a, agt_self_t self) {
      a.init(self);
    };

    template <typename Agt>
    concept has_finalizer = requires(Agt& a, agt_self_t self){
      a.finalize(self);
    };

    template <typename Agt>
    concept has_non_trivial_dtor = !std::is_trivially_destructible_v<Agt>;

    template <typename Agt>
    concept has_state_type = requires{
      typename Agt::state_type;
    } && (sizeof(typename Agt::state_type) <= sizeof(void*));

    template <typename Agt>
    concept has_state_type_initializer = has_state_type<Agt> &&
                                         requires(agt_self_t self, typename Agt::state_type state){
                                           Agt::init(self, state);
                                         };

    template <typename Agt>
    concept has_stateless_initializer = !has_state_type_initializer<Agt> &&
                                        requires(agt_self_t self, typename Agt::state_type state){
                                          Agt::init(self);
                                        };

    template <typename Agt>
    concept has_state_type_finalizer = has_state_type<Agt> &&
                                       requires(agt_self_t self, typename Agt::state_type state) {
                                         Agt::finalize(self, state);
                                       };

    template <typename Agt>
    concept has_stateless_finalizer = !has_state_type_finalizer<Agt> &&
                                      requires(agt_self_t self) {
                                        Agt::finalize(self);
                                      };

    template <typename Agt>
    concept has_state_type_proc = has_state_type<Agt> &&
                                  requires(agt_self_t self,
                                           typename Agt::state_type state,
                                           const void* msg,
                                           size_t msgSize) {
                                    Agt::proc(self, state, msg, msgSize);
                                  };

    template <typename Agt>
    concept has_stateless_proc = !has_state_type_proc<Agt> &&
                                 requires(agt_self_t self, const void* msg, size_t msgSize) {
                                   Agt::proc(self, msg, msgSize);
                                 };

    template <typename Agt>
    concept has_default_state = (stateless_agent<Agt> && !has_state_type<Agt>) ||
                                (!stateless_agent<Agt> && std::constructible_from<Agt>);


    template <typename Agt, typename ...Args>
    void*            make_agent_state(Args&& ...args) noexcept {
      if constexpr (stateless_agent<Agt>) {
        if constexpr (has_state_type<Agt>)
          return (void*)(typename Agt::state_type(std::forward<Args>(args)...));
        else
          return nullptr;
      }
      else {
        return new Agt(std::forward<Args>(args)...);
      }
    }

    template <typename Agt>
    agt_agent_init_t get_agent_init() noexcept {
      if constexpr (stateless_agent<Agt>) {
        if constexpr (has_state_type_initializer<Agt>)
          return [](agt_self_t self, void* state){
            Agt::init(self, (typename Agt::state_type)state);
          };
        else if constexpr (has_stateless_initializer<Agt>)
          return [](agt_self_t self, void* state){
            (void)state;
            Agt::init(self);
          };
        else
          return nullptr;
      }
      else if constexpr (has_init_function<Agt>) {
        return [](agt_self_t self, void* state){
          static_cast<Agt*>(state)->init(self);
        };
      }
      else
        return nullptr;
    }

    template <typename Agt>
    agt_agent_proc_t get_agent_proc() noexcept {
      if constexpr (stateless_agent<Agt>) {
        if constexpr (has_state_type_proc<Agt>)
          return [](agt_self_t self, void* state, const void* msg, size_t msgSize){
            Agt::proc(self, (typename Agt::state_type)state, msg, msgSize);
          };
        else {
          static_assert(has_stateless_proc<Agt>);
          return [](agt_self_t self, void* state, const void* msg, size_t msgSize){
            Agt::proc(self, msg, msgSize);
          };
        }
      }
      else {
        return [](agt_self_t self, void* state, const void* msg, size_t msgSize){
          (*static_cast<Agt*>(state))(self, msg, msgSize);
        };
      }
    }

    template <typename Agt>
    agt_agent_dtor_t get_agent_dtor() noexcept {
      if constexpr (stateless_agent<Agt>) {
        if constexpr (has_state_type_finalizer<Agt>) {
          return [](agt_self_t self, void* state){
            Agt::finalize(self, (typename Agt::state_type)state);
          };
        }
        else if constexpr (has_stateless_finalizer<Agt>) {
          return [](agt_self_t self, void* state){
            Agt::finalize(self);
          };
        }
        else
          return nullptr;
      }
      else {
        return [](agt_self_t self, void* state){
          const auto agent = static_cast<Agt*>(state);
          if constexpr (has_finalizer<Agt>)
            agent->finalize(self);
          delete agent;
        };
      }
    }

    template <typename Agt>
    size_t get_static_message_size() noexcept {
      if constexpr (has_static_message_size<Agt>)
        return Agt::static_message_size;
      else
        return 0;
    }


    struct owner_t {
      agt_agent_t value;
    };

    template <typename T>
    struct state_t {
      T value;
    };

    struct executor_t {
      agt_executor_t value;
    };

    struct return_ptr_t {
      agt_agent_t* value;
    };

    struct busy_flag_t {};

    struct detached_flag_t {};


    template <template <typename...> typename TT, typename U>
    struct is_instantiation_of : std::false_type {};

    template <template <typename...> typename TT, typename ...Args>
    struct is_instantiation_of<TT, TT<Args...>> : std::true_type {};


    template <typename T, typename ...Args>
    concept cvref_contains_type = std::conjunction_v<std::is_same<T, std::remove_cvref_t<Args>>...>;

    template <template <typename...> typename TT, typename ...Args>
    concept cvref_contains_template = std::conjunction_v<is_instantiation_of<TT, std::remove_cvref_t<Args>>...>;


    struct agent_builder {
      agt_agent_create_info_t createInfo;
      agt_status_t            status;
      agt_async_t             boundAsyncHandle;
      agt_agent_t*            returnPtr;
      bool                    setNameToken;
      bool                    setOwner;
      bool                    setExecutor;
      bool                    setState;
    };


    template <typename T>
    struct remove_rvalue_reference : std::type_identity<T>{ };
    template <typename T>
    struct remove_rvalue_reference<T&&> : std::type_identity<T>{ };


    template <typename Agt>
    inline agt_status_t _do_process_arg(agt_ctx_t ctx, agent_builder& builder, return_ptr_t returnPtr) noexcept {
      builder.returnPtr = returnPtr.value;
    }
    template <typename Agt>
    inline agt_status_t _do_process_arg(agt_ctx_t ctx, agent_builder& builder, owner_t owner) noexcept {
      builder.createInfo.owner = owner.value;
      builder.setOwner = true;
      return AGT_SUCCESS;
    }
    template <typename Agt>
    inline agt_status_t _do_process_arg(agt_ctx_t ctx, agent_builder& builder, executor_t executor) noexcept {
      builder.createInfo.executor = executor.value;
      builder.setExecutor = true;
      return AGT_SUCCESS;
    }
    template <typename Agt>
    inline agt_status_t _do_process_arg(agt_ctx_t ctx, agent_builder& builder, const agt_name_desc_t& nameDesc) noexcept {
      agt_name_result_t result;
      auto status = agt_reserve_name(ctx, &nameDesc, &result);
      if (status == AGT_SUCCESS) {
        builder.createInfo.name = result.name;
        builder.setNameToken = true;
      }
      else if (status == AGT_DEFERRED) {
        builder.boundAsyncHandle = nameDesc.async;
      }
      return status;
    }
    template <typename Agt, typename T>
    inline agt_status_t _do_process_arg(agt_ctx_t ctx, agent_builder& builder, state_t<T> state) noexcept {
      static_assert(has_state_type<Agt>);
      static_assert(std::convertible_to<T, typename Agt::state_type>);

      if (builder.setState)
        return AGT_ERROR_INVALID_ARGUMENT;

      builder.createInfo.state = (void*)state.value;
      builder.setState = true;
      return AGT_SUCCESS;
    }
    template <typename Agt, typename ...Args>
    inline agt_status_t _do_process_arg(agt_ctx_t ctx, agent_builder& builder, std::tuple<Args...>&& args) noexcept {

      if (builder.setState)
        return AGT_ERROR_INVALID_ARGUMENT;

      builder.createInfo.state = std::apply(make_agent_state<Agt, Args...>, std::move(args));
      builder.setState = true;
      return AGT_SUCCESS;
    }

    template <typename Agt>
    inline agt_status_t _do_process_arg(agt_ctx_t ctx, agent_builder& builder, busy_flag_t) noexcept {
      builder.createInfo.flags |= AGT_AGENT_CREATE_BUSY;
      return AGT_SUCCESS;
    }

    template <typename Agt>
    inline agt_status_t _do_process_arg(agt_ctx_t ctx, agent_builder& builder, detached_flag_t) noexcept {
      builder.createInfo.flags |= AGT_AGENT_CREATE_DETACHED;
      return AGT_SUCCESS;
    }


    template <typename Agt>
    inline void _finish_process_args(agt_ctx_t ctx, agent_builder& builder) noexcept {

      if constexpr (has_default_state<Agt>) {
        if (!builder.setState)
          builder.createInfo.state = make_agent_state<Agt>();
      }
      else {
        assert( builder.setState );
      }

      if (!builder.setOwner)
        builder.createInfo.owner    = nullptr;
      if (!builder.setExecutor)
        builder.createInfo.executor = nullptr;
      if (!builder.setNameToken)
        builder.createInfo.name     = AGT_INVALID_NAME_TOKEN;
    }

    template <typename Agt>
    inline void _cleanup_failure(agt_ctx_t ctx, agent_builder& builder) noexcept {
      if (builder.status == AGT_DEFERRED) {
        agt_clear_async(builder.boundAsyncHandle);
      }
      else {
        if (builder.createInfo.name != AGT_INVALID_NAME_TOKEN)
          agt_release_name(ctx, builder.createInfo.name);
      }
      if constexpr (!stateless_agent<Agt>) {
        if (builder.createInfo.state)
          delete static_cast<Agt*>(builder.createInfo.state);
      }
    }

    template <typename Agt>
    inline void _process_args(agt_ctx_t ctx, agent_builder& builder) noexcept {
      _finish_process_args<Agt>(ctx, builder);
    }

    template <typename Agt, typename HeadArg, typename ...TailArg>
    inline void _process_args(agt_ctx_t ctx, agent_builder& builder, HeadArg&& head, TailArg&& ...tail) noexcept {
      auto tmpStatus = _do_process_arg<Agt>(ctx, builder, std::forward<HeadArg>(head));
      if (tmpStatus != AGT_SUCCESS && tmpStatus != AGT_DEFERRED) {
        _cleanup_failure<Agt>(ctx, builder);
        builder.status = tmpStatus;
        return;
      }

      // If the previously stored status was success, but tmpStatus is AGT_DEFERRED, set builder.status to AGT_DEFERRED
      if (builder.status == AGT_SUCCESS)
        builder.status = tmpStatus;

      _process_args<Agt>(ctx, builder, std::forward<TailArg>(tail)...);
    }
  }


  auto name(std::string_view name, agt_scope_t scope = AGT_INSTANCE_SCOPE) noexcept {
    return agt_name_desc_t {
        .async = nullptr,
        .name = {
            .data = name.data(),
            .length = name.length()
        },
        .flags = (agt_name_flags_t)scope
    };
  }

  auto name(std::string_view name, agt_async_t& async, agt_scope_t scope = AGT_INSTANCE_SCOPE) noexcept {
    return agt_name_desc_t {
        .async = &async,
        .name = {
            .data = name.data(),
            .length = name.length()
        },
        .flags = (agt_name_flags_t)scope
    };
  }

  auto owner(agt_agent_t agent) noexcept {
    return dtl::owner_t{ agent };
  }

  auto return_to(agt_agent_t& handleLocation) noexcept {
    return dtl::return_ptr_t{ &handleLocation };
  }

  auto executor(agt_executor_t exec) noexcept {
    return dtl::executor_t{ exec };
  }

  template <typename T>
  auto state(T val) noexcept {
    return dtl::state_t<T> { val };
  }

  template <typename ...Args>
  auto args(Args&& ...a) noexcept {
    return std::forward_as_tuple(std::forward<Args>(a)...);
  }

  inline constexpr dtl::busy_flag_t busy = {};

  inline constexpr dtl::detached_flag_t detached = {};



  template <typename Agt, typename ...Args>
  agt_status_t make_agent(agt_ctx_t ctx, Args&& ...args) noexcept {
    dtl::agent_builder builder{
        .status       = AGT_SUCCESS,
        .returnPtr    = nullptr,
        .setNameToken = false,
        .setOwner     = false,
        .setExecutor  = false,
        .setState     = false
    };
    dtl::_process_args<Agt>(ctx, builder, std::forward<Args>(args)...);

    if (builder.status != AGT_SUCCESS)
      return builder.status;

    return agt_create_agent(ctx, &builder.createInfo, builder.returnPtr);
  }

  template <typename Agt, typename ...Args>
  agt_status_t make_agent(agt_self_t self, Args&& ...args) noexcept {
    return make_agent<Agt>(agt_self_ctx(self), std::forward<Args>(args)...);
  }



  agt_ctx_t initialize_library() {

    agt_ctx_t ctx;

    uint32_t  hardwareThreadCount = std::thread::hardware_concurrency();
    uint32_t  minThreadCount = hardwareThreadCount == 1 ? 1 : 2;

    const char* requiredModules[] = {
        AGT_POOL_MODULE_NAME
    };

    agt_config_option_t configOptions[] = {
        {// Ensure that the shared library is not loaded to avoid said overhead
         // However, allow environment variable override so that benchmarks can be compared :)
            .id = AGT_CONFIG_IS_SHARED,
            .flags = AGT_ALLOW_ENVIRONMENT_OVERRIDE,
            .necessity = AGT_INIT_REQUIRED,
            .type = AGT_TYPE_BOOLEAN,
            .value = {
                .boolean = AGT_FALSE
            }
        },
        {// Ensure multithreaded library is loaded, though again, allow for envvar override for benchmarking purposes.
            .id = AGT_CONFIG_THREAD_COUNT,
            .flags = AGT_ALLOW_ENVIRONMENT_OVERRIDE | AGT_VALUE_GREATER_THAN_OR_EQUALS,
            .necessity = AGT_INIT_REQUIRED,
            .type = AGT_TYPE_UINT32,
            .value = {
                .uint32 = minThreadCount
            }
        },
        {
            .id = AGT_CONFIG_THREAD_COUNT,
            .flags = AGT_ALLOW_ENVIRONMENT_OVERRIDE | AGT_VALUE_LESS_THAN_OR_EQUALS,
            .necessity = AGT_INIT_REQUIRED,
            .type = AGT_TYPE_UINT32,
            .value = {
                .uint32 = hardwareThreadCount
            }
        }
    };

    auto config = agt_get_config(AGT_ROOT_CONFIG);

    agt_config_init_modules(config, AGT_INIT_REQUIRED, std::size(requiredModules), requiredModules);
    agt_config_set_options(config, std::size(configOptions), configOptions);


    agt_status_t result = agt_init(&ctx, config);
    assert( result == AGT_SUCCESS );

    return ctx;
  }

}

#endif//AGATE_AGATE_TEST_HPP
