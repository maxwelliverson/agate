//
// Created by maxwe on 2022-12-02.
//

#ifndef AGATE_SUPPORT_TAGGED_VALUE_HPP
#define AGATE_SUPPORT_TAGGED_VALUE_HPP

#include "atomic.hpp"

namespace agt {
  namespace impl {
    template <size_t N>
    class tagged_storage;

    template <>
    struct tagged_storage<1> {

      tagged_storage() = default;
      tagged_storage(const tagged_storage &) = default;
      template <typename T> requires(sizeof(T) == 1)
      inline explicit tagged_storage(T value, int vid = 0) noexcept
          : m_value(std::bit_cast<agt_u8_t>(value)),
            m_vid(vid)
      { }

      agt_u8_t m_value      = 0;
      agt_u8_t m_padding[3] = {};
      int      m_vid        = 0;
    };
    template <>
    struct tagged_storage<2> {

      tagged_storage() = default;
      tagged_storage(const tagged_storage &) = default;
      template <typename T> requires(sizeof(T) == 2)
      inline explicit tagged_storage(T value, int vid = 0) noexcept
          : m_value(std::bit_cast<agt_u16_t>(value)),
            m_vid(vid)
      { }

      agt_u16_t m_value   = 0;
      agt_u16_t m_padding = 0;
      int       m_vid     = 0;
    };
    template <>
    struct tagged_storage<4> {

      tagged_storage() = default;
      tagged_storage(const tagged_storage &) = default;
      template <typename T> requires(sizeof(T) == 4)
      inline explicit tagged_storage(T value, int vid = 0) noexcept
          : m_value(std::bit_cast<agt_u32_t>(value)),
            m_vid(vid)
      { }

      agt_u32_t m_value = 0;
      int       m_vid   = 0;
    };
    template <>
    struct tagged_storage<8> {

      tagged_storage() = default;
      tagged_storage(const tagged_storage &) = default;
      template <typename T> requires(sizeof(T) == 8)
      inline explicit tagged_storage(T value, int vid = 0) noexcept
          : m_value(std::bit_cast<agt_u64_t>(value)),
            m_vid(vid)
      { }

      agt_u64_t m_value   = 0;
      int       m_vid     = 0;
      agt_u32_t m_padding = 0;
    };

    template <size_t N>
    bool atomic_cas(tagged_storage<N>& self, tagged_storage<N>& compare, tagged_storage<N> newValue) noexcept {
      if constexpr (N == 8) {
        return atomicCompareExchange16(&self, &compare, &newValue);
      }
      else {
        return atomicCompareExchange(reinterpret_cast<agt_u64_t&>(self),
                                     reinterpret_cast<agt_u64_t&>(compare),
                                     *reinterpret_cast<agt_u64_t*>(&newValue));
      }
    }
    template <size_t N>
    bool atomic_cas_weak(tagged_storage<N>& self, tagged_storage<N>& compare, tagged_storage<N> newValue) noexcept {
      if constexpr (N == 8) {
        return atomicCompareExchangeWeak16(&self, &compare, &newValue);
      }
      else {
        return atomicCompareExchangeWeak(reinterpret_cast<agt_u64_t&>(self),
                                         reinterpret_cast<agt_u64_t&>(compare),
                                         *reinterpret_cast<agt_u64_t*>(&newValue));
      }
    }

    template <size_t N>
    [[nodiscard]] tagged_storage<N> atomic_load(const tagged_storage<N>& self) noexcept {
      if constexpr (N == 8) {
        tagged_storage<N> loaded{};
        atomic_cas(const_cast<tagged_storage<N>&>(self), loaded, loaded);
        return loaded;
      }
      else {
        return std::bit_cast<tagged_storage<N>>(atomicLoad(reinterpret_cast<const agt_u64_t&>(self)));
      }

    }
    template <size_t N>
    [[nodiscard]] tagged_storage<N> atomic_relaxed_load(const tagged_storage<N>& self) noexcept {
      if constexpr (N == 8) {
        return atomic_load(self);
      }
      else {
        return std::bit_cast<tagged_storage<N>>(atomicRelaxedLoad(reinterpret_cast<const agt_u64_t&>(self)));
      }
    }

    template <size_t N>
    [[nodiscard]] tagged_storage<N> atomic_exchange(tagged_storage<N>& self, tagged_storage<N> tagged) noexcept {
      if constexpr (N == 8) {
        tagged_storage<N> compareValue = atomic_relaxed_load(self);
        while (!atomic_cas_weak(self, compareValue, tagged));
        return compareValue;
      }
      else {
        return std::bit_cast<tagged_storage<N>>(
            atomicExchange(reinterpret_cast<agt_u64_t&>(self),
                           *reinterpret_cast<const agt_u64_t*>(&tagged)));
      }
    }


    template <size_t N>
    void atomic_store(tagged_storage<N>& self, tagged_storage<N> tagged) noexcept {
      if constexpr (N == 8) {
        tagged_storage<N> compareValue = atomic_relaxed_load(self);
        while (!atomic_cas_weak(self, compareValue, tagged));
      }
      else {
        atomicStore(reinterpret_cast<agt_u64_t&>(self), *reinterpret_cast<const agt_u64_t*>(&tagged));
      }
    }
    template <size_t N>
    void atomic_relaxed_store(tagged_storage<N>& self, tagged_storage<N> tagged) noexcept {
      if constexpr (N == 8) {
        atomic_store(self, tagged);
      }
      else {
        atomicRelaxedStore(reinterpret_cast<agt_u64_t&>(self), *reinterpret_cast<const agt_u64_t*>(&tagged));
      }
    }
  }

  template <typename T, typename Tag = int>
  class tagged_atomic;

  template <typename T, typename Tag = int>
  class tagged_value {
    friend tagged_atomic<T, Tag>;

    using storage_type = impl::tagged_storage<sizeof(T)>;

    tagged_value(storage_type storage) noexcept : m_storage(storage) { }

  public:

    using value_type = T;
    using tag_type   = Tag;

    tagged_value() = default;
    tagged_value(const tagged_value &) = default;
    tagged_value(value_type value) noexcept : m_storage(value) { }
    tagged_value(value_type value, tag_type tag) noexcept : m_storage(value, std::bit_cast<int>(tag)) { }

    explicit tagged_value(const tagged_atomic<T, Tag>&) noexcept;

    [[nodiscard]]       value_type&  value()       &  noexcept {
      if constexpr (std::same_as<decltype(m_storage.m_value), value_type>)
        return m_storage.m_value;
      else
        return reinterpret_cast<value_type&>(m_storage.m_value);
    }
    [[nodiscard]] const value_type&  value() const &  noexcept {
      if constexpr (std::same_as<decltype(m_storage.m_value), value_type>)
        return m_storage.m_value;
      else
        return reinterpret_cast<const value_type&>(m_storage.m_value);
    }
    [[nodiscard]]       value_type&& value()       && noexcept {
      if constexpr (std::same_as<decltype(m_storage.m_value), value_type>)
        return std::move(m_storage.m_value);
      else
        return reinterpret_cast<value_type&&>(std::move(m_storage.m_value));
    }
    [[nodiscard]] const value_type&& value() const && noexcept {
      if constexpr (std::same_as<decltype(m_storage.m_value), value_type>)
        return std::move(m_storage.m_value);
      else
        return reinterpret_cast<const value_type&&>(std::move(m_storage.m_value));
    }


    [[nodiscard]]       tag_type&    tag()       &  noexcept {
      if constexpr (!std::same_as<tag_type, int>)
        return reinterpret_cast<tag_type&>(m_storage.m_vid);
      else
        return m_storage.m_vid;
    }
    [[nodiscard]] const tag_type&    tag() const &  noexcept {
      if constexpr (!std::same_as<tag_type, int>)
        return reinterpret_cast<const tag_type&>(m_storage.m_vid);
      else
        return m_storage.m_vid;
    }
    [[nodiscard]]       tag_type&&   tag()       && noexcept {
      if constexpr (!std::same_as<tag_type, int>)
        return reinterpret_cast<tag_type&&>(std::move(m_storage.m_vid));
      else
        return std::move(m_storage.m_vid);
    }
    [[nodiscard]] const tag_type&&   tag() const && noexcept {
      if constexpr (!std::same_as<tag_type, int>)
        return reinterpret_cast<const tag_type&&>(std::move(m_storage.m_vid));
      else
        return std::move(m_storage.m_vid);
    }

    friend bool operator==(tagged_value a, tagged_value b) noexcept {
      return a.m_storage.m_value == b.m_storage.m_value &&
             a.m_storage.m_vid == b.m_storage.m_vid;
    }

  private:
    storage_type m_storage = {};
  };

  template <typename T, typename Tag>
  class tagged_atomic {
    friend tagged_value<T, Tag>;
  public:

    using value_type = T;
    using tag_type   = Tag;
    using tagged_type = tagged_value<T, Tag>;

    tagged_atomic() = default;
    tagged_atomic(const tagged_atomic &) = delete;
    tagged_atomic(tagged_atomic &&) noexcept = default;

    explicit tagged_atomic(tagged_type tagged) noexcept : m_storage(tagged) { }

    tagged_type load() const noexcept {
      return tagged_type(impl::atomic_load(m_storage));
    }
    tagged_type relaxedLoad() const noexcept {
      return tagged_type(impl::atomic_relaxed_load(m_storage));
    }

    void        store(tagged_type tagged) noexcept {
      impl::atomic_store(m_storage, tagged.m_storage);
    }
    void        relaxedStore(tagged_type tagged) noexcept {
      impl::atomic_relaxed_store(m_storage, tagged.m_storage);
    }

    tagged_type exchange(tagged_type newValue) noexcept {
      return tagged_type(impl::atomic_exchange(m_storage, newValue.m_storage));
    }

    // May be called with a value_type second argument,
    // given that tagged_type is implicitly convertible from value_type.
    bool        compareAndSwap(tagged_type& expectedOrCaptured, tagged_type newValue) noexcept {
      return impl::atomic_cas(m_storage, expectedOrCaptured.m_storage, newValue.m_storage);
    }
    bool        compareAndSwapWeak(tagged_type& expectedOrCaptured, tagged_type newValue) noexcept {
      return impl::atomic_cas_weak(m_storage, expectedOrCaptured.m_storage, newValue.m_storage);
    }


    void        notifyOne() const noexcept {}
    void        notifyAll() const noexcept {}

    void        wait() const noexcept {}

    void        tagNotifyOne() const noexcept {
      atomicNotifyOne(const_cast<void*>(static_cast<const void*>(std::addressof(m_storage.m_vid))));
    }
    void        tagNotifyAll() const noexcept {
      atomicNotifyAll(const_cast<void*>(static_cast<const void*>(std::addressof(m_storage.m_vid))));
    }

    void        tagWait(tag_type val = {}) const noexcept {

    }
    bool        tagWaitFor(tag_type val = {}) const noexcept {

    }
    bool        tagWaitUntil(tag_type val = {}) const noexcept {

    }

  private:
    using storage_type = impl::tagged_storage<sizeof(T)>;

    storage_type m_storage;
  };


  template <typename T, typename Tag>
  inline tagged_value<T, Tag>::tagged_value(const tagged_atomic<T, Tag>& s) noexcept
      : m_storage(atomic_relaxed_load(s.m_storage)) { }




  // variable that is only ever accessed by fully atomic update operations
  // In particular, guarded_atomic is NOT vulnerable to the ABA problem;
  // On a successful update, it is guaranteed that the stored value was not modified in between
  // the initial read and subsequent successful write.
  //
  // There are 3 types of updates:
  //   required:  will continually attempt to update until successful
  //   try:       will continually attempt to update either until successful, or the read value satisfies some predicate
  //   try until: will continually attempt to update either until successful, or until a specified time limit passes
  //
  // Required Updates:
  //
  //   loop:
  //     read
  //     if write is successful, return
  //     else, continue loop
  //
  // Try Updates:
  //
  //   loop:
  //     read
  //     if predicate is true, return failure
  //     else if write is successful, return success
  //     else, continue loop
  //
  //
  // Try Until Updates:
  //
  //   loop:
  //     read
  //     if timeout has passed, return failure
  //     else if write is successful, return success
  //     else, continue loop
  //
  template <typename T>
  class guarded_atomic {

    inline void notifyUpdate() const noexcept {
      m_var.tagNotifyOne();
    }

  public:



    using atomic_type = tagged_atomic<T>;
    using tagged_type = tagged_value<T>;
    using value_type  = T;

    struct result_type {
      value_type oldValue;
      value_type newValue;
    };

    guarded_atomic() = default;
    explicit guarded_atomic(tagged_type value) noexcept : m_var(value) { }



    /**
     * Updates the stored value.
     *
     * \details Performs the following pseudocode atomically:
     * \code
     *      oldValue = storedValue
     *      storedValue = newValue
     *      return oldValue
     * \endcode
     *
     * \param [in] newValue The value to be stored
     *
     * \note This causes a "modification" even in the case where newValue == storedValue, and will
     *       cause any other concurrent attempted updates to fail, even though storedValue is not
     *       modified.
     * */
    value_type  update(value_type newValue) noexcept {
      tagged_type compare = m_var.relaxedLoad();
      tagged_type writeValue{ newValue, compare.tag() + 1 };

      while (!m_var.compareAndSwapWeak(compare, writeValue)) {
        writeValue.tag() = compare.tag();
      }

      notifyUpdate();

      return compare.value();
    }

    /**
     * Updates the stored value
     *
     * \details Performs the following pseudocode atomically:
     * \code
     *      oldValue = storedValue
     *      storedValue = newValue
     *      return oldValue
     * \endcode
     *
     * \param [in] updateFn The functor invoked on the stored value that returns the desired updated value.
     *
     * \note The update functor may have side effects, which are guaranteed to be visible as soon as the
     *       stored value has been successfully updated, but keep in mind that the functor may be called
     *       several times with different values that result in unsuccessful updates. These calls are made
     *       sequentially, so upon return, the most recent call will have been the call made during the
     *       successful update.
     *       In addition, try to keep the work done by the functor to a minimum; given that it is called
     *       in between the initial read and the attempted write, the longer the functor takes to run,
     *       which for a highly contested variable means the more likely it is that another thread will
     *       have updated the stored value in the meantime, causing a failed attempt. This is true for any
     *       of the update methods that take an update functor, so keep this in mind.
     *
     * */
    template <std::invocable<value_type> UpdateFn> requires std::is_invocable_r_v<value_type, UpdateFn, value_type>
    result_type update(UpdateFn&& updateFn) noexcept {
      tagged_type compare = m_var.relaxedLoad();
      tagged_type writeValue;

      do {
        writeValue = tagged_type{ std::invoke(updateFn, compare.value()), compare.tag() + 1 };
      } while(!m_var.compareAndSwapWeak(compare, writeValue));

      notifyUpdate();

      return { compare.value(), writeValue.value() };
    }

    value_type waitAndUpdate(value_type notEqual, value_type newValue) noexcept {
      tagged_type stored  = m_var.relaxedLoad();
      tagged_type writeValue{newValue};

      do {
        if (stored.value() == notEqual)
          _waitForUpdate(stored);
        writeValue.tag() = stored.tag() + 1;
      } while(!m_var.compareAndSwap(stored, writeValue));

      return stored.value();
    }

    template <std::predicate<value_type> Pred>
    value_type waitAndUpdate(Pred&& predFn, value_type newValue) noexcept {
      tagged_type stored  = m_var.relaxedLoad();
      tagged_type writeValue{newValue};

      do {
        if (!std::invoke(predFn, stored.value())) {
          _waitForUpdate(stored);
          continue;
        }
        writeValue.tag() = stored.tag() + 1;
        if (m_var.compareAndSwap(stored, writeValue))
          break;
      } while(true);

      return stored.value();
    }

    template <std::invocable<value_type> UpdateFn> requires std::is_invocable_r_v<value_type, UpdateFn, value_type>
    result_type waitAndUpdate(value_type notEqual, UpdateFn&& updateFn) noexcept {
      tagged_type stored  = m_var.relaxedLoad();
      tagged_type writeValue;

      do {
        if (stored.value() == notEqual)
          _waitForUpdate(stored);
        writeValue = { std::invoke(updateFn, stored.value()), stored.tag() + 1 };
      } while(!m_var.compareAndSwap(stored, writeValue));

      return { stored.value(), writeValue.value() };
    }

    template <std::predicate<value_type> Pred, std::invocable<value_type> UpdateFn> requires std::is_invocable_r_v<value_type, UpdateFn, value_type>
    result_type waitAndUpdate(Pred&& predFn, UpdateFn&& updateFn) noexcept {
      tagged_type stored  = m_var.relaxedLoad();
      tagged_type writeValue;

      do {
        if (!std::invoke(predFn, stored.value())) {
          _waitForUpdate(stored);
          continue;
        }
        writeValue = { std::invoke(updateFn, stored.value()), stored.tag() + 1 };
        if (m_var.compareAndSwap(stored, writeValue))
          break;
      } while(true);

      return { stored.value(), writeValue.value() };
    }





    /**
     * Updates the stored value if it is not equal to oldValue
     *
     * \details Performs the following pseudocode atomically: \par
     * \code
     *      if storedValue == oldValue
     *          return false
     *      else
     *          oldValue = storedValue
     *          storedValue = newValue
     *          return true
     * \endcode
     *
     * \param [in,out] oldValue Update will only occur if stored value differs from this value; is set to the previously stored value in the case of a successful update
     * \param [in] newValue The value that will be stored in the case of a successful update
     *
     * \post oldValue is equal to the previously stored value, regardless of whether or not newValue was written.
     * */
    bool        tryUpdate(value_type& oldValue, value_type newValue) noexcept {
      tagged_type compare = m_var.relaxedLoad();
      tagged_type writeValue{ newValue };

      do {
        if (compare.value() == oldValue)
          return false;
        writeValue.tag() = compare.tag() + 1;
      } while(!m_var.compareAndSwapWeak(compare, writeValue));

      notifyUpdate();

      oldValue = compare.value();
      return true;
    }

    /**
     * Updates the stored value if it is not equal to oldValue
     *
     * \details Performs the following pseudocode atomically: \par
     * \code
     *      if storedValue == oldValue
     *          return nothing
     *      else
     *          newValue = updateFn(storedValue)
     *          oldValue = storedValue
     *          storedValue = newValue
     *          return newValue
     * \endcode
     *
     * \param [in,out] oldValue Update will only occur if stored value differs from this value; is set to the previously stored value in the case of a successful update
     * \param [in] newValue The value that will be stored in the case of a successful update
     *
     * \post oldValue is equal to the previously stored value, regardless of whether or not newValue was written.
     * \sa   update, tryUpdate
     * */
    template <std::invocable<value_type> UpdateFn> requires std::is_invocable_r_v<value_type, UpdateFn, value_type>
    std::optional<value_type> tryUpdate(value_type& oldValue, UpdateFn&& updateFn) noexcept {
      tagged_type compare = m_var.relaxedLoad();
      tagged_type writeValue;

      do {
        if (compare.value() == oldValue)
          return std::nullopt;
        writeValue = tagged_type{ std::invoke(updateFn, compare.value()), compare.tag() + 1 };
      } while(!m_var.compareAndSwapWeak(compare, writeValue));

      notifyUpdate();

      oldValue = compare.value();

      return writeValue.value();
    }


    template <std::predicate<value_type> Pred>
    bool        tryUpdate(value_type& oldValue, value_type newValue, Pred&& predFn) noexcept {
      tagged_type compare = m_var.relaxedLoad();
      tagged_type writeValue{ newValue };

      do {
        if (!std::invoke(predFn, compare.value()))
          return false;
        writeValue.tag() = compare.tag() + 1;
      } while(!m_var.compareAndSwapWeak(compare, writeValue));

      notifyUpdate();

      oldValue = compare.value();
      return true;
    }

    template <std::predicate<value_type> Pred, std::invocable<value_type> UpdateFn> requires std::is_invocable_r_v<value_type, UpdateFn, value_type>
    std::optional<value_type> tryUpdate(value_type& oldValue, UpdateFn&& updateFn, Pred&& predFn) noexcept {
      tagged_type compare = m_var.relaxedLoad();
      tagged_type writeValue;

      do {
        if (!std::invoke(predFn, compare.value()))
          return std::nullopt;
        writeValue = tagged_type{ std::invoke(updateFn, compare.value()), compare.tag() + 1 };
      } while(!m_var.compareAndSwapWeak(compare, writeValue));

      notifyUpdate();

      oldValue = compare.value();

      return writeValue.value();
    }



    bool tryUpdateFor(value_type& oldValue, value_type newValue, agt_timeout_t timeout) noexcept {
      return this->tryUpdateUntil(oldValue, newValue, deadline::fromTimeout(timeout));
    }


    template <std::invocable<value_type> UpdateFn> requires std::is_invocable_r_v<value_type, UpdateFn, value_type>
    std::optional<value_type> tryUpdateFor(value_type& oldValue, UpdateFn&& updateFn, agt_timeout_t timeout) noexcept {
      return this->tryUpdateUntil(oldValue, std::forward<UpdateFn>(updateFn), deadline::fromTimeout(timeout));
    }


    template <std::predicate<value_type> Pred>
    bool tryUpdateFor(value_type& oldValue, value_type newValue, agt_timeout_t timeout, Pred&& predFn) noexcept {
      return this->tryUpdateUntil(oldValue, newValue, deadline::fromTimeout(timeout), std::forward<Pred>(predFn));
    }


    template <std::predicate<value_type> Pred, std::invocable<value_type> UpdateFn> requires std::is_invocable_r_v<value_type, UpdateFn, value_type>
    std::optional<value_type> tryUpdateFor(value_type& oldValue, UpdateFn&& updateFn, agt_timeout_t timeout, Pred&& predFn) noexcept {
      return this->tryUpdateUntil(oldValue, std::forward<UpdateFn>(updateFn), deadline::fromTimeout(timeout), std::forward<Pred>(predFn));
    }




    bool tryUpdateUntil(value_type& oldValue, value_type newValue, deadline dl) noexcept {
      tagged_type compare = m_var.relaxedLoad();
      tagged_type writeValue{newValue};

      do {
        if (compare.value() == oldValue) {
          if (_waitForUpdateUntil(compare, dl))
            continue;
          // else deadline has passed;
          return false;
        }
        if (dl.hasPassed() )
          return false;
        writeValue.tag() = compare.tag() + 1;
      } while(!m_var.compareAndSwap(compare, writeValue)); // Don't go with weak version; loop may involve deep waiting, so avoiding spurious failure is more important in this case.

      notifyUpdate();

      oldValue = compare.value();
      return true;
    }

    template <std::invocable<value_type> UpdateFn> requires std::is_invocable_r_v<value_type, UpdateFn, value_type>
    std::optional<value_type> tryUpdateUntil(value_type& oldValue, UpdateFn&& updateFn, deadline dl) noexcept {
      tagged_type compare = m_var.relaxedLoad();
      tagged_type writeValue;

      do {
        if (compare.value() == oldValue) {
          if (_waitForUpdateUntil(compare, dl))
            continue;
          // else deadline has passed;
          return std::nullopt;
        }
        if (dl.hasPassed() )
          return std::nullopt;
        writeValue = { std::invoke(updateFn, compare.value()), compare.tag() + 1 };
      } while(!m_var.compareAndSwap(compare, writeValue)); // Don't go with weak version; loop may involve deep waiting, so avoiding spurious failure is more important in this case.

      notifyUpdate();

      oldValue = compare.value();
      return writeValue.value();
    }

    template <std::predicate<value_type> Pred>
    bool tryUpdateUntil(value_type& oldValue, value_type newValue, deadline dl, Pred&& predFn) noexcept {
      tagged_type compare = m_var.relaxedLoad();
      tagged_type writeValue{newValue};

      do {
        if (!std::invoke(predFn, compare.value())) {
          if (_waitForUpdateUntil(compare, dl))
            continue;
          // else deadline has passed;
          return false;
        }
        if (dl.hasPassed() )
          return false;
        writeValue.tag() = compare.tag() + 1;
      } while(!m_var.compareAndSwap(compare, writeValue)); // Don't go with weak version; loop may involve deep waiting, so avoiding spurious failure is more important in this case.

      notifyUpdate();

      oldValue = compare.value();
      return true;
    }

    template <std::predicate<value_type> Pred, std::invocable<value_type> UpdateFn> requires std::is_invocable_r_v<value_type, UpdateFn, value_type>
    std::optional<value_type> tryUpdateUntil(value_type& oldValue, UpdateFn&& updateFn, deadline dl, Pred&& predFn) noexcept {
      tagged_type compare = m_var.relaxedLoad();
      tagged_type writeValue;

      do {
        if (!std::invoke(predFn, compare.value())) {
          if (_waitForUpdateUntil(compare, dl))
            continue;
          // else deadline has passed;
          return std::nullopt;
        }
        if (dl.hasPassed() )
          return std::nullopt;
        writeValue = { std::invoke(updateFn, compare.value()), compare.tag() + 1 };
      } while(!m_var.compareAndSwap(compare, writeValue)); // Don't go with weak version; loop may involve deep waiting, so avoiding spurious failure is more important in this case.

      notifyUpdate();

      oldValue = compare.value();
      return writeValue.value();
    }


    value_type wait(value_type notEqual) const noexcept {
      tagged_type stored = m_var.load();

      do {
        if (stored.value() != notEqual)
          return stored.value();
        _waitForUpdate(stored);
      } while(true);
    }
    template <std::predicate<value_type> Pred>
    value_type wait(Pred&& predFn)       const noexcept {
      tagged_type stored = m_var.load();

      do {
        if (std::invoke(predFn, stored.value()))
          return stored.value();
        _waitForUpdate(stored);
      } while(true);
    }



  private:

    void _waitForUpdate(tagged_type& compare) const noexcept {}

    bool _waitForUpdateUntil(tagged_type& compare, deadline dl) const noexcept {

    }

    atomic_type m_var;
  };
}

#endif//AGATE_SUPPORT_TAGGED_VALUE_HPP
