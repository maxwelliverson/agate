//
// Created by maxwe on 2021-09-21.
//

#ifndef JEMSYS_INTERNAL_ITERATOR_HPP
#define JEMSYS_INTERNAL_ITERATOR_HPP

#include <utility>
#include <ranges>
#include <iterator>


namespace agt{
  template <typename BeginIter, typename EndIter = BeginIter>
  class range_view{
  public:

    using iterator = BeginIter;
    using sentinel = EndIter;

    range_view(const range_view&) = default;
    range_view(range_view&&) noexcept = default;

    range_view(iterator b, sentinel e) noexcept : begin_(std::move(b)), end_(std::move(e)){}
    template <typename Rng> requires(std::ranges::range<Rng> && !std::same_as<std::remove_cvref_t<Rng>, range_view>)
      range_view(Rng&& rng) noexcept
        : begin_(std::ranges::begin(rng)),
          end_(std::ranges::end(rng)){}

    AGT_nodiscard iterator begin() const noexcept {
      return begin_;
    }
    AGT_nodiscard sentinel end()   const noexcept {
      return end_;
    }

  private:
    BeginIter begin_;
    EndIter   end_;
  };

  template <typename T, typename S>
  range_view<T, S> make_range(T x, S y) {
    return range_view<T, S>(std::move(x), std::move(y));
  }
  template <typename T, typename S>
  range_view<T, S> make_range(std::pair<T, S> p) {
    return range_view<T, S>(std::move(p.first), std::move(p.second));
  }



  template <typename DerivedT,
            typename IteratorCategoryT,
            typename T,
            typename DifferenceTypeT = std::ptrdiff_t,
            typename PointerT = T *,
            typename ReferenceT = T &>
  class iterator_facade_base : public std::iterator<IteratorCategoryT, T, DifferenceTypeT, PointerT, ReferenceT> {
  protected:

    inline constexpr static bool IsRandomAccess  = std::derived_from<IteratorCategoryT, std::random_access_iterator_tag>;
    inline constexpr static bool IsBidirectional = std::derived_from<IteratorCategoryT, std::bidirectional_iterator_tag>;

    /// A proxy object for computing a reference via indirecting a copy of an
    /// iterator. This is used in APIs which need to produce a reference via
    /// indirection but for which the iterator object might be a temporary. The
    /// proxy preserves the iterator internally and exposes the indirected
    /// reference via a conversion operator.
    class reference_proxy {
      friend iterator_facade_base;

      DerivedT I;

      reference_proxy(DerivedT I) : I(std::move(I)) {}

    public:
      operator ReferenceT() const { return *I; }
    };

    using derived_type = DerivedT;

  public:

    using iterator_category = IteratorCategoryT;
    using value_type        = T;
    using difference_type   = DifferenceTypeT;
    using pointer           = PointerT;
    using reference         = ReferenceT;



    derived_type operator+(difference_type n) const requires(IsRandomAccess)  {
      static_assert(std::is_base_of<iterator_facade_base, derived_type>::value,
                    "Must pass the derived type to this template!");
      derived_type tmp = *static_cast<const derived_type *>(this);
      tmp += n;
      return tmp;
    }
    friend derived_type operator+(difference_type n, const derived_type &i) requires(IsRandomAccess) {
      return i + n;
    }
    derived_type operator-(difference_type n) const requires(IsRandomAccess) {
      derived_type tmp = *static_cast<const derived_type *>(this);
      tmp -= n;
      return tmp;
    }

    derived_type& operator++() {
      return static_cast<derived_type *>(this)->operator+=(1);
    }
    derived_type operator++(int) {
      derived_type tmp = *static_cast<derived_type *>(this);
      ++*static_cast<derived_type *>(this);
      return tmp;
    }
    derived_type &operator--()   requires(IsBidirectional) {
      static_assert(
        IsBidirectional,
        "The decrement operator is only defined for bidirectional iterators.");
      return static_cast<derived_type *>(this)->operator-=(1);
    }
    derived_type operator--(int) requires(IsBidirectional) {
      static_assert(
        IsBidirectional,
        "The decrement operator is only defined for bidirectional iterators.");
      derived_type tmp = *static_cast<derived_type *>(this);
      --*static_cast<derived_type *>(this);
      return tmp;
    }


    PointerT operator->() { return std::addressof(static_cast<derived_type *>(this)->operator*()); }
    PointerT operator->() const { return std::addressof(static_cast<const derived_type *>(this)->operator*()); }
    reference_proxy operator[](difference_type n) requires(IsRandomAccess) { return reference_proxy(static_cast<derived_type *>(this)->operator+(n)); }
    reference_proxy operator[](difference_type n) const requires(IsRandomAccess) { return reference_proxy(static_cast<const derived_type *>(this)->operator+(n)); }
  };

  /// CRTP base class for adapting an iterator to a different type.
  ///
  /// This class can be used through CRTP to adapt one iterator into another.
  /// Typically this is done through providing in the derived class a custom \c
  /// operator* implementation. Other methods can be overridden as well.
  template <
    typename DerivedT,
    typename WrappedIteratorT,
    typename IteratorCategoryT = typename std::iterator_traits<WrappedIteratorT>::iterator_category,
    typename T                 = std::iter_value_t<WrappedIteratorT>,
    typename DifferenceTypeT   = std::iter_difference_t<WrappedIteratorT>,
    typename PointerT          = std::conditional_t<std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<std::iter_value_t<WrappedIteratorT>>>, typename std::iterator_traits<WrappedIteratorT>::pointer, T*>,
    typename ReferenceT        = std::conditional_t<std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<std::iter_value_t<WrappedIteratorT>>>, std::iter_reference_t<WrappedIteratorT>, T&>>
  class iterator_adaptor_base : public iterator_facade_base<DerivedT, IteratorCategoryT, T, DifferenceTypeT, PointerT, ReferenceT> {
    using BaseT = typename iterator_adaptor_base::iterator_facade_base;

  protected:
    WrappedIteratorT I;

    iterator_adaptor_base() = default;

    explicit iterator_adaptor_base(WrappedIteratorT u) : I(std::move(u)) {
      static_assert(std::is_base_of<iterator_adaptor_base, DerivedT>::value,
                    "Must pass the derived type to this template!");
    }

    const WrappedIteratorT &wrapped() const { return I; }

  public:
    using difference_type = DifferenceTypeT;

    DerivedT &operator+=(difference_type n) {
      static_assert(
        BaseT::IsRandomAccess,
        "The '+=' operator is only defined for random access iterators.");
      I += n;
      return *static_cast<DerivedT *>(this);
    }
    DerivedT &operator-=(difference_type n) {
      static_assert(
        BaseT::IsRandomAccess,
        "The '-=' operator is only defined for random access iterators.");
      I -= n;
      return *static_cast<DerivedT *>(this);
    }
    using BaseT::operator-;
    difference_type operator-(const DerivedT &RHS) const {
      static_assert(
        BaseT::IsRandomAccess,
        "The '-' operator is only defined for random access iterators.");
      return I - RHS.I;
    }

    // We have to explicitly provide ++ and -- rather than letting the facade
    // forward to += because WrappedIteratorT might not support +=.
    using BaseT::operator++;
    DerivedT &operator++() {
      ++I;
      return *static_cast<DerivedT *>(this);
    }
    using BaseT::operator--;
    DerivedT &operator--() {
      static_assert(
        BaseT::IsBidirectional,
        "The decrement operator is only defined for bidirectional iterators.");
      --I;
      return *static_cast<DerivedT *>(this);
    }

    bool operator==(const DerivedT &RHS) const { return I == RHS.I; }
    bool operator<(const DerivedT &RHS) const {
      static_assert(
        BaseT::IsRandomAccess,
        "Relational operators are only defined for random access iterators.");
      return I < RHS.I;
    }

    ReferenceT operator*() const { return *I; }
  };

}

#endif//JEMSYS_INTERNAL_ITERATOR_HPP
