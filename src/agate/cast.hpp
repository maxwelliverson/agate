//
// Created by maxwe on 2023-01-14.
//

#ifndef AGATE_CAST_HPP
#define AGATE_CAST_HPP

#include "config.hpp"
#include "atomic.hpp"

namespace agt {

  template <typename To, typename From>
  struct horizontal_casting /*{
      inline static bool      isa(const From& from) noexcept;
      inline static const To* cast(const From* from) noexcept;
    }*/;

  template <typename To, typename From>
  struct handle_casting /*{
    inline static bool isa(From from) noexcept;
    inline static To*  cast(From from) noexcept;
  }*/;

  namespace cast_impl {
    template <typename To, typename From>
    concept horizontal_castable_from =
        requires(const From& ref, const From* ptr){
          sizeof(horizontal_casting<To, From>);
          { horizontal_casting<To, From>::isa(ref) } -> std::convertible_to<bool>;
          { horizontal_casting<To, From>::cast(ptr) } -> std::same_as<const To*>;
        };

    template <typename To, typename From>
    struct error_incompatible_types {
      inline constexpr static bool value = false;
    };

    template <typename Obj>
    concept has_object_type_id = requires{
                                   sizeof(::agt::impl::obj_types::object_type_id<Obj>);
                                   { ::agt::impl::obj_types::object_type_id<Obj>::value } -> std::convertible_to<object_type>;
                                 };

    template <typename Obj>
    concept has_object_type_range = requires {
                                      sizeof(::agt::impl::obj_types::object_type_id<Obj>);
                                      { ::agt::impl::obj_types::object_type_id<Obj>::minValue } -> std::convertible_to<object_type>;
                                      { ::agt::impl::obj_types::object_type_id<Obj>::maxValue } -> std::convertible_to<object_type>;
                                    };

    template <typename To>
    struct error_unregistered_type {
      inline constexpr static bool value = false;
    };

    template <typename To>
    concept defined = requires{
                        sizeof(To);
                      };

    template <typename To, typename From>
    concept static_castable_from =
        requires(const From* from) {
          static_cast<const To*>(from);
        };

    template <typename To, typename From>
    concept handle_castable_from =
        std::is_trivial_v<From> &&
        sizeof(From) <= sizeof(void*) &&
        requires(From from) {
          sizeof(handle_casting<To, From>);
          { handle_casting<To, From>::isa(from) } -> std::convertible_to<bool>;
          { handle_casting<To, From>::cast(from) } -> std::same_as<To*>;
        };


    template <typename To, typename From>
    AGT_forceinline static const To* do_object_cast(const From* from) noexcept {
      if constexpr (horizontal_castable_from<To, From>)
        return horizontal_casting<To, From>::cast(from);
      else if constexpr (static_castable_from<To, From>)
        return static_cast<const To*>(from);
      else
        static_assert(error_incompatible_types<To, From>::value, "No procedure exists to cast a pointer to From to a pointer to To.");

    }
  }

  template <typename Type, typename Src>
  AGT_forceinline static bool object_isa(const Src& src) noexcept {

    using namespace cast_impl;

    if constexpr (std::derived_from<Src, Type>)
      return true;
    else if constexpr (std::derived_from<Type, Src>) {
      if constexpr (has_object_type_id<Type>) {
        return AGT_type_id_of(Type) == src.type;
      }
      else if constexpr (has_object_type_range<Type>) {
        return (AGT_type_id_min(Type) <= src.type) && (src.type <= AGT_type_id_max(Type));
      }
      else {
        static_assert(defined<Type>, "Type has not yet been defined");
        static_assert(std::derived_from<Type, object>, "Type is not derived from object");
        static_assert(error_unregistered_type<Type>::value, "Type was defined without using the appropriate AGT_*_object_type macro.");
      }
    }
    else if constexpr (horizontal_castable_from<Type, Src>) {
      return horizontal_casting<Type, Src>::isa(src);
    }
    else {
      static_assert(error_incompatible_types<Type, Src>::value, "No casting procedure exists for these two types.");
    }
  }

  template <typename Type, typename Src>
  AGT_forceinline static bool object_isa(const Src* src) noexcept {
    return src ? object_isa<Type>(*src) : false;
  }



  template <typename Type, typename Hdl>
  AGT_forceinline static bool handle_isa(Hdl handle) noexcept {
    static_assert(cast_impl::handle_castable_from<Type, Hdl>);
    return handle_casting<Type, Hdl>::isa(handle);
  }



  template <typename To, typename From>
  AGT_forceinline static To*       unsafe_handle_cast(From handle) noexcept {
    static_assert(cast_impl::handle_castable_from<To, From>);
    AGT_invariant( handle_isa<To>(handle) );
    return handle_casting<To, From>::cast(handle);
  }

  template <typename To, typename From>
  AGT_forceinline static To*       handle_cast(From handle) noexcept {
    static_assert(cast_impl::handle_castable_from<To, From>);
    return handle_isa<To>(handle) ? unsafe_handle_cast<To>(handle) : nullptr;
  }

  template <typename To, typename From>
  AGT_forceinline static const To& object_cast(const From& obj) noexcept {
    AGT_invariant( object_isa<To>(obj) );
    return *cast_impl::do_object_cast<To>(std::addressof(obj));
  }

  template <typename To, typename From>
  AGT_forceinline static To&       object_cast(From& obj) noexcept {
    return const_cast<To&>(object_cast<To>(const_cast<const From&>(obj)));
  }

  template <typename To, typename From>
  AGT_forceinline static const To* object_cast(const From* obj) noexcept {
    return object_isa<To>(obj) ? cast_impl::do_object_cast<To>(obj) : nullptr;
  }

  template <typename To, typename From>
  AGT_forceinline static To*       object_cast(From* obj) noexcept {
    return const_cast<To*>(object_cast<To>(const_cast<const From*>(obj)));
  }




  /// Casts from [const] void*, which are fairly common in the public interfaces.

  template <typename Type>
  AGT_forceinline static bool nonnull_object_isa(const void* p) noexcept {
    return object_isa<Type>(*static_cast<const object*>(p));
  }

  template <typename Type>
  AGT_forceinline static bool object_isa(const void* p) noexcept {
    return p ? nonnull_object_isa<Type>(p) : false;
  }


  template <typename To>
  AGT_forceinline static const To* nonnull_object_cast(const void* ptr) noexcept {
    AGT_invariant( ptr != nullptr );
    return nonnull_object_isa<To>(ptr) ? cast_impl::do_object_cast<To>(static_cast<const object*>(ptr)) : nullptr;
  }

  template <typename To>
  AGT_forceinline static To*       nonnull_object_cast(void* ptr) noexcept {
    return const_cast<To*>(nonnull_object_cast<To>(const_cast<const void*>(ptr)));
  }

  template <typename To>
  AGT_forceinline static const To* object_cast(const void* ptr) noexcept {
    return object_isa<To>(ptr) ? cast_impl::do_object_cast<To>(static_cast<const object*>(ptr)) : nullptr;
  }

  template <typename To>
  AGT_forceinline static To*       object_cast(void* ptr) noexcept {
    return const_cast<To*>(object_cast<To>(const_cast<const void*>(ptr)));
  }

  template <typename To>
  AGT_forceinline static const To* unsafe_nonnull_object_cast(const void* ptr) noexcept {
    AGT_invariant( ptr != nullptr );
    AGT_invariant( nonnull_object_isa<To>(ptr) );
    return cast_impl::do_object_cast<To>(static_cast<const object*>(ptr));
  }

  template <typename To>
  AGT_forceinline static To*       unsafe_nonnull_object_cast(void* ptr) noexcept {
    return const_cast<To*>(unsafe_nonnull_object_cast<To>(const_cast<const void*>(ptr)));
  }

  template <typename To>
  AGT_forceinline static const To* unsafe_object_cast(const void* ptr) noexcept {
    AGT_invariant( ptr == nullptr || nonnull_object_isa<To>(ptr) ); // either ptr is null, or is *is* type To
    return ptr ? unsafe_nonnull_object_cast<To>(ptr) : nullptr;
  }

  template <typename To>
  AGT_forceinline static To*       unsafe_object_cast(void* ptr) noexcept {
    AGT_invariant( ptr == nullptr || nonnull_object_isa<To>(ptr) ); // either ptr is null, or is *is* type To
    return ptr ? unsafe_nonnull_object_cast<To>(ptr) : nullptr;
  }




  namespace cast_impl {
    template <auto MemberPtr>
    struct member_ptr_info;
    template <typename S, typename M, M S::* MemberPtr>
    struct member_ptr_info<MemberPtr>{
      using member_type = M;
      using struct_type = S;
    };
  }


  template <auto MemberPtr, typename From>
  AGT_forceinline static auto      member_to_struct_cast(From* from) noexcept {
    using from_t = std::remove_const_t<From>;
    using info = cast_impl::member_ptr_info<MemberPtr>;
    static_assert( not std::is_volatile_v<from_t> );
    static_assert( std::same_as<typename info::member_type, from_t> );
    using base_struct = typename info::struct_type;
    using to_t = std::conditional_t<std::is_const_v<From>, const base_struct, base_struct>;
    using byte_t = std::conditional_t<std::is_const_v<From>, const std::byte, std::byte>;


    const auto Offset = std::bit_cast<impl::atomic_type_t<decltype(MemberPtr)>>(MemberPtr);
    return reinterpret_cast<to_t*>(reinterpret_cast<byte_t*>(from) - Offset);
  }
}

#endif//AGATE_CAST_HPP
