//
// Created by maxwe on 2022-07-13.
//

#ifndef AGATE_CORE_H
#define AGATE_CORE_H


/* ====================[ Portability Config ]================== */


#if defined(__cplusplus)
#define AGT_begin_c_namespace extern "C" {
#define AGT_end_c_namespace }

#include <cassert>
#include <cstddef>

#else
#define AGT_begin_c_namespace
#define AGT_end_c_namespace

#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#endif

#if defined(NDEBUG)
#define AGT_debug_build false
#else
#define AGT_debug_build true
#endif

#if defined(_WIN64) || defined(_WIN32)
#define AGT_system_windows true
#define AGT_system_linux false
#define AGT_system_macos false
#if defined(_WIN64)
#define AGT_64bit true
#define AGT_32bit false
#else
#define AGT_64bit false
#define AGT_32bit true
#endif
#else
#define AGT_system_windows false
#if defined(__LP64__)
#define AGT_64bit true
#define AGT_32bit false
#else
#define AGT_64bit false
#define AGT_32bit true
#endif
#if defined(__linux__)

#define AGT_system_linux true
#define AGT_system_macos false
#elif defined(__OSX__)
#define AGT_system_linux false
#define AGT_system_macos true
#else
#define AGT_system_linux false
#define AGT_system_macos false
#endif
#endif


#if defined(_MSC_VER)
#define AGT_compiler_msvc true
#else
#define AGT_compiler_msvc false
#endif


#if defined(__clang__)
#define AGT_compiler_clang true
#else
#define AGT_compiler_clang false
#endif
#if defined(__GNUC__)
#define AGT_compiler_gcc true
#else
#define AGT_compiler_gcc false
#endif




/* =================[ Portable Attributes ]================= */



#if defined(__cplusplus)
# if AGT_compiler_msvc
#  define AGT_cplusplus _MSVC_LANG
# else
#  define AGT_cplusplus __cplusplus
# endif
# if AGT_cplusplus >= 201103L  // C++11
#  define AGT_noexcept   noexcept
#  define AGT_noreturn [[noreturn]]
# else
#  define AGT_noexcept   throw()
# endif
# if (AGT_cplusplus >= 201703)
#  define AGT_nodiscard [[nodiscard]]
# else
#  define AGT_nodiscard
# endif
#else
# define AGT_cplusplus 0
# define AGT_noexcept
# if (__GNUC__ >= 4) || defined(__clang__)
#  define AGT_nodiscard    __attribute__((warn_unused_result))
# elif (_MSC_VER >= 1700)
#  define AGT_nodiscard    _Check_return_
# else
#  define AGT_nodiscard
# endif
#endif


# if defined(__has_attribute)
#  define AGT_has_attribute(...) __has_attribute(__VA_ARGS__)
# else
#  define AGT_has_attribute(...) false
# endif

#define AGT_static_api
#define AGT_core_api
#define AGT_agents_api
#define AGT_async_api
#define AGT_exec_api     // This isn't a distinct module, but is rather a tag for functions that should only be called from custom executors
#define AGT_shmem_api
#define AGT_channels_api
#define AGT_log_api
#define AGT_network_api
#define AGT_pool_api

#if defined(_MSC_VER) || defined(__MINGW32__)
# if defined(__MINGW32__)
#  define AGT_restricted
# else
#  define AGT_restricted __declspec(restrict)
# endif
# pragma warning(disable:4127)   // suppress constant conditional warning
# define AGT_noinline          __declspec(noinline)
# define AGT_forceinline       __forceinline
# define AGT_noalias           __declspec(noalias)
# define AGT_thread_local      __declspec(thread)
# define AGT_alignas(n)        __declspec(align(n))
# define AGT_restrict          __restrict
# define AGT_nonnull
# define AGT_nonnull_params(...)
#
# define AGT_cdecl __cdecl
# define AGT_stdcall __stdcall
# define AGT_vectorcall __vectorcall
# define AGT_may_alias
# define AGT_alloc_size(...)
# define AGT_alloc_align(p)
#
# define AGT_assume(...) __assume(__VA_ARGS__)
# define AGT_assert(expr) assert(expr)
# define AGT_unreachable AGT_assert(false); AGT_assume(false)
#
# if !defined(AGT_noreturn)
#  define AGT_noreturn __declspec(noreturn)
# endif
#
#elif defined(__GNUC__)                 // includes clang and icc
#
# if defined(__cplusplus)
# define AGT_restrict __restrict
# else
# define AGT_restrict restrict
# endif
# define AGT_cdecl                      // leads to warnings... __attribute__((cdecl))
# define AGT_stdcall
# define AGT_vectorcall
# define AGT_may_alias    __attribute__((may_alias))
# define AGT_restricted
# define AGT_malloc       __attribute__((malloc))
# define AGT_noinline     __attribute__((noinline))
# define AGT_noalias
# define AGT_thread_local __thread
# define AGT_alignas(n)   __attribute__((aligned(n)))
# define AGT_assume(...)
# define AGT_forceinline  __attribute__((always_inline))
# define AGT_unreachable  __builtin_unreachable()
# define AGT_nonnull_params(...) __attribute__((nonnull(__VA_ARGS__)))
# if AGT_compiler_clang
#  define AGT_nonnull _Nonnull
# else
#  define AGT_nonnull
# endif
#
# if (defined(__clang_major__) && (__clang_major__ < 4)) || (__GNUC__ < 5)
#  define AGT_alloc_size(...)
#  define AGT_alloc_align(p)
# elif defined(__INTEL_COMPILER)
#  define AGT_alloc_size(...)       __attribute__((alloc_size(__VA_ARGS__)))
#  define AGT_alloc_align(p)
# else
#  define AGT_alloc_size(...)       __attribute__((alloc_size(__VA_ARGS__)))
#  define AGT_alloc_align(p)      __attribute__((alloc_align(p)))
# endif
#
# if !defined(AGT_noreturn)
#  define AGT_noreturn __attribute__((noreturn))
# endif
#
#else
# define AGT_restrict
# define AGT_cdecl
# define AGT_stdcall
# define AGT_vectorcall
# define AGT_may_alias
# define AGT_restricted
# define AGT_malloc
# define AGT_alloc_size(...)
# define AGT_alloc_align(p)
# define AGT_noinline
# define AGT_noalias
# define AGT_forceinline
# define AGT_thread_local            __thread        // hope for the best :-)
# define AGT_nonnull
# define AGT_nonnull_params(...)
# define AGT_alignas(n)
# define AGT_assume(...)
# define AGT_unreachable AGT_assume(false)
#endif


#if 0
#if AGT_cplusplus > 0
namespace agtxx {
# if AGT_cplusplus >= 201103L

  namespace meta {

    template <typename T, typename ...Types>
    struct enable_if_one_of;
    template <typename T, typename Head, typename ...Tail>
    struct enable_if_one_of<T, Head, Tail...> : enable_if_one_of<T, Tail...> { };
    template <typename T, typename ...Tail>
    struct enable_if_one_of<T, T, Tail...> {
      using type = int;
    };
    template <typename T>
    struct enable_if_one_of<T>;
    template <typename T, typename ...Types>
    using enable_if_one_of_t = typename enable_if_one_of<T, Types...>::type;

    struct tu_list_head;

    template <class...>
    using void_t = void;

    template <bool Value>
    struct boolean_value {
      constexpr static bool value = Value;
      using type = boolean_value<Value>;
    };

    using true_type = boolean_value<true>;

    using false_type = boolean_value<false>;

    template <typename T,
             template <typename...> class TT,
             typename = void>
    struct is_instantiable : false_type { };

    template <typename T, template <typename...> class TT>
    struct is_instantiable<T, TT, void_t<TT<T>>> : true_type { };

    template <typename T>
    auto declval() -> T;

    template <typename T,
             typename = boolean_value<sizeof(T) == sizeof(unsigned long long)>,
             typename = void>
    struct valid_tu_member;
    template <typename T>
    struct valid_tu_member<T,
                           true_type,
                           void_t<decltype((void*)(declval<T>())),
                                  decltype((T)(declval<void*>()))>> { };

    template <typename Void, template <typename...> class TT, typename ...Types>
    struct all_instantiable : false_type { };
    template <template <typename...> class TT, typename Head, typename ...Types>
    struct all_instantiable<void_t<TT<Head>>, TT, Head, Types...> : all_instantiable<void, TT, Types...> {};
    template <template <typename...> class TT>
    struct all_instantiable<void, TT> : true_type {};

    template <typename ...Types>
    struct typelist;

    template <typename Typelist>
    struct pop_list;
    template <typename Head, typename ...Tail>
    struct pop_list<typelist<Head, Tail...>> {
      using type = typelist<Tail...>;
    };

    template <typename Typelist>
    struct list_front;
    template <typename Head, typename ...Tail>
    struct list_front<typelist<Head, Tail...>> {
      using type = Head;
    };

    template <typename Typelist, typename T>
    struct push_list;
    template <typename ...Types, typename T>
    struct push_list<typelist<Types...>, T> {
      using type = typelist<Types..., T>;
    };

    template <typename List>
    using pop = typename pop_list<List>::type;
    template <typename List>
    using front = typename list_front<List>::type;
    template <typename List, typename T>
    using push = typename push_list<List, T>::type;

    template <typename T, typename TypeList>
    struct is_in_list;
    template <typename T, typename Head, typename ...Tail>
    struct is_in_list<T, typelist<Head, Tail...>> : is_in_list<T, typelist<Tail...>> {};
    template <typename T, typename ...Tail>
    struct is_in_list<T, typelist<T, Tail...>> : true_type {};
    template <typename T>
    struct is_in_list<T, typelist<>> : false_type {};

    template <typename List, typename UniqueList = typelist<>, typename = false_type>
    struct list_has_no_duplicates : false_type { };
    template <typename List, typename UniqueList>
    struct list_has_no_duplicates<List, UniqueList, typename is_in_list<front<List>, UniqueList>::type>
        : list_has_no_duplicates<pop<List>, push<UniqueList, front<List>>, false_type> { };
    template <typename UniqueList>
    struct list_has_no_duplicates<typelist<>, UniqueList, false_type> : true_type { };

    template <typename ...Types>
    using list_has_no_duplicates_t = list_has_no_duplicates<typelist<Types...>>;


    template <typename ...Types>
    struct transparent_union_is_valid : boolean_value<
                                            all_instantiable<void, valid_tu_member, Types...>::value &&
                                            (sizeof...(Types) > 1) &&
                                            list_has_no_duplicates_t<Types...>::value> { };

  }

  template <typename ...Types>
  class transparent_union_param {
    static_assert(meta::transparent_union_is_valid<Types...>::value, "Each union member type must be castable to and from void*, be exactly 8 bytes, and there must be at least 2 distinct types.");
  public:

    template <typename T, meta::enable_if_one_of_t<T, Types...> = 0>
    transparent_union_param(T value) noexcept
        : m_value((void*)value)
    { }

    template <typename T, meta::enable_if_one_of_t<T, Types...> = 0>
    inline T as() const noexcept {
      return (T)m_value;
    }

  private:
    void* m_value;
  };

#  define AGT_transparent_union(...) ::agtxx::transparent_union_param<__VA_ARGS__>
# else
#  define PP_AGT_impl_TRANSPARENT_UNION_DISPATCH
# endif
}
#elif AGT_has_attribute(transparent_union)
# define PP_AGT_impl_TRANSPARENT_UNION_DISPATCH
# define PP_AGT_impl_TRANSPARENT_UNION_2(a, b) union __attribute__((transparent_union)) { a m_##a; b m_##b; }
# define PP_AGT_impl_TRANSPARENT_UNION_3(a, b, c) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; }
# define PP_AGT_impl_TRANSPARENT_UNION_4(a, b, c, d) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; }
# define PP_AGT_impl_TRANSPARENT_UNION_5(a, b, c, d, e) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; }
# define PP_AGT_impl_TRANSPARENT_UNION_6(a, b, c, d, e, f) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; }
# define PP_AGT_impl_TRANSPARENT_UNION_7(a, b, c, d, e, f, g) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; }
# define PP_AGT_impl_TRANSPARENT_UNION_8(a, b, c, d, e, f, g, h) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; }
# define PP_AGT_impl_TRANSPARENT_UNION_9(a, b, c, d, e, f, g, h, i) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; }
# define PP_AGT_impl_TRANSPARENT_UNION_10(a, b, c, d, e, f, g, h, i, j) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j;  }
# define PP_AGT_impl_TRANSPARENT_UNION_11(a, b, c, d, e, f, g, h, i, j, k) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j; k m_##k; }
# define PP_AGT_impl_TRANSPARENT_UNION_12(a, b, c, d, e, f, g, h, i, j, k, l) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j; k m_##k; l m_##l; }
# define PP_AGT_impl_TRANSPARENT_UNION_13(a, b, c, d, e, f, g, h, i, j, k, l, m) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j; k m_##k; l m_##l; m m_##m; }
# define PP_AGT_impl_TRANSPARENT_UNION_14(a, b, c, d, e, f, g, h, i, j, k, l, m, n) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j; k m_##k; l m_##l; m m_##m; n m_##n; }
# define PP_AGT_impl_TRANSPARENT_UNION_15(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j; k m_##k; l m_##l; m m_##m; n m_##n; o m_##o; }
# define PP_AGT_impl_TRANSPARENT_UNION_16(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j; k m_##k; l m_##l; m m_##m; n m_##n; o m_##o; p m_##p; }
# define PP_AGT_impl_TRANSPARENT_UNION_17(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j; k m_##k; l m_##l; m m_##m; n m_##n; o m_##o; p m_##p; q m_##q; }
# define PP_AGT_impl_TRANSPARENT_UNION_18(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j; k m_##k; l m_##l; m m_##m; n m_##n; o m_##o; p m_##p; q m_##q; r m_##r; }
#else
# define AGT_transparent_union(...) void*
#endif

#endif

#if AGT_has_attribute(transparent_union)
#define AGT_transparent_union_2(a, b) union __attribute__((transparent_union)) { a m_##a; b m_##b; }
#define AGT_transparent_union_3(a, b, c) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; }
#define AGT_transparent_union_4(a, b, c, d) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; }
#define AGT_transparent_union_5(a, b, c, d, e) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; }
#define AGT_transparent_union_6(a, b, c, d, e, f) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; }
#define AGT_transparent_union_7(a, b, c, d, e, f, g) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; }
#define AGT_transparent_union_8(a, b, c, d, e, f, g, h) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; }
#define AGT_transparent_union_9(a, b, c, d, e, f, g, h, i) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; }
#define AGT_transparent_union_10(a, b, c, d, e, f, g, h, i, j) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j;  }
#define AGT_transparent_union_18(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j; k m_##k; l m_##l; m m_##m; n m_##n; o m_##o; p m_##p; q m_##q; r m_##r; }
#else
#define AGT_transparent_union_2(a, b) void*
#define AGT_transparent_union_3(a, b, c) void*
#define AGT_transparent_union_4(a, b, c, d) void*
#define AGT_transparent_union_5(a, b, c, d, e) void*
#define AGT_transparent_union_6(a, b, c, d, e, f) void*
#define AGT_transparent_union_7(a, b, c, d, e, f, g) void*
#define AGT_transparent_union_8(a, b, c, d, e, f, g, h) void*
#define AGT_transparent_union_9(a, b, c, d, e, f, g, h, i) void*
#define AGT_transparent_union_10(a, b, c, d, e, f, g, h, i, j) void*
#define AGT_transparent_union_18(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r) void*
#endif




#define AGT_no_default default: AGT_unreachable

#define AGT_cache_aligned     AGT_alignas(AGT_CACHE_LINE)
#define AGT_page_aligned      AGT_alignas(AGT_PHYSICAL_PAGE_SIZE)


/* =================[ Macro Functions ]================= */


#if AGT_compiler_gcc || AGT_compiler_clang
# define AGT_assume_aligned(p, align) __builtin_assume_aligned(p, align)
#elif AGT_compiler_msvc
# define AGT_assume_aligned(p, align) (AGT_assume(((agt_u64_t)p) % align == 0), p)
#else
# define AGT_assume_aligned(p, align) p
#endif


/* =================[ Constants ]================= */


#define AGT_FALSE              0
#define AGT_TRUE               1

#define AGT_DONT_CARE          ((agt_u64_t)-1)
#define AGT_NULL_HANDLE        ((void*)0)

#define AGT_CACHE_LINE         ((agt_size_t)1 << 6)
#define AGT_PHYSICAL_PAGE_SIZE ((agt_size_t)1 << 12)
#define AGT_VIRTUAL_PAGE_SIZE  ((agt_size_t)(1 << 16))

#define AGT_INVALID_NAME_TOKEN ((agt_name_t)0)
#define AGT_ANONYMOUS ((agt_name_t)0)


#if !defined(AGT_DISABLE_STATIC_STRUCT_SIZES)
# define AGT_ASYNC_STRUCT_SIZE 40
# define AGT_ASYNC_STRUCT_ALIGNMENT 8
# define AGT_SIGNAL_STRUCT_SIZE 24
# define AGT_SIGNAL_STRUCT_ALIGNMENT 8
#endif

#define AGT_OBJECT_PARAMS_BUFFER_SIZE 64
#define AGT_OBJECT_PARAMS_BUFFER_ALIGNMENT 8


#define AGT_INVALID_OBJECT_ID ((agt_object_id_t)-1)
#define AGT_SYNCHRONIZE ((agt_async_t)AGT_NULL_HANDLE)

#define AGT_INVALID_INSTANCE ((agt_instance_t)AGT_NULL_HANDLE)
#define AGT_INVALID_CTX ((agt_ctx_t)AGT_NULL_HANDLE)
#define AGT_DEFAULT_CTX ((agt_ctx_t)AGT_NULL_HANDLE)


#define AGT_NO_EXECUTOR      ((agt_executor_t)AGT_NULL_HANDLE)
#define AGT_DEFAULT_EXECUTOR ((agt_executor_t)1)
#define AGT_EVENT_EXECUTOR   ((agt_executor_t)-1)
#define AGT_BUSY_EXECUTOR    ((agt_executor_t)-2)


#if AGT_system_windows
# define AGT_NATIVE_TIMEOUT_GRANULARITY 100ULL
#else
# define AGT_NATIVE_TIMEOUT_GRANULARITY 1ULL
#endif

#define AGT_TIMEOUT_MS(milliseconds) ((agt_timeout_t)((milliseconds) * (1000000ULL / AGT_NATIVE_TIMEOUT_GRANULARITY)))
#define AGT_TIMEOUT_US(microseconds) ((agt_timeout_t)((microseconds) * (1000ULL / AGT_NATIVE_TIMEOUT_GRANULARITY)))
#define AGT_TIMEOUT_NS(nanoseconds) ((agt_timeout_t)((nanoseconds) / AGT_NATIVE_TIMEOUT_GRANULARITY))
#define AGT_DO_NOT_WAIT ((agt_timeout_t)0)
#define AGT_WAIT ((agt_timeout_t)-1)


#define AGT_NULL_WEAK_REF ((agt_weak_ref_t)0)


#define AGT_VERSION_MAJOR 0
#define AGT_VERSION_MINOR 1
#define AGT_VERSION_PATCH 0
#define AGT_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))
#define AGT_API_VERSION AGT_MAKE_VERSION(AGT_VERSION_MAJOR, AGT_VERSION_MINOR, AGT_VERSION_PATCH)

#define AGT_MIN_VERSION_DONT_CARE 0
#define AGT_MAX_VERSION_DONT_CARE 0xFFFFFFFFu


// Version format:
//                 [    Major   ][    Minor   ][    Patch     ]
//                 [   10 bits  ][   10 bits  ][    12 bits   ]
//                 [   31 - 22  ][   21 - 12  ][    11 - 00   ]
//                 [ 0000000000 ][ 0000000000 ][ 000000000000 ]



AGT_begin_c_namespace


typedef unsigned char            agt_u8_t;
typedef   signed char            agt_i8_t;
typedef          char            agt_char_t;
typedef unsigned short           agt_u16_t;
typedef   signed short           agt_i16_t;
typedef unsigned int             agt_u32_t;
typedef   signed int             agt_i32_t;
typedef unsigned long long       agt_u64_t;
typedef   signed long long       agt_i64_t;



typedef size_t                   agt_size_t;
typedef uintptr_t                agt_address_t;
typedef ptrdiff_t                agt_ptrdiff_t;


typedef agt_u32_t                agt_flags32_t;
typedef agt_u64_t                agt_flags64_t;


typedef agt_i32_t                agt_bool_t;



typedef agt_u64_t                agt_message_id_t;
typedef agt_u64_t                agt_type_id_t;
typedef agt_u64_t                agt_object_id_t;

typedef agt_u64_t                agt_handle_t;

typedef agt_u64_t                agt_send_token_t;

typedef agt_u64_t                agt_instance_id_t;

typedef struct agt_instance_st*  agt_instance_t;
typedef struct agt_ctx_st*       agt_ctx_t;

typedef void*                    agt_async_t;
typedef struct agt_query_t       agt_query_t;


typedef void*                    agt_object_t;

typedef struct agt_signal_st*    agt_signal_t;

typedef struct agt_agent_st*     agt_agent_t;
typedef struct agt_executor_st*  agt_executor_t;




typedef agt_u64_t                agt_duration_t;
typedef agt_duration_t           agt_timeout_t;
typedef agt_u64_t                agt_timestamp_t;






typedef enum agt_status_t {
  AGT_SUCCESS   /** < No errors */,
  AGT_NOT_READY /** < An asynchronous operation is not yet complete */,
  AGT_DEFERRED  /** < An operation has been deferred, and code that requires the completion of the operation must wait on the async handle provided */,
  AGT_CANCELLED /** < An asynchronous operation has been cancelled. This will only be returned if an operation is cancelled before it is complete*/,
  AGT_TIMED_OUT /** < A wait operation exceeded the specified timeout duration */,
  AGT_INCOMPLETE_MESSAGE,
  AGT_ERROR_UNKNOWN,
  AGT_ERROR_ALREADY_EXISTS,
  AGT_ERROR_CTX_NOT_ACQUIRED,
  AGT_ERROR_UNKNOWN_FOREIGN_OBJECT,
  AGT_ERROR_INVALID_OBJECT_ID,
  AGT_ERROR_EXPIRED_OBJECT_ID,
  AGT_ERROR_NULL_HANDLE,
  AGT_ERROR_INVALID_HANDLE_TYPE,
  AGT_ERROR_NOT_BOUND,
  AGT_ERROR_FOREIGN_SENDER,
  AGT_ERROR_STATUS_NOT_SET,
  AGT_ERROR_AT_CAPACITY,
  AGT_ERROR_UNKNOWN_MESSAGE_TYPE,
  AGT_ERROR_INVALID_CMD /** < The given handle does not implement the called function. eg. agt_receive_message cannot be called on a channel sender*/,
  AGT_ERROR_INVALID_FLAGS,
  AGT_ERROR_INVALID_MESSAGE,
  AGT_ERROR_INVALID_SIGNAL,
  AGT_ERROR_CANNOT_DISCARD,
  AGT_ERROR_MESSAGE_TOO_LARGE,
  AGT_ERROR_INSUFFICIENT_SLOTS,
  AGT_ERROR_NOT_MULTIFRAME,
  AGT_ERROR_BAD_SIZE,
  AGT_ERROR_INVALID_ARGUMENT,
  AGT_ERROR_AGENT_IS_DETACHED,
  AGT_ERROR_BAD_UTF8_ENCODING,
  AGT_ERROR_BAD_NAME,
  AGT_ERROR_NAME_TOO_LONG,
  AGT_ERROR_NAME_ALREADY_IN_USE,
  AGT_ERROR_BAD_DISPATCH_KIND,
  AGT_ERROR_BAD_ALLOC,
  AGT_ERROR_MAILBOX_IS_FULL,
  AGT_ERROR_MAILBOX_IS_EMPTY,
  AGT_ERROR_TOO_MANY_SENDERS,
  AGT_ERROR_NO_SENDERS,
  AGT_ERROR_PARTNER_DISCONNECTED,
  AGT_ERROR_CONNECTION_DROPPED,
  AGT_ERROR_TOO_MANY_RECEIVERS,
  AGT_ERROR_NO_RECEIVERS,
  AGT_ERROR_ALREADY_RECEIVED,
  AGT_ERROR_INITIALIZATION_FAILED,
  AGT_ERROR_CANNOT_CREATE_SHARED,
  AGT_ERROR_NOT_YET_IMPLEMENTED,
  AGT_ERROR_CORRUPTED_MESSAGE,
  AGT_ERROR_COULD_NOT_REACH_ALL_TARGETS,
  AGT_ERROR_INTERNAL_OVERFLOW,            ///< Indicates an internal overflow error that was caught and corrected but that caused the requested operation to fail.
  AGT_ERROR_OBJECT_IS_BUSY,               ///< Indicates that the requested operation failed because the receiving object is doing something else at the moment.

  AGT_ERROR_ALREADY_COMPLETED,

  AGT_ERROR_UNKNOWN_ATTRIBUTE,
  AGT_ERROR_INVALID_ATTRIBUTE_VALUE,
  AGT_ERROR_INVALID_ENVVAR_VALUE,
  AGT_ERROR_MODULE_NOT_FOUND,
  AGT_ERROR_NOT_A_DIRECTORY,
  AGT_ERROR_CORRUPT_MODULE,
  AGT_ERROR_BAD_MODULE_VERSION,

  AGT_ERROR_NO_FCTX,              ///< Returned by a procedure that must only be called from within a fiber if it is called from outside of a fiber.
  AGT_ERROR_ALREADY_FIBER,        ///< Returned by agt_enter_efiber if the calling context is already executing in a fiber.
  AGT_ERROR_IN_AGENT_CONTEXT,     ///< Returned by procedures that can only be called outside of an Agent Execution Context are called by an agent.
  AGT_ERROR_FCTX_EXCEPTION,       ///< Returned by agt_enter_fctx when the fctx undergoes abnormal termination (most frequently by a call to agt_exit_fctx with an exit code other than 0)
} agt_status_t;

/*typedef struct agt_batch_status_t {
  agt_bool_t      complete;
  size_t          successCount;
  size_t          errorCount;
} agt_batch_status_t;*/

typedef enum agt_error_handler_status_t {
  AGT_ERROR_HANDLED,
  AGT_ERROR_IGNORED,
  AGT_ERROR_NOT_HANDLED
} agt_error_handler_status_t;

typedef enum agt_scope_t {
  AGT_CTX_SCOPE       = 0x1, ///< Visible to (and intended for use by) only the current context. Generally implies the total absence of any concurrency protection (ie no locks, no atomic operations, etc). Roughly analogous to "thread local" scope.
  AGT_INSTANCE_SCOPE  = 0x2, ///< Visible to any context created by the current instance. Roughly analogous to "process local" scope.
  AGT_SHARED_SCOPE    = 0x4  ///< Visible to any context created by any instance attached to the same shared instance as the current instance. Roughly analogous to "system" scope.
} agt_scope_t;
typedef agt_flags32_t agt_scope_mask_t; // Some bitwise-or combination of agt_scope_t values


typedef enum agt_priority_t {
  AGT_PRIORITY_BACKGROUND = -3,
  AGT_PRIORITY_VERY_LOW   = -2,
  AGT_PRIORITY_LOW        = -1,
  AGT_PRIORITY_NORMAL     = 0,
  AGT_PRIORITY_HIGH       = 1,
  AGT_PRIORITY_VERY_HIGH  = 2,
  AGT_PRIORITY_CRITICAL   = 3
} agt_priority_t;



// Specifying this enum as a bit mask allows these values to be used to easily filter operations by type
typedef enum agt_object_type_t {
  AGT_AGENT_TYPE           = 0x1,
  AGT_QUEUE_TYPE           = 0x2,
  AGT_BQUEUE_TYPE          = 0x4,
  AGT_SENDER_TYPE          = 0x8,
  AGT_RECEIVER_TYPE        = 0x10,
  AGT_POOL_TYPE            = 0x20,
  AGT_RCPOOL_TYPE          = 0x40,
  AGT_SIGNAL_TYPE          = 0x80,
  AGT_USER_ALLOCATION_TYPE = 0x100,
  AGT_SHEAP_TYPE           = 0x200,
  AGT_SHPOOL_TYPE          = 0x400,
  AGT_SHMEM_TYPE           = 0x800,
  AGT_EXECUTOR_TYPE        = 0x1000,
  AGT_UNKNOWN_TYPE         = (-0x7FFFFFFF) - 1 /// Specifying 0x80000000 is implementation defined because the underlying integer type is int, which cannot represent the positive value 0x80000000.
} agt_object_type_t;
typedef agt_flags32_t agt_object_type_mask_t;
#define AGT_ANY_TYPE 0x7FFFFFFF


typedef struct agt_string_t {
  const char* data;
  size_t      length;
} agt_string_t;



typedef struct agt_object_desc_t {
  void*               object;
  agt_object_type_t   type;
  agt_scope_t         scope;
  const agt_string_t* name;
  const void*         params; ///< Depends on type
} agt_object_desc_t;
typedef struct AGT_alignas(AGT_OBJECT_PARAMS_BUFFER_ALIGNMENT) agt_object_params_buffer_t {
  agt_u8_t reserved[AGT_OBJECT_PARAMS_BUFFER_SIZE];
} agt_object_params_buffer_t;



typedef agt_error_handler_status_t (AGT_stdcall *agt_error_handler_t)(agt_status_t errorCode, void* errorData);

typedef void (AGT_stdcall* agt_internal_log_handler_t)(const struct agt_internal_log_info_t* logInfo);




typedef void (* AGT_stdcall agt_proc_t)(void);

/// Attribute Types

typedef enum agt_value_type_t {
  AGT_TYPE_UNKNOWN,
  AGT_TYPE_BOOLEAN,
  AGT_TYPE_ADDRESS,
  AGT_TYPE_STRING,
  AGT_TYPE_WIDE_STRING,
  AGT_TYPE_UINT32,
  AGT_TYPE_INT32,
  AGT_TYPE_UINT64,
  AGT_TYPE_INT64,
  AGT_TYPE_FLOAT32,
  AGT_TYPE_FLOAT64,
  AGT_VALUE_TYPE_COUNT        ///< The number of type enums
} agt_value_type_t;

typedef union agt_value_t {
  agt_bool_t     boolean;
  const void*    address;
  const char*    string;
  const wchar_t* wideString;
  agt_u32_t      uint32;
  agt_i32_t      int32;
  agt_u64_t      uint64;
  agt_i64_t      int64;
  float          float32;
  double         float64;
} agt_value_t;

typedef enum agt_attr_id_t {
  AGT_ATTR_ASYNC_STRUCT_SIZE,              ///< type: UINT32
  AGT_ATTR_THREAD_COUNT,                   ///< type: UINT32
  AGT_ATTR_LIBRARY_PATH,                   ///< type: STRING or WIDE_STRING
  AGT_ATTR_LIBRARY_VERSION,                ///< type: INT32 or INT32_RANGE
  AGT_ATTR_SHARED_CONTEXT,                 ///< type: BOOLEAN
  AGT_ATTR_CXX_EXCEPTIONS_ENABLED,         ///< type: BOOLEAN
  AGT_ATTR_SHARED_NAMESPACE,               ///< type: STRING or WIDE_STRING
  AGT_ATTR_CHANNEL_DEFAULT_CAPACITY,       ///< type: UINT32
  AGT_ATTR_CHANNEL_DEFAULT_MESSAGE_SIZE,   ///< type: UINT32
  AGT_ATTR_CHANNEL_DEFAULT_TIMEOUT_MS,     ///< type: UINT32
  AGT_ATTR_DURATION_UNIT_SIZE_NS,          ///< type: UINT64
  AGT_ATTR_NATIVE_DURATION_UNIT_SIZE_NS,   ///< type: UINT64
  AGT_ATTR_FIXED_CHANNEL_SIZE_GRANULARITY, ///< type: UINT64
  AGT_ATTR_STACK_SIZE_ALIGNMENT,           ///< type: UINT64
  AGT_ATTR_DEFAULT_THREAD_STACK_SIZE,      ///< type: UINT64
  AGT_ATTR_DEFAULT_FIBER_STACK_SIZE,       ///< type: UINT64
  AGT_ATTR_FULL_STATE_SAVE_MASK,           ///< type: UINT64
  AGT_ATTR_MAX_STATE_SAVE_SIZE,            ///< type: UINT32
} agt_attr_id_t;

typedef struct agt_attr_t {
  agt_attr_id_t    id;
  agt_value_type_t type;
  agt_value_t      value;
} agt_attr_t;



/// Name Types

typedef agt_u64_t agt_name_t;



/// Pool Types


typedef struct agt_pool_st*     agt_pool_t;
typedef struct agt_rcpool_st*   agt_rcpool_t;


typedef agt_u64_t               agt_weak_ref_t;
typedef agt_u32_t               agt_epoch_t;


typedef enum agt_pool_flag_bits_t {
  AGT_POOL_IS_THREAD_SAFE = 0x1
} agt_pool_flag_bits_t;
typedef agt_flags32_t agt_pool_flags_t;

typedef enum agt_weak_ref_flag_bits_t {
  AGT_WEAK_REF_RETAIN_IF_ACQUIRED = 0x1
} agt_weak_ref_flag_bits_t;
typedef agt_flags32_t agt_weak_ref_flags_t;



typedef struct agt_allocator_params_t {
  size_t               structSize;              ///< equals: sizeof(agt_allocator_params_t)
  const agt_size_t*    blockSizes;              ///< length: blockSizeCount
  const agt_size_t*    blocksPerChunk;          ///< length: blockSizeCount
  size_t               blockSizeCount;
  const agt_size_t (*  partneredBlockSizes)[2]; ///< length: partneredBlockSizeCount
  size_t               partneredBlockSizeCount;
  const agt_size_t*    soloBlockSizes;          ///< length: soloBlockSizeCount
  size_t               soloBlockSizeCount;
  size_t               maxChunkSize;
} agt_allocator_params_t;



#define AGT_COMMON_FLAG_BIT(x) ((x) << 16)

/**
 * \brief Flags that are common to a number of API calls.
 *
 * \details agt_common_flag_bits_t is not used anywhere directly,
 *          but rather provides a set of flags that are used in the
 *          exact same capacity across a set of API calls. This is
 *          intended to help simplify the implementation and
 *          increase cache performance by allowing common
 *          subroutines to all share the same code.
 *          Bits [31:16] are reserved for common flags.
 *          A given flag type that requires more than 16 bits may
 *          use bits [31:16] if the routines for which the
 *          flag type is specified do not take any common flags,
 *          and will not in the future.
 *
 * \note For a flag to be considered "common", it should be used
 *       across at least 3 distinct API calls, and is required to
 *       have the exact same semantics relative to the dispatch
 *       type of the call.
 * */
// typedef enum agt_common_flag_bits_t {
#define AGT_ASYNC_IS_UNINITIALIZED 0x10000//, ///< Indicates that the async handle specified has not yet been initialized; If the async handle is required, it will be initialized before use. If not required, it will remain uninitialized. This helps prevent spurious/unnecessary overhead
#define AGT_ONE_AND_DONE           0x20000//, ///< Indicates that the specified object should be automatically destroyed/released after its first use. What exactly constitutes "destroy/release" and "use" is specified in the documentation of any call that takes this flag.
// } agt_common_flag_bits_t;


/** ===============[ Core Static API Functions ]=================== **/


/**
 * @param [optional] context If context is AGT_INVALID_CTX,
 * */
AGT_static_api agt_instance_t      AGT_stdcall agt_get_instance(agt_ctx_t context) AGT_noexcept;


/**
 * Returns an integer value that uniquely identifies the library instance (essentially, the library loaded by the current process; this is effectively a portable process identifier)
 */
AGT_static_api agt_instance_id_t   AGT_stdcall agt_get_instance_id() AGT_noexcept;






/**
 * Returns the API version of the linked library.
 * */
AGT_static_api int                 AGT_stdcall agt_get_instance_version(agt_instance_t instance) AGT_noexcept;


AGT_static_api int                 AGT_stdcall agt_get_static_version() AGT_noexcept;


AGT_static_api agt_error_handler_t AGT_stdcall agt_get_error_handler(agt_instance_t instance) AGT_noexcept;

AGT_static_api agt_error_handler_t AGT_stdcall agt_set_error_handler(agt_instance_t instance, agt_error_handler_t errorHandlerCallback) AGT_noexcept;


AGT_static_api agt_bool_t          AGT_stdcall agt_query_attributes(size_t attrCount, const agt_attr_id_t* pAttrId, agt_value_type_t* pTypes, agt_value_t* pValues) AGT_noexcept;

AGT_static_api agt_status_t        AGT_stdcall agt_query_instance_attributes(agt_instance_t instance, agt_attr_t* pAttributes, size_t attributeCount) AGT_noexcept;




/** ===============[ Core API Functions ]=================== **/


/**
 * Returns the thread local agate context.
 *
 * @result A handle to the thread local library context.
 * If the calling thread has not already acquired a context, this returns null.
 *
 * This is the preferred method of retrieving the context object when it is known that
 * one already exists in the current thread, as it avoids the overhead of having to do any checks.
 *
 * This function may be used to query whether or not the current thread has initialized a context already,
 *
 * */
AGT_core_api agt_ctx_t AGT_stdcall agt_ctx() AGT_noexcept;

/**
 * Attempts to create a new context in the local thread with
 * optional parameters for initializing the context allocator.
 *
 * If the current thread has already acquired a context, it is returned instead.
 * Otherwise, a new context is created and attached to the current thread.
 *
 * If a new context is created, and pAllocParams is not null, the parameters
 * it points to will be used to initialize the context allocator. This is primarily
 * intended as an optimization feature, whereby application authors may profile the allocation behaviour of
 * certain contexts, and provide allocator parameters based on the expected behaviour of the context.
 *
 */
AGT_core_api agt_ctx_t AGT_stdcall agt_acquire_ctx(const agt_allocator_params_t* pAllocParams) AGT_noexcept;




/**
 * pDstObject should be a pointer to an object that is the real type of srcObject.
 *
 * Example:
 *
 * agt_agent_t clone_agent(agt_agent_t src) {
 *   agt_agent_t dst;
 *   agt_dup(src, (agt_object_t*)&dst, 1);
 *   return dst;
 * }
 *
 * std::vector<agt_agent_t> multiclone_agent(agt_agent_t src, size_t count) {
 *   std::vector<agt_agent_t> agentArray;
 *   agentArray.resize(count);
 *   agt_dup(src, (agt_object_t*)agentArray.data(), count);
 *   return agentArray; // C++17 and beyond guarantees copy ellision here, but in C++11 or C++14, we would want to std::move the vector
 * }
 * */
AGT_core_api agt_status_t AGT_stdcall agt_dup(agt_object_t srcObject, agt_object_t* pDstObject, size_t dstObjectCount) AGT_noexcept;

AGT_core_api void         AGT_stdcall agt_close(void* object) AGT_noexcept;



/**
 * Imports an object from a generic, sharable handle.
 *
 * \note While the handles API is a part of the core module, and is thus always
 *       available, it's generally not worth using when ctx is known to be private
 *       (ie. not shared).
 *       The value of a handle of an object with instance local (ie. context or
 *       instance ) scope is little more than the address at which it is located
 *       in the local address space. As such, when passing instance local objects
 *       around within the local instance, one may simply pass the objects themselves.
 * */
AGT_core_api agt_status_t AGT_stdcall agt_import_handle(agt_ctx_t ctx, agt_handle_t handle, agt_object_type_mask_t typeMask, agt_object_desc_t* pObjectDesc) AGT_noexcept;

AGT_core_api agt_status_t AGT_stdcall agt_export_handle(agt_ctx_t ctx, agt_object_t object, agt_handle_t* pHandle) AGT_noexcept;


/**
 * Returns an opaque, platform dependent timestamp that has the following properties:
 *  - Monotonic: It increases as time advances; If timestamp A is acquired before timestamp B,
 *               the raw value of B will be greater than or equal to A. Timestamps can thus be
 *               compared and sorted with respect to value.
 *  - Process Local: Agate timestamp values only remain coherent within a given process;
 *                   timestamps shared across process boundaries are invalid and do not
 *                   contain meaningful values.
 *
 * The only way to convert these values into anything meaningful is with agt_get_timespec.
 * */
AGT_core_api agt_timestamp_t AGT_stdcall agt_now(agt_ctx_t ctx) AGT_noexcept;

/**
 * Fills the timespec struct (not defined in this library, and is rather provided by either libc or the STL in the <time.h> or <ctime> header files respectively)
 * with the time elapsed between the UNIX epoch and the time at which the given timestamp was acquired.
 * To either interpret timestamps, or convert to a persistent value, clients should call agt_get_timespec.
 * */
AGT_core_api void            AGT_stdcall agt_get_timespec(agt_ctx_t ctx, agt_timestamp_t timestamp, struct timespec* ts) AGT_noexcept;



/**
 * Closes the provided context. Behaviour of this function depends on how the library was configured
 *
 * TODO: Decide whether to provide another API call with differing behaviour depending on whether or not
 *       one wishes to wait for processing to finish.
 * */
AGT_core_api agt_status_t        AGT_stdcall agt_finalize(agt_ctx_t context) AGT_noexcept;



AGT_static_api agt_proc_t   AGT_stdcall agt_get_proc_address(const char* symbol) AGT_noexcept;


AGT_end_c_namespace


#endif//AGATE_CORE_H
