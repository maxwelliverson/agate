//
// Created by maxwe on 2023-01-26.
//

#ifndef AGATE_EXAMPLE_CXX_WRAPPER_ASYNC_HPP
#define AGATE_EXAMPLE_CXX_WRAPPER_ASYNC_HPP

#include <agate/agents.h>
#include <agate/async.h>
#include <agate/naming.h>
#include <agate/signal.h>

#include <cstring>
#include <string_view>
#include <optional>
#include <chrono>

namespace agtcxx {

  class context;

  class instance {
    agt_instance_t m_instance;
  public:
    instance() noexcept : m_instance(AGT_INVALID_INSTANCE) { }

    [[nodiscard]] agt_instance_t raw_handle() const noexcept {
      return m_instance;
    }
  };

  class self {
    agt_self_t m_self;

    friend class context;
    explicit self(agt_self_t s) noexcept : m_self(s) { }
  public:

    [[nodiscard]] context    context() const noexcept;

    [[nodiscard]] agt_self_t raw_handle() const noexcept {
      return m_self;
    }
  };

  class context {

    friend class self;

    agt_ctx_t m_ctx = AGT_INVALID_CTX;
    explicit context(agt_ctx_t ctx) noexcept : m_ctx(ctx) { }
  public:
    context() = default;
    context(const context&) = default;
    context(context&&) noexcept = default;

    context(std::nullptr_t) noexcept { }


    static context current() noexcept {
      return context(agt_ctx());
    }

    static context acquire(instance inst = {}) noexcept {
      return context(agt_acquire_ctx(inst.raw_handle()));
    }

    static context null() noexcept {
      return context(nullptr);
    }

    [[nodiscard]] self      self() const noexcept {
      return agtcxx::self(agt_self(m_ctx));
    }

    [[nodiscard]] agt_ctx_t raw_handle() const noexcept {
      return m_ctx;
    }
  };

  context self::context() const noexcept {
    return agtcxx::context(agt_self_ctx(m_self));
  }


  class async;

  class signal {
    agt_signal_t m_signal;

    explicit signal(agt_signal_t sig) noexcept : m_signal(sig) { }

    friend class async;
  public:

    signal() noexcept : signal(context::null()) { }
    explicit signal(context ctx, agt_signal_flags_t flags = 0) noexcept {
      agt_new_signal(ctx.raw_handle(), &m_signal, AGT_ANONYMOUS, flags);
    }
    signal(agt_ctx_t ctx, agt_signal_flags_t flags = 0) noexcept {
      agt_new_signal(ctx, &m_signal, AGT_ANONYMOUS, flags);
    }

    signal(const signal&) = delete;
    signal(signal&& other) noexcept : m_signal(other.m_signal) { other.m_signal = nullptr; }

    ~signal() {
      if (m_signal)
        agt_close_signal(m_signal);
    }

    static std::optional<signal> open(std::string_view name, agt_scope_t scope = AGT_INSTANCE_SCOPE, agt_signal_flags_t flags = 0) noexcept {
      return signal::open(context::current(), name, scope, flags);
    }

    static std::optional<signal> open(context ctx, std::string_view name, agt_scope_t scope = AGT_INSTANCE_SCOPE, agt_signal_flags_t flags = 0) noexcept {
      agt_name_t nameResult;
      agt_name_desc_t nameDesc {
          .async      = nullptr,
          .pName      = name.data(),
          .nameLength = name.length(),
          .flags      = AGT_NAME_RETAIN_OBJECT,
          .retainMask = AGT_SIGNAL_TYPE,
          .scope      = scope
      };
      switch(agt_reserve_name(ctx.raw_handle(), &nameDesc, &nameResult)) {
        case AGT_SUCCESS: {
          agt_signal_t newSignal;
          auto result = agt_new_signal(ctx.raw_handle(), &newSignal, nameResult, flags);
          if (result != AGT_SUCCESS) {
            agt_release_name(ctx.raw_handle(), nameResult);
            return std::nullopt;
          }

          return signal(newSignal);
        }
        case AGT_ERROR_NAME_ALREADY_IN_USE: {
          // If boundObject is NULL despite the name already being in use,
          // that means that the object with the requested name is not a signal,
          // which for our purposes here, is as good as total failure.
          if (nameResult.boundObject) {
            assert( nameResult.boundObject->type == AGT_SIGNAL_TYPE);
            return signal((agt_signal_t)nameResult.boundObject->object);
          }
        }
        case AGT_ERROR_BAD_UTF8_ENCODING:
        case AGT_ERROR_NAME_TOO_LONG:
        case AGT_ERROR_BAD_NAME:
          return std::nullopt;
        AGT_no_default;
      }

    }

    void raise() noexcept {
      agt_raise_signal(m_signal);
    }
  };

  template <typename RetType>
  class promise;

  class async_storage {

    friend class async;

    agt_inline_async_t inlineAsync;
  public:
    explicit async_storage(context ctx = context::acquire()) noexcept
        : inlineAsync(){
      inlineAsync.ctx = ctx.raw_handle();
    }
  };

  class async {
    agt_async_t m_async;

    template <typename>
    friend class promise;

    explicit async(agt_async_t handle) noexcept
        : m_async(handle) { }
  public:

    async(agt_async_flags_t flags = 0) noexcept
        : m_async(agt_new_async(agt_ctx(), flags)) { }
    async(async_storage& storage, agt_async_flags_t flags = 0) noexcept
        : m_async(agt_init_async(&storage.inlineAsync, flags)) { }
    explicit async(context ctx, agt_async_flags_t flags = 0) noexcept
        : m_async(agt_new_async(ctx.raw_handle(), flags)) { }

    async(const async& other) {
      assert( other.m_async != nullptr );
      m_async = agt_new_async(agt_ctx(), AGT_ASYNC_UNINITIALIZED);
      agt_copy_async(other.m_async, m_async);
    }
    async(async&& other) noexcept : m_async(std::exchange(other.m_async, nullptr)) { }

    async& operator=(const async& other) {
      if (this != &other) {
        if (!m_async)
          m_async = agt_new_async(agt_ctx(), AGT_ASYNC_UNINITIALIZED);
        agt_copy_async(other.m_async, m_async);
      }
      return *this;
    }
    async& operator=(async&& other) noexcept {
      if (m_async)
        agt_move_async(std::exchange(other.m_async, nullptr), m_async);
      else
        m_async = std::exchange(other.m_async, nullptr);
      return *this;
    }

    ~async() {
      if (m_async)
        agt_destroy_async(m_async);
    }

    [[nodiscard]] async copy_to(async_storage& storage) noexcept {
      assert( m_async != nullptr );
      auto handle = agt_init_async(&storage.inlineAsync, AGT_ASYNC_UNINITIALIZED | AGT_ASYNC_MAY_REPLACE);
      agt_copy_async(m_async, handle);
      return async(handle);
    }

    [[nodiscard]] async move_to(async_storage& storage) noexcept {
      assert( m_async != nullptr );
      auto handle = agt_init_async(&storage.inlineAsync, AGT_ASYNC_UNINITIALIZED | AGT_ASYNC_MAY_REPLACE);
      agt_move_async(std::exchange(m_async, nullptr), handle);
      return async(handle);
    }

    [[nodiscard]] agt_status_t status(agt_u64_t& value) noexcept {
      return agt_async_status(m_async, &value);
      /*if (m_async.status == AGT_NOT_READY)
        return agt_wait(&m_async, AGT_DO_NOT_WAIT);
      return m_async.status;*/
    }

    void wait() noexcept {
      agt_u64_t result;
      agt_wait(m_async, &result, AGT_WAIT);
      agt_wait(&m_async, AGT_WAIT);
    }

    void wait_for(agt_timeout_t timeout) noexcept {
      agt_wait(m_async, timeout);
    }

    void attach(const signal& s) noexcept {
      agt_attach_signal(s.m_signal, &m_async);
    }

    void clear() noexcept {
      agt_clear_async(&m_async);
    }
  };

  template <typename RetType>
  class promise {
    agt_async_t         m_async;
    mutable agt_status_t m_status;
    mutable RetType      m_result;

    static_assert(sizeof(RetType) <= sizeof(agt_u64_t));
  public:

    promise() noexcept requires std::default_initializable<RetType>
        : m_async(nullptr),
          m_status(AGT_SUCCESS),
          m_result()
    { }

    explicit promise(RetType value) noexcept
        : m_async(nullptr),
          m_status(AGT_SUCCESS),
          m_result(value)
    { }

    promise(const async& async) noexcept
        : m_async(async.m_async),
          m_status(AGT_NOT_READY),
          m_result()
    { }



    [[nodiscard]] RetType wait() const noexcept {
      agt_u64_t result;
      agt_wait(m_async, &result, AGT_WAIT);
    }


    [[nodiscard]] agt_status_t status() const noexcept {
      agt_u64_t result;
      if (m_status != AGT_NOT_READY) {
        m_status = agt_async_status(m_async, &result);
        if (m_status == AGT_SUCCESS)
          m_result = static_cast<RetType>(result);
      }
      return m_status;
    }

    explicit operator bool() const noexcept {
      return m_status == AGT_SUCCESS;
    }
  };


}

#endif//AGATE_EXAMPLE_CXX_WRAPPER_ASYNC_HPP
