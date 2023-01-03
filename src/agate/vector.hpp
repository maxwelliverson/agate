//
// Created by maxwe on 2021-09-23.
//

#ifndef JEMSYS_INTERNAL_VECTOR_HPP
#define JEMSYS_INTERNAL_VECTOR_HPP

#include "agate.h"


#include "allocator.hpp"

#include <initializer_list>


#if AGT_compiler_msvc
#define AGT_empty_bases __declspec(empty_bases)
#else
#define AGT_empty_bases
#endif


namespace agt {
  template <typename T, size_t N>
  class vector;
  

  namespace impl{

    template <typename T, typename U>
    concept same_as = std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;
    template <typename T, typename U>
    concept not_same_as = !same_as<T, U>;

    template <typename T>
    concept trivial_type = std::is_trivial_v<std::remove_cv_t<T>>;


    template <typename SizeType>
    struct vector_base {

      using size_type = SizeType;

      static agt_u64_t get_new_capacity(agt_u64_t minimum_size, agt_u64_t old_capacity) noexcept;

    protected:
      void*     begin_;
      size_type size_ = 0;
      size_type capacity_;

      /// The maximum value of the Size_T used.
      static constexpr size_type max_size() noexcept {
        return std::numeric_limits<size_type>::max();
      }

      vector_base(void* first_element, agt_u64_t total_capacity) noexcept
          : begin_(first_element), capacity_(total_capacity) {}


      AGT_noinline void* malloc_for_grow(agt_u64_t minimum_size, agt_u64_t type_size, agt_u64_t& new_capacity, agt_u64_t alignment);
      AGT_noinline void  grow_pod(void *first_element, agt_u64_t minimum_capacity, agt_u64_t type_size, agt_u64_t alignment);




    public:
      vector_base() = delete;

      AGT_nodiscard size_type size() const noexcept { return size_; }
      AGT_nodiscard size_type capacity() const noexcept { return capacity_; }

      AGT_nodiscard bool empty() const noexcept { return !size_; }

      void set_size(agt_u64_t new_size) noexcept {
        AGT_assert(new_size <= capacity());
        size_ = new_size;
      }
    };

    template <typename T>
    using small_array_size_type = std::conditional_t<sizeof(T) <= 4, agt_u64_t, agt_u32_t>;



    template <typename T, typename ...U>
    struct buffer_aligner{
      T t;
      buffer_aligner<U...> tail;
      buffer_aligner() = delete;
    };
    template <typename T>
    struct buffer_aligner<T>{
      T t;
      buffer_aligner() = delete;
    };
    template <typename T, typename ...U>
    union buffer_sizer{
      char buffer[sizeof(T)];
      buffer_sizer<U...> tail;
    };
    template <typename T>
    union buffer_sizer<T>{
      char buffer[sizeof(T)];
    };

    template <typename T, typename ...U>
    struct aligned_union{
      alignas(buffer_aligner<T, U...>) char buffer[sizeof(buffer_sizer<T, U...>)];
    };

    template <typename T, typename = void>
    struct small_array_alignment_and_size{
      aligned_union<vector_base<small_array_size_type<T>>> base;
      aligned_union<T> first_element;
    };

    template <typename T, typename = void>
    class small_array_template_common : public vector_base<small_array_size_type<T>>{
      using base = vector_base<small_array_size_type<T>>;

      AGT_nodiscard void* get_first_element() const noexcept {
        return const_cast<void*>(reinterpret_cast<const void*>(
          reinterpret_cast<const std::byte*>(this) +
          offsetof(small_array_alignment_and_size<T>, first_element)));
      }

    protected:
      small_array_template_common(agt_u64_t size) noexcept : base(get_first_element(), size){ }

      void grow_pod(agt_u64_t minimum_capacity, agt_u64_t size, agt_u64_t align) noexcept {
        base::grow_pod(get_first_element(), minimum_capacity, size, align);
      }

      AGT_nodiscard bool is_small() const noexcept {
        return this->begin_ == get_first_element();
      }

      void reset_to_small() noexcept {
        this->begin_ = get_first_element();
        this->size_ = 0;
        // this->capacity_ = 0;
      }

      /// Return true if V is an internal reference to the given range.
      bool is_reference_to_range(const void *V, const void *First, const void *Last) const noexcept {
        // Use std::less to avoid UB.
        std::less<> LessThan;
        return !LessThan(V, First) && LessThan(V, Last);
      }

      /// Return true if V is an internal reference to this vector.
      bool is_reference_to_storage(const void *V) const noexcept {
        return is_reference_to_range(V, begin(), end());
      }

      /// Return true if First and Last form a valid (possibly empty) range in this
      /// vector's storage.
      bool is_range_in_storage(const void *First, const void *Last) const {
        // Use std::less to avoid UB.
        std::less<> LessThan;
        return !LessThan(First, begin()) && !LessThan(Last, First) &&
               !LessThan(end(), Last);
      }

      /// Return true unless element will be invalidated by resizing the vector to
      /// NewSize.
      bool is_safe_to_reference_after_resize(const void *element, agt_u64_t NewSize) noexcept {
        // Past the end.
        if (!is_reference_to_storage(element)) [[likely]]
          return true;

        // Return false if element will be destroyed by shrinking.
        if (NewSize <= this->size())
          return element < this->begin() + NewSize;

        // Return false if we need to grow.
        return NewSize <= this->capacity();
      }

      /// Check whether element will be invalidated by resizing the vector to NewSize.
      void assert_safe_to_reference_after_resize(const void *element, agt_u64_t NewSize) noexcept {
        AGT_assert(is_safe_to_reference_after_resize(element, NewSize) &&
                  "Attempting to reference an element of the vector in an operation "
                  "that invalidates it");
      }

      /// Check whether element will be invalidated by increasing the size of the
      /// vector by N.
      void assert_safe_to_add(const void *element, agt_u64_t N = 1) {
        this->assert_safe_to_reference_after_resize(element, this->size() + N);
      }

      /// Check whether any part of the range will be invalidated by clearing.
      void assert_safe_to_reference_after_clear(const T *From, const T *To) noexcept {
        if (From == To)
          return;
        this->assert_safe_to_reference_after_resize(From, 0);
        this->assert_safe_to_reference_after_resize(To - 1, 0);
      }
      template <typename ItTy> requires not_same_as<ItTy, T*>
      void assert_safe_to_reference_after_clear(ItTy, ItTy) noexcept {}

      /// Check whether any part of the range will be invalidated by growing.
      void assert_safe_to_add_range(const T *From, const T *To) noexcept {
        if (From == To)
          return;
        this->assert_safe_to_add(From, To - From);
        this->assert_safe_to_add(To - 1, To - From);
      }
      template <typename ItTy> requires not_same_as<ItTy, T*>
      void assert_safe_to_add_range(ItTy, ItTy) noexcept { }

      /// Reserve enough space to add one element, and return the updated element
      /// pointer in case it was a reference to the storage.
      template <class U>
      static const T *reserve_for_param_and_get_address_impl(U *This, const T & element, agt_u64_t N) {
        agt_u64_t NewSize = This->size() + N;
        if (NewSize <= This->capacity()) [[likely]]
          return std::addressof(element);

        bool references_storage = false;
        agt_u64_t Index = -1;
        if constexpr ( !U::takes_params_by_value ) {
          if (This->is_reference_to_storage(&element)) [[unlikely]] {
              references_storage = true;
              Index = &element - This->begin();
            }
        }
        This->grow(NewSize);
        return references_storage ? This->begin() + Index : &element;
      }

    public:
      using size_type = agt_u64_t;
      using difference_type = agt_i64_t;
      using value_type = T;
      using iterator   = T*;
      using const_iterator = const T *;

      using reverse_iterator = std::reverse_iterator<iterator>;
      using const_reverse_iterator = std::reverse_iterator<const_iterator>;

      using reference = T &;
      using const_reference = const T &;
      using pointer = T *;
      using const_pointer = const T *;

      using base::capacity;
      using base::empty;
      using base::size;


      // forward iterator creation methods.
      AGT_nodiscard iterator       begin()        noexcept { return (iterator)this->begin_; }
      AGT_nodiscard const_iterator begin()  const noexcept { return (const_iterator)this->begin_; }
      AGT_nodiscard const_iterator cbegin() const noexcept { return this->begin(); }

      AGT_nodiscard iterator       end()        noexcept { return begin() + size(); }
      AGT_nodiscard const_iterator end()  const noexcept { return begin() + size(); }
      AGT_nodiscard const_iterator cend() const noexcept { return this->end(); }

      // reverse iterator creation methods.
      AGT_nodiscard reverse_iterator       rbegin()        noexcept { return reverse_iterator(end()); }
      AGT_nodiscard const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator(end()); }
      AGT_nodiscard const_reverse_iterator crbegin() const noexcept { return this->rbegin(); }

      AGT_nodiscard reverse_iterator       rend()        noexcept { return reverse_iterator(begin()); }
      AGT_nodiscard const_reverse_iterator rend()  const noexcept { return const_reverse_iterator(begin()); }
      AGT_nodiscard const_reverse_iterator crend() const noexcept { return this->rend(); }

      AGT_nodiscard size_type size_in_bytes() const noexcept { return size() * sizeof(T); }
      AGT_nodiscard size_type max_size() const noexcept {
        constexpr static agt_u64_t max_elements = size_type(-1) / sizeof(T);
        return std::min(base::max_size(), max_elements);
      }

      AGT_nodiscard size_type capacity_in_bytes() const noexcept { return capacity() * sizeof(T); }

      /// Return a pointer to the vector's buffer, even if empty().
      AGT_nodiscard pointer data() noexcept { return pointer(begin()); }
      /// Return a pointer to the vector's buffer, even if empty().
      AGT_nodiscard const_pointer data() const noexcept { return const_pointer(begin()); }

      AGT_nodiscard reference operator[](size_type idx) noexcept {
        AGT_assert(idx < size());
        return begin()[idx];
      }
      AGT_nodiscard const_reference operator[](size_type idx) const noexcept {
        AGT_assert(idx < size());
        return begin()[idx];
      }

      AGT_nodiscard reference       front() noexcept {
        AGT_assert(!empty());
        return begin()[0];
      }
      AGT_nodiscard const_reference front() const noexcept {
        AGT_assert(!empty());
        return begin()[0];
      }

      AGT_nodiscard reference       back() noexcept {
        AGT_assert(!empty());
        return end()[-1];
      }
      AGT_nodiscard const_reference back() const noexcept {
        AGT_assert(!empty());
        return end()[-1];
      }
    };

    template <typename T, bool Trivial = trivial_type<T>>
    class small_array_template_base
        : public small_array_template_common<T> {
      friend class small_array_template_common<T>;

    protected:
      static constexpr bool takes_param_by_value = false;
      using value_param_t = const T&;

      explicit small_array_template_base(agt_u64_t size)
          : small_array_template_common<T>(size){}
      

      void destroy_range(T* start, T* end) noexcept {
        if constexpr ( !std::is_trivially_destructible_v<std::remove_cv_t<T>> ) {
          while ( start != end ) {
            --end;
            end->~T();
          }
        }
      }

      /// Move the range [I, E) into the uninitialized memory starting with "Dest",
      /// constructing elements as needed.
      template<typename It1, typename It2>
      void uninitialized_move(It1 I, It1 E, It2 Dest) noexcept {
        std::uninitialized_move(I, E, Dest);
        notify_init_range(std::addressof(*Dest), std::ranges::distance(I, E));
      }

      /// Copy the range [I, E) onto the uninitialized memory starting with "Dest",
      /// constructing elements as needed.
      template<typename It1, typename It2>
      void uninitialized_copy(It1 I, It1 E, It2 Dest) {
        std::uninitialized_copy(I, E, Dest);
      }

      /// Grow the allocated memory (without initializing new elements), doubling
      /// the size of the allocated memory. Guarantees space for at least one more
      /// element, or minimum_size more elements if specified.
      void grow(agt_u64_t minimum_size = 0);

      /// Create a new allocation big enough for \p minimum_size and pass back its size
      /// in \p new_capacity. This is the first section of \a grow().
      T* malloc_for_grow(agt_u64_t minimum_size, agt_u64_t& new_capacity) {
        return static_cast<T *>(vector_base<small_array_size_type<T>>::malloc_for_grow(
          minimum_size, 
          sizeof(T), 
          new_capacity, 
          alignof(T)));
      }

      /// Move existing elements over to the new allocation \p new_elements, the middle
      /// section of \a grow().
      void move_elements_for_grow(T *new_elements);

      /// Transfer ownership of the allocation, finishing up \a grow().
      void take_allocation_for_grow(T *new_elements, agt_u64_t new_capacity);

      /// Reserve enough space to add one element, and return the updated element
      /// pointer in case it was a reference to the storage.
      const T* reserve_for_param_and_get_address(const T &element, agt_u64_t N = 1) {
        return this->reserve_for_param_and_get_address_impl(this, element, N);
      }

      /// Reserve enough space to add one element, and return the updated element
      /// pointer in case it was a reference to the storage.
      T* reserve_for_param_and_get_address(T &element, agt_u64_t N = 1) {
        return const_cast<T *>(this->reserve_for_param_and_get_address_impl(this, element, N));
      }

      static T && forward_value_param(T &&V) { return std::move(V); }
      static const T & forward_value_param(const T &V) { return V; }

      void grow_and_assign(agt_u64_t num_elements, const T & element) {
        // Grow manually in case element is an internal reference.
        agt_u64_t new_capacity;
        T *new_elements = malloc_for_grow(num_elements, new_capacity);
        std::uninitialized_fill_n(new_elements, num_elements, element);
        notify_init_range(new_elements, num_elements);
        this->destroy_range(this->begin(), this->end());
        take_allocation_for_grow(new_elements, new_capacity);
        this->set_size(num_elements);
      }

      template <typename... ArgTypes> 
      T& grow_and_emplace_back(ArgTypes &&... Args) {
        // Grow manually in case one of Args is an internal reference.
        agt_u64_t new_capacity;
        T *new_elements = malloc_for_grow(0, new_capacity);
        ::new ((void *)(new_elements + this->size())) T(std::forward<ArgTypes>(Args)...);
        notify_init_object(new_elements + this->size());
        move_elements_for_grow(new_elements);
        take_allocation_for_grow(new_elements, new_capacity);
        this->set_size(this->size() + 1);
        return this->back();
      }

    public:
      void push_back(const T &element) {
        const T *element_ptr = reserve_for_param_and_get_address(element);
        ::new ((void *)this->end()) T(*element_ptr);
        this->set_size(this->size() + 1);
      }

      void push_back(T&& element) {
        T *element_ptr = reserve_for_param_and_get_address(element);
        ::new ((void *)this->end()) T(::std::move(*element_ptr));
        this->set_size(this->size() + 1);
      }

      void pop_back() {
        this->set_size(this->size() - 1);
        this->end()->~T();
      }
    };

    template <typename T, bool Trivial>
    void small_array_template_base<T, Trivial>::grow(agt_u64_t minimum_size) {
      agt_u64_t new_capacity;
      T *new_elements = this->malloc_for_grow(minimum_size, new_capacity);
      move_elements_for_grow(new_elements);
      take_allocation_for_grow(new_elements, new_capacity);
    }

    template <typename T, bool Trivial>
    void small_array_template_base<T, Trivial>::move_elements_for_grow(T *new_elements) {
      // Move the elements over.
      this->uninitialized_move(this->begin(), this->end(), new_elements);

      // Destroy the original elements.
      destroy_range(this->begin(), this->end());
    }

    template <typename T, bool Trivial>
    void small_array_template_base<T, Trivial>::take_allocation_for_grow(T *new_elements, agt_u64_t new_capacity) {
      // If this wasn't grown from the inline copy, deallocate the old space.
      if ( !this->is_small() )
        _aligned_free(this->begin()/*, this->capacity() * sizeof(T), alignof(T)*/);

      this->begin_ = new_elements;
      this->capacity_ = new_capacity;
    }


    template <typename T>
    class small_array_template_base<T, true> : public small_array_template_common<T> {
      friend class small_array_template_common<T>;
    protected:
      /// True if it's cheap enough to take parameters by value. Doing so avoids
      /// overhead related to mitigations for reference invalidation.
      static constexpr bool takes_param_by_value = sizeof(T) <= 2 * sizeof(void *);

      /// Either const T& or T, depending on whether it's cheap enough to take
      /// parameters by value.
      using value_param_t = std::conditional_t<takes_param_by_value, T, const T&>;


      explicit small_array_template_base(agt_u64_t Size)
          : small_array_template_common<T>(Size){}
      

      // No need to do a destroy loop for POD's.
      static void destroy_range(T *, T *) { }

      /// Move the range [I, E) onto the uninitialized memory
      /// starting with "Dest", constructing elements into it as needed.
      template<typename It1, typename It2>
      static void uninitialized_move(It1 I, It1 E, It2 Dest) {
        // Just do a copy.
        uninitialized_copy(I, E, Dest);
      }

      /// Copy the range [I, E) onto the uninitialized memory
      /// starting with "Dest", constructing elements into it as needed.
      template<typename It1, typename It2>
      static void uninitialized_copy(It1 I, It1 E, It2 Dest) {
        // Arbitrary iterator types; just use the basic implementation.
        std::uninitialized_copy(I, E, Dest);
      }

      /// Copy the range [I, E) onto the uninitialized memory
      /// starting with "Dest", constructing elements into it as needed.
      template <typename T1, typename T2> requires same_as<T1, T2>
      void uninitialized_copy(T1* I, T1* E, T2* Dest) {
        if (I != E)
          std::memcpy(reinterpret_cast<void *>(Dest), I, (E - I) * sizeof(T));
      }

      /// Double the size of the allocated memory, guaranteeing space for at
      /// least one more element or minimum_size if specified.
      void grow(agt_u64_t minimum_size = 0) { this->grow_pod(minimum_size, sizeof(T), alignof(T)); }

      /// Reserve enough space to add one element, and return the updated element
      /// pointer in case it was a reference to the storage.
      const T *reserve_for_param_and_get_address(const T &element, agt_u64_t N = 1) {
        return this->reserve_for_param_and_get_address_impl(this, element, N);
      }

      /// Reserve enough space to add one element, and return the updated element
      /// pointer in case it was a reference to the storage.
      T *reserve_for_param_and_get_address(T &element, agt_u64_t N = 1) {
        return const_cast<T *>(
          this->reserve_for_param_and_get_address_impl(this, element, N));
      }

      /// Copy \p V or return a reference, depending on \a value_param_t.
      static value_param_t forward_value_param(value_param_t V) { return V; }

      void grow_and_assign(agt_u64_t num_elements, T element) {
        // element has been copied in case it's an internal reference, side-stepping
        // reference invalidation problems without losing the realloc optimization.
        this->set_size(0);
        this->grow(num_elements);
        std::uninitialized_fill_n(this->begin(), num_elements, element);
        this->set_size(num_elements);
      }

      template <typename... ArgTypes> T &
      grow_and_emplace_back(ArgTypes &&... Args) {
        // Use push_back with a copy in case Args has an internal reference,
        // side-stepping reference invalidation problems without losing the realloc
        // optimization.
        push_back(T(std::forward<ArgTypes>(Args)...));
        return this->back();
      }

    public:
      void push_back(value_param_t element) {
        const T *element_ptr = reserve_for_param_and_get_address(element);
        std::memcpy(reinterpret_cast<void *>(this->end()), element_ptr, sizeof(T));
        this->set_size(this->size() + 1);
      }

      void pop_back() { this->set_size(this->size() - 1); }
    };


    template <typename T, size_t N>
    struct small_array_inline_storage{
      alignas(T) char inline_elements[sizeof(T) * N];
    };
    template <typename T>
    struct alignas(T) small_array_inline_storage<T, 0>{ };


    template <typename T>
    struct small_array_default_inline_elements{
      inline constexpr static agt_u64_t preferred_small_array_sizeof = 64;
      static_assert(
        sizeof(T) <= 256,
        "You are trying to use a default number of inlined elements for "
        "`small_array<T>` but `sizeof(T)` is really big! Please use an "
        "explicit number of inlined elements with `small_array<T, N>` to make "
        "sure you really want that much inline storage.");
      static constexpr size_t preferred_inline_bytes = preferred_small_array_sizeof - sizeof(vector<T, 0>);
      static constexpr size_t num_elements_that_fit = preferred_inline_bytes / sizeof(T);
      static constexpr size_t value = num_elements_that_fit == 0 ? 1 : num_elements_that_fit;
    };
  }
  
  template <typename T>
  class any_vector : public impl::small_array_template_base<T>{
  public:

    using supertype = impl::small_array_template_base<T>;
    
    using iterator = typename supertype::iterator;
    using const_iterator = typename supertype::const_iterator;
    using reference = typename supertype::reference;
    using size_type = typename supertype::size_type;

  protected:
    using supertype::takes_param_by_value;
    using value_param_t = typename supertype::value_param_t;

    // Default ctor - Initialize to empty.
    explicit any_vector(unsigned N) : supertype(N) {}



    void notify_init_range(void*, size_t) const noexcept {}

    void notify_init_object(void*) const noexcept {}

  public:
    any_vector(const any_vector&) = delete;

    ~any_vector() {
      // Subclass has already destructed this vector's elements.
      // If this wasn't grown from the inline copy, deallocate the old space.
      if (!this->is_small())
        _aligned_free(this->begin());
    }

    void clear() {
      this->destroy_range(this->begin(), this->end());
      this->set_size(0);
    }

  private:
    template <bool ForOverwrite> 
    void resize_impl(size_type N) {
      if (N < this->size()) {
        this->pop_back_n(this->size() - N);
      } else if (N > this->size()) {
        this->reserve(N);
        for (auto I = this->end(), E = this->begin() + N; I != E; ++I)
          if constexpr (ForOverwrite)
            new (std::addressof(*I)) T;
          else
            new (std::addressof(*I)) T();
        this->notify_init_range(this->end(), N - this->size());
        this->set_size(N);
      }
    }

  public:
    void resize(size_type N) { resize_impl<false>(N); }

    /// Like resize, but \ref T is POD, the new values won't be initialized.
    void resize_for_overwrite(size_type N) { resize_impl<true>(N); }

    void resize(size_type N, value_param_t NV) {
      if (N == this->size())
        return;

      if (N < this->size()) {
        this->pop_back_n(this->size() - N);
        return;
      }

      // N > this->size(). Defer to append.
      this->append(N - this->size(), NV);
    }

    void reserve(size_type N) {
      if (this->capacity() < N)
        this->grow(N);
    }

    void pop_back_n(size_type NumItems) {
      AGT_assert(this->size() >= NumItems);
      this->destroy_range(this->end() - NumItems, this->end());
      this->set_size(this->size() - NumItems);
    }

    AGT_nodiscard T pop_back_val() {
      T Result = ::std::move(this->back());
      this->pop_back();
      return Result;
    }

    void swap(any_vector&RHS);

    /// Add the specified range to the end of the small_array.
    template <typename in_iter> requires std::input_iterator<in_iter>
    void append(in_iter in_start, in_iter in_end) {
      this->assert_safe_to_add_range(in_start, in_end);
      size_type NumInputs = std::distance(in_start, in_end);
      this->reserve(this->size() + NumInputs);
      this->uninitialized_copy(in_start, in_end, this->end());
      this->notify_init_range(this->end(), NumInputs);
      this->set_size(this->size() + NumInputs);
    }

    /// Append \p NumInputs copies of \p Elt to the end.
    void append(size_type NumInputs, value_param_t Elt) {
      const T *EltPtr = this->reserve_for_param_and_get_address(Elt, NumInputs);
      std::uninitialized_fill_n(this->end(), NumInputs, *EltPtr);
      this->notify_init_range(this->end(), NumInputs);
      this->set_size(this->size() + NumInputs);
    }

    void append(std::initializer_list<T> IL) {
      append(IL.begin(), IL.end());
    }

    void append(const any_vector&RHS) { append(RHS.begin(), RHS.end()); }

    void assign(size_type NumElts, value_param_t Elt) {
      // Note that Elt could be an internal reference.
      if (NumElts > this->capacity()) {
        this->grow_and_assign(NumElts, Elt);
        return;
      }

      // Assign over existing elements.
      std::fill_n(this->begin(), std::min(NumElts, this->size()), Elt);
      if (NumElts > this->size()) {
        std::uninitialized_fill_n(this->end(), NumElts - this->size(), Elt);
        this->notify_init_range(this->end(), NumElts - this->size());
      }
      else if (NumElts < this->size())
        this->destroy_range(this->begin() + NumElts, this->end());
      this->set_size(NumElts);
    }

    // FIXME: Consider assigning over existing elements, rather than clearing &
    // re-initializing them - for all assign(...) variants.

    template <typename in_iter> requires std::input_iterator<in_iter>
    void assign(in_iter in_start, in_iter in_end) {
      this->assert_safe_to_reference_after_clear(in_start, in_end);
      clear();
      append(in_start, in_end);
    }

    void assign(std::initializer_list<T> IL) {
      clear();
      append(IL);
    }

    void assign(const any_vector&RHS) { assign(RHS.begin(), RHS.end()); }

    iterator erase(const_iterator CI) {
      // Just cast away constness because this is a non-const member function.
      iterator I = const_cast<iterator>(CI);

      AGT_assert(this->is_reference_to_storage(CI) && "Iterator to erase is out of bounds.");

      iterator N = I;
      // Shift all elts down one.
      std::move(I+1, this->end(), I);
      // Drop the last elt.
      this->pop_back();
      return(N);
    }

    iterator erase(const_iterator CS, const_iterator CE) {
      // Just cast away constness because this is a non-const member function.
      iterator S = const_cast<iterator>(CS);
      iterator E = const_cast<iterator>(CE);

      AGT_assert(this->is_range_in_storage(S, E) && "Range to erase is out of bounds.");

      iterator N = S;
      // Shift all elts down.
      iterator I = std::move(E, this->end(), S);
      // Drop the last elts.
      this->destroy_range(I, this->end());
      this->set_size(I - this->begin());
      return(N);
    }

  private:
    template <typename ArgType> 
    iterator insert_one_impl(iterator I, ArgType &&Elt) {
      // Callers ensure that ArgType is derived from T.
      static_assert(
        std::is_same<std::remove_const_t<std::remove_reference_t<ArgType>>,
                     T>::value,
        "ArgType must be derived from T!");

      if (I == this->end()) {  // Important special case for empty vector.
        this->push_back(::std::forward<ArgType>(Elt));
        return this->end()-1;
      }

      AGT_assert(this->is_reference_to_storage(I) && "Insertion iterator is out of bounds.");

      // Grow if necessary.
      size_t Index = I - this->begin();
      std::remove_reference_t<ArgType> *EltPtr =
        this->reserve_for_param_and_get_address(Elt);
      I = this->begin() + Index;

      ::new ((void*) this->end()) T(::std::move(this->back()));
      // Push everything else over.
      std::move_backward(I, this->end()-1, this->end());
      this->set_size(this->size() + 1);

      // If we just moved the element we're inserting, be sure to update
      // the reference (never happens if takes_param_by_value).
      static_assert(!takes_param_by_value || std::is_same<ArgType, T>::value,
                    "ArgType must be 'T' when taking by value!");
      if constexpr ( !takes_param_by_value ) {
        if (this->is_reference_to_range(EltPtr, I, this->end()))
          ++EltPtr;
      }
      
      *I = ::std::forward<ArgType>(*EltPtr);
      return I;
    }

  public:
    iterator insert(iterator I, T &&Elt) {
      return insert_one_impl(I, this->forward_value_param(std::move(Elt)));
    }

    iterator insert(iterator I, const T &Elt) {
      return insert_one_impl(I, this->forward_value_param(Elt));
    }

    iterator insert(iterator I, size_type NumToInsert, value_param_t Elt) {
      // Convert iterator to elt# to avoid invalidating iterator when we reserve()
      size_t InsertElt = I - this->begin();

      if (I == this->end()) {  // Important special case for empty vector.
        append(NumToInsert, Elt);
        return this->begin()+InsertElt;
      }

      AGT_assert(this->is_reference_to_storage(I) && "Insertion iterator is out of bounds.");

      // Ensure there is enough space, and get the (maybe updated) address of
      // Elt.
      const T *EltPtr = this->reserve_for_param_and_get_address(Elt, NumToInsert);

      // Uninvalidate the iterator.
      I = this->begin()+InsertElt;

      // If there are more elements between the insertion point and the end of the
      // range than there are being inserted, we can use a simple approach to
      // insertion.  Since we already reserved space, we know that this won't
      // reallocate the vector.
      if (size_t(this->end()-I) >= NumToInsert) {
        T *OldEnd = this->end();
        append(std::move_iterator<iterator>(this->end() - NumToInsert),
               std::move_iterator<iterator>(this->end()));

        // Copy the existing elements that get replaced.
        std::move_backward(I, OldEnd-NumToInsert, OldEnd);

        // If we just moved the element we're inserting, be sure to update
        // the reference (never happens if takes_param_by_value).
        if constexpr ( !takes_param_by_value ) {
          if (I <= EltPtr && EltPtr < this->end())
            EltPtr += NumToInsert;
        }
        

        std::fill_n(I, NumToInsert, *EltPtr);
        return I;
      }

      // Otherwise, we're inserting more elements than exist already, and we're
      // not inserting at the end.

      // Move over the elements that we're about to overwrite.
      T *OldEnd = this->end();
      this->set_size(this->size() + NumToInsert);
      size_t NumOverwritten = OldEnd-I;
      this->uninitialized_move(I, OldEnd, this->end()-NumOverwritten);

      // If we just moved the element we're inserting, be sure to update
      // the reference (never happens if takes_param_by_value).
      if constexpr ( !takes_param_by_value ) {
        if (I <= EltPtr && EltPtr < this->end())
          EltPtr += NumToInsert;
      }
      
      // Replace the overwritten part.
      std::fill_n(I, NumOverwritten, *EltPtr);

      // Insert the non-overwritten middle part.
      std::uninitialized_fill_n(OldEnd, NumToInsert - NumOverwritten, *EltPtr);
      
      return I;
    }

    template <typename ItTy> requires std::input_iterator<ItTy>
      iterator insert(iterator I, ItTy From, ItTy To) {
      // Convert iterator to elt# to avoid invalidating iterator when we reserve()
      size_t InsertElt = I - this->begin();

      if (I == this->end()) {  // Important special case for empty vector.
        append(From, To);
        return this->begin()+InsertElt;
      }

      AGT_assert(this->is_reference_to_storage(I) && "Insertion iterator is out of bounds.");

      // Check that the reserve that follows doesn't invalidate the iterators.
      this->assert_safe_to_add_range(From, To);

      size_t NumToInsert = std::distance(From, To);

      // Ensure there is enough space.
      reserve(this->size() + NumToInsert);

      // Uninvalidate the iterator.
      I = this->begin()+InsertElt;

      // If there are more elements between the insertion point and the end of the
      // range than there are being inserted, we can use a simple approach to
      // insertion.  Since we already reserved space, we know that this won't
      // reallocate the vector.
      if (size_t(this->end()-I) >= NumToInsert) {
        T *OldEnd = this->end();
        append(std::move_iterator<iterator>(this->end() - NumToInsert),
               std::move_iterator<iterator>(this->end()));

        // Copy the existing elements that get replaced.
        std::move_backward(I, OldEnd-NumToInsert, OldEnd);

        std::copy(From, To, I);
        return I;
      }

      // Otherwise, we're inserting more elements than exist already, and we're
      // not inserting at the end.

      // Move over the elements that we're about to overwrite.
      T *OldEnd = this->end();
      this->set_size(this->size() + NumToInsert);
      size_t NumOverwritten = OldEnd-I;
      this->uninitialized_move(I, OldEnd, this->end()-NumOverwritten);

      // Replace the overwritten part.
      for (T *J = I; NumOverwritten > 0; --NumOverwritten) {
        *J = *From;
        ++J; ++From;
      }

      // Insert the non-overwritten middle part.
      this->uninitialized_copy(From, To, OldEnd);
      return I;
    }

    void insert(iterator I, std::initializer_list<T> IL) {
      insert(I, IL.begin(), IL.end());
    }

    template <typename... ArgTypes> reference emplace_back(ArgTypes &&... Args) {
      if (this->size() >= this->capacity()) [[unlikely]]
                                            return this->grow_and_emplace_back(std::forward<ArgTypes>(Args)...);

      ::new ((void *)this->end()) T(std::forward<ArgTypes>(Args)...);
      this->notify_init_object(this->end());
      this->set_size(this->size() + 1);
      return this->back();
    }

    any_vector&operator=(const any_vector&RHS);

    any_vector&operator=(any_vector&&RHS) noexcept;

    friend bool operator==(const any_vector& a, const any_vector& b) noexcept {
      if (a.size() != b.size())
        return false;
      return std::equal(a.begin(), a.end(), b.begin());
    }

    friend bool operator<=>(const any_vector& a, const any_vector& b) noexcept {
      return std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end());
    }
  };


  template <typename T>
  void any_vector<T>::swap(any_vector& RHS) {
    if (this == &RHS) 
      return;

    // We can only avoid copying elements if neither vector is small.
    if (!this->is_small() && !RHS.is_small()) {
      std::swap(this->begin_, RHS.begin_);
      std::swap(this->size_, RHS.size_);
      std::swap(this->capacity_, RHS.capacity_);
      this->container_swap(RHS);
      return;
    }
    this->reserve(RHS.size());
    RHS.reserve(this->size());

    // Swap the shared elements.
    size_t NumShared = std::min(this->size(), RHS.size());
    for (size_type i = 0; i != NumShared; ++i)
      std::swap((*this)[i], RHS[i]);

    // Copy over the extra elts.
    if ( this->size() > RHS.size() ) {
      size_t EltDiff = this->size() - RHS.size();
      RHS.uninitialized_copy(this->begin()+NumShared, this->end(), RHS.end());
      RHS.set_size(RHS.size() + EltDiff);
      this->destroy_range(this->begin() + NumShared, this->end());
      this->set_size(NumShared);
    }
    else if (RHS.size() > this->size()) {
      size_t EltDiff = RHS.size() - this->size();
      this->uninitialized_copy(RHS.begin() + NumShared, RHS.end(), this->end());
      this->set_size(this->size() + EltDiff);
      RHS.destroy_range(RHS.begin() + NumShared, RHS.end());
      RHS.set_size(NumShared);
    }
  }


  // TODO: Improve assignment operators for any_small_array to take bitwise_movable_c into account
  template <typename T>
  any_vector<T>& any_vector<T>::operator=(const any_vector& RHS) {
    // Avoid self-assignment.
    if (this == &RHS)
      return *this;

    // If we already have sufficient space, assign the common elements, then
    // destroy any excess.
    size_t RHSSize = RHS.size();
    size_t CurSize = this->size();
    if (CurSize >= RHSSize) {
      // Assign common elements.
      iterator NewEnd;
      if (RHSSize)
        NewEnd = std::copy(RHS.begin(), RHS.begin()+RHSSize, this->begin());
      else
        NewEnd = this->begin();

      // Destroy excess elements.
      this->destroy_range(NewEnd, this->end());

      // Trim.
      this->set_size(RHSSize);
      return *this;
    }

    // If we have to grow to have enough elements, destroy the current elements.
    // This allows us to avoid copying them during the grow.
    // FIXME: don't do this if they're efficiently moveable.
    if (this->capacity() < RHSSize) {
      // Destroy current elements.
      this->clear();
      CurSize = 0;
      this->grow(RHSSize);
    } else if (CurSize) {
      // Otherwise, use assignment for the already-constructed elements.
      std::copy(RHS.begin(), RHS.begin()+CurSize, this->begin());
    }

    // Copy construct the new elements in place.
    this->uninitialized_copy(RHS.begin()+CurSize, RHS.end(),
                             this->begin()+CurSize);

    // Set end.
    this->set_size(RHSSize);
    return *this;
  }
  template <typename T>
  any_vector<T>& any_vector<T>::operator=(any_vector&&RHS) noexcept {
    // Avoid self-assignment.
    if (this == &RHS) return *this;

    // If the RHS isn't small, clear this vector and then steal its buffer.
    if (!RHS.is_small()) {
      this->destroy_range(this->begin(), this->end());
      if (!this->is_small()) free(this->begin());
      this->begin_ = RHS.begin_;
      this->size_ = RHS.size_;
      this->capacity_ = RHS.capacity_;
      RHS.resetToSmall();
      return *this;
    }

    // If we already have sufficient space, assign the common elements, then
    // destroy any excess.
    size_t RHSSize = RHS.size();
    size_t CurSize = this->size();
    if (CurSize >= RHSSize) {
      // Assign common elements.
      iterator NewEnd = this->begin();
      if (RHSSize)
        NewEnd = std::move(RHS.begin(), RHS.end(), NewEnd);

      // Destroy excess elements and trim the bounds.
      this->destroy_range(NewEnd, this->end());
      this->set_size(RHSSize);

      // Clear the RHS.
      RHS.clear();

      return *this;
    }

    // If we have to grow to have enough elements, destroy the current elements.
    // This allows us to avoid copying them during the grow.
    // FIXME: this may not actually make any sense if we can efficiently move
    // elements.
    if (this->capacity() < RHSSize) {
      // Destroy current elements.
      this->clear();
      CurSize = 0;
      this->grow(RHSSize);
    } else if (CurSize) {
      // Otherwise, use assignment for the already-constructed elements.
      std::move(RHS.begin(), RHS.begin()+CurSize, this->begin());
    }

    // Move-construct the new elements in place.
    this->uninitialized_move(RHS.begin()+CurSize, RHS.end(),
                             this->begin()+CurSize);

    // Set end.
    this->set_size(RHSSize);

    RHS.clear();
    return *this;
  }






  // FIXME: Add constructors with allocator parameters, and make sure that allocators are
  //        appropriately moved around during assignment and such
  template <typename T, size_t N = impl::small_array_default_inline_elements<T>::value>
  class AGT_empty_bases vector : public any_vector<T>, impl::small_array_inline_storage<T, N>{
  public:
    vector() : any_vector<T>(N) {}

    ~vector() {
      // Destroy the constructed elements in the vector.
      this->destroy_range(this->begin(), this->end());
    }

    explicit vector(size_t Size, const T& value = T()) : any_vector<T>(N) {
      this->assign(Size, value);
    }

    template <typename ItTy> requires std::input_iterator<ItTy>
    vector(ItTy S, ItTy E) : any_vector<T>(N) {
      this->append(S, E);
    }

    template <std::ranges::range RangeTy>
    explicit vector(RangeTy&& rng) : any_vector<T>(N) {
      this->append(std::ranges::begin(rng), std::ranges::end(rng));
    }

    vector(std::initializer_list<T> IL) : any_vector<T>(N) {
      this->assign(IL);
    }

    vector(const vector&RHS) : any_vector<T>(N) {
      if (!RHS.empty())
        any_vector<T>::operator=(RHS);
    }

    vector&operator=(const vector&RHS) {
      any_vector<T>::operator=(RHS);
      return *this;
    }

    vector(vector&& RHS) : any_vector<T>(N) {
      if (!RHS.empty())
        any_vector<T>::operator=(::std::move(RHS));
    }

    vector(any_vector<T>&& RHS) : any_vector<T>(N) {
      if (!RHS.empty())
        any_vector<T>::operator=(::std::move(RHS));
    }

    vector&operator=(vector&&RHS) noexcept {
      any_vector<T>::operator=(::std::move(RHS));
      return *this;
    }

    vector&operator=(any_vector<T>&& RHS) noexcept {
      any_vector<T>::operator=(::std::move(RHS));
      return *this;
    }

    vector&operator=(std::initializer_list<T> IL) {
      this->assign(IL);
      return *this;
    }
  };



  extern template class impl::vector_base<agt_u32_t>;
  extern template class impl::vector_base<agt_u64_t>;
}

#endif//JEMSYS_INTERNAL_VECTOR_HPP
