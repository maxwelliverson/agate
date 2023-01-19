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


#if defined(_MSC_VER) || defined(__MINGW32__)
# if !defined(AGT_SHARED_LIB)
#  define AGT_api
# elif defined(AGT_SHARED_LIB_EXPORT)
#  define AGT_api __declspec(dllexport)
# else
#  define AGT_api __declspec(dllimport)
# endif
#
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
# define AGT_api          __attribute__((visibility("default")))
# define AGT_restricted
# define AGT_malloc       __attribute__((malloc))
# define AGT_noinline     __attribute__((noinline))
# define AGT_noalias
# define AGT_thread_local __thread
# define AGT_alignas(n)   __attribute__((aligned(n)))
# define AGT_assume(...)
# define AGT_forceinline  __attribute__((always_inline))
# define AGT_unreachable  __builtin_unreachable()
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
# define AGT_api
# define AGT_may_alias
# define AGT_restricted
# define AGT_malloc
# define AGT_alloc_size(...)
# define AGT_alloc_align(p)
# define AGT_noinline
# define AGT_noalias
# define AGT_forceinline
# define AGT_thread_local            __thread        // hope for the best :-)
# define AGT_alignas(n)
# define AGT_assume(...)
# define AGT_unreachable AGT_assume(false)
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


#define AGT_agent_api AGT_api


/* =================[ Macro Functions ]================= */


#if AGT_compiler_gcc || AGT_compiler_clang
# define AGT_assume_aligned(p, align) __builtin_assume_aligned(p, align)
#elif AGT_compiler_msvc
# define AGT_assume_aligned(p, align) (AGT_assume(((agt_u64_t)p) % align == 0), p)
#else
# define AGT_assume_aligned(p, align) p
#endif



#define AGT_NAME(name) ((agt_name_t){ .data = name, .length = ((sizeof name) - 1) })




/* =================[ Constants ]================= */


#define AGT_DONT_CARE          ((agt_u64_t)-1)
#define AGT_NULL_HANDLE        ((void*)0)

#define AGT_CACHE_LINE         ((agt_size_t)1 << 6)
#define AGT_PHYSICAL_PAGE_SIZE ((agt_size_t)1 << 12)
#define AGT_VIRTUAL_PAGE_SIZE  ((agt_size_t)(1 << 16))

#define AGT_INVALID_NAME_TOKEN ((agt_name_token_t)0)


#if !defined(AGT_DISABLE_STATIC_STRUCT_SIZES)
# define AGT_ASYNC_STRUCT_SIZE 40
# define AGT_ASYNC_STRUCT_ALIGNMENT 8
# define AGT_SIGNAL_STRUCT_SIZE 24
# define AGT_SIGNAL_STRUCT_ALIGNMENT 8
#endif


#define AGT_INVALID_OBJECT_ID ((agt_object_id_t)-1)
#define AGT_SYNCHRONIZE ((agt_async_t)AGT_NULL_HANDLE)



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

typedef agt_u64_t                agt_send_token_t;


typedef struct agt_instance_st*  agt_instance_t;
typedef struct agt_ctx_st*       agt_ctx_t;

typedef struct agt_async_t       agt_async_t;
typedef struct agt_signal_t      agt_signal_t;

typedef struct agt_agent_st*     agt_agent_t;

typedef struct agt_namespace_st* agt_namespace_t;




typedef agt_u64_t               agt_duration_t;
typedef agt_duration_t          agt_timeout_t;
typedef agt_u64_t               agt_timestamp_t;






typedef enum agt_status_t {
  AGT_SUCCESS   /** < No errors */,
  AGT_NOT_READY /** < An asynchronous operation is not yet complete */,
  AGT_DEFERRED  /** < An operation has been deferred, and code that requires the completion of the operation must wait on the async handle provided */,
  AGT_CANCELLED /** < An asynchronous operation has been cancelled. This will only be returned if an operation is cancelled before it is complete*/,
  AGT_TIMED_OUT /** < A wait operation exceeded the specified timeout duration */,
  AGT_INCOMPLETE_MESSAGE,
  AGT_ERROR_UNKNOWN,
  AGT_ERROR_UNKNOWN_FOREIGN_OBJECT,
  AGT_ERROR_INVALID_OBJECT_ID,
  AGT_ERROR_EXPIRED_OBJECT_ID,
  AGT_ERROR_NULL_HANDLE,
  AGT_ERROR_INVALID_HANDLE_TYPE,
  AGT_ERROR_NOT_BOUND,
  AGT_ERROR_FOREIGN_SENDER,
  AGT_ERROR_STATUS_NOT_SET,
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
  AGT_ERROR_RESERVATION_FAILED,
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
  AGT_ERROR_TOO_MANY_RECEIVERS,
  AGT_ERROR_NO_RECEIVERS,
  AGT_ERROR_ALREADY_RECEIVED,
  AGT_ERROR_INITIALIZATION_FAILED,
  AGT_ERROR_CANNOT_CREATE_SHARED,
  AGT_ERROR_NOT_YET_IMPLEMENTED,
  AGT_ERROR_CORRUPTED_MESSAGE,
  AGT_ERROR_COULD_NOT_REACH_ALL_TARGETS,
  AGT_ERROR_INTERNAL_OVERFLOW,            /** < Indicates an internal overflow error that was caught and corrected but that caused the requested operation to fail. */

  AGT_ERROR_INVALID_ATTRIBUTE_VALUE,
  AGT_ERROR_INVALID_ENVVAR_VALUE,
  AGT_ERROR_MODULE_NOT_FOUND,
  AGT_ERROR_NOT_A_DIRECTORY,
  AGT_ERROR_CORRUPT_MODULE,
  AGT_ERROR_BAD_MODULE_VERSION

} agt_status_t;

typedef enum agt_error_handler_status_t {
  AGT_ERROR_HANDLED,
  AGT_ERROR_IGNORED,
  AGT_ERROR_NOT_HANDLED
} agt_error_handler_status_t;

typedef enum agt_scope_t {
  AGT_CTX_SCOPE,       ///< Visible to (and intended for use by) only the current context. Generally implies the total absence of any concurrency protection (ie no locks, no atomic operations, etc). Roughly analogous to "thread local" scope.
  AGT_INSTANCE_SCOPE,  ///< Visible to any context created by the current instance. Roughly analogous to "process local" scope.
  AGT_SHARED_SCOPE     ///< Visible to any context created by any instance attached to the same shared instance as the current instance. Roughly analogous to "system" scope.
} agt_scope_t;

typedef enum agt_object_type_t {
  AGT_AGENT_TYPE,
  AGT_QUEUE_TYPE,
  AGT_BQUEUE_TYPE,
  AGT_SENDER_TYPE,
  AGT_RECEIVER_TYPE,
  AGT_POOL_TYPE,
  AGT_RCPOOL_TYPE,
  AGT_SIGNAL_TYPE,
  AGT_USER_ALLOCATION_TYPE,
  AGT_UNKNOWN_TYPE
} agt_object_type_t;




typedef agt_error_handler_status_t (AGT_stdcall *agt_error_handler_t)(agt_status_t errorCode, void* errorData);

typedef void (AGT_stdcall* agt_internal_log_handler_t)(const struct agt_internal_log_info_t* logInfo);



/// Attribute Types

typedef enum agt_attr_type_t {
  AGT_ATTR_TYPE_BOOLEAN,
  AGT_ATTR_TYPE_STRING,
  AGT_ATTR_TYPE_WIDE_STRING,
  AGT_ATTR_TYPE_UINT32,
  AGT_ATTR_TYPE_INT32,
  AGT_ATTR_TYPE_UINT64,
  AGT_ATTR_TYPE_INT64
} agt_attr_type_t;

typedef enum agt_attr_id_t {
  AGT_ATTR_ASYNC_STRUCT_SIZE,                  ///< type: UINT32
  AGT_ATTR_SIGNAL_STRUCT_SIZE,                 ///< type: UINT32
  AGT_ATTR_LIBRARY_PATH,                       ///< type: STRING or WIDE_STRING
  AGT_ATTR_LIBRARY_VERSION,                    ///< type: INT32 or INT32_RANGE
  AGT_ATTR_SHARED_CONTEXT,                     ///< type: BOOLEAN
  AGT_ATTR_SHARED_NAMESPACE,                   ///< type: STRING or WIDE_STRING
  AGT_ATTR_CHANNEL_DEFAULT_CAPACITY,           ///< type: UINT32
  AGT_ATTR_CHANNEL_DEFAULT_MESSAGE_SIZE,       ///< type: UINT32
  AGT_ATTR_CHANNEL_DEFAULT_TIMEOUT_MS,         ///< type: UINT32
  AGT_ATTR_DURATION_UNIT_SIZE_NS,              ///< type: UINT64
  AGT_ATTR_MIN_FIXED_CHANNEL_SIZE_GRANULARITY, ///< type: UINT64
} agt_attr_id_t;

typedef struct agt_attr_t {
  agt_attr_id_t   id;
  agt_attr_type_t type;
  union {
    const void*  ptr;
    agt_bool_t   boolean;
    agt_u32_t    u32;
    agt_i32_t    i32;
    agt_u64_t    u64;
    agt_i64_t    i64;
    struct {
      agt_i32_t min;
      agt_i32_t max;
    } i32range;
    struct {
      agt_u32_t min;
      agt_u32_t max;
    } u32range;
  };
} agt_attr_t;



/// Naming Types

typedef agt_u64_t               agt_name_token_t;

typedef struct agt_name_t {
  const char* data;   ///< Name data
  agt_size_t  length; ///< Name length; optional. If 0, data must be null terminated.
} agt_name_t;

typedef struct agt_name_binding_info_t {
  void*             object;
  agt_object_type_t type;
  agt_scope_t       scope;
  const void*       params;
} agt_name_binding_info_t;

typedef struct agt_reservation_desc_t {
  agt_name_t      name;      ///< Encoded in utf8
  agt_scope_t     scope;     ///< As always, AGT_SHARED_SCOPE is only valid if AGT_ATTR_SHARED_CONTEXT is true
  agt_namespace_t nameSpace; ///< [optional] "namespace" is a reserved identifier in C++, so we abuse camelCase.
  agt_async_t*    async;     ///< [optional]
} agt_reservation_desc_t;

typedef union agt_reserve_name_result_t {
  agt_name_token_t               token;
  const agt_name_binding_info_t* bindingInfo;
} agt_reserve_name_result_t;


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






/** ===============[ Core API Functions ]=================== **/

AGT_api agt_instance_t      AGT_stdcall agt_get_instance(agt_ctx_t context) AGT_noexcept;

AGT_api agt_ctx_t           AGT_stdcall agt_current_context() AGT_noexcept;

/**
 * Returns the API version of the linked library.
 * */
AGT_api int                 AGT_stdcall agt_get_library_version(agt_instance_t instance) AGT_noexcept;



AGT_api agt_error_handler_t AGT_stdcall agt_get_error_handler(agt_instance_t instance) AGT_noexcept;

AGT_api agt_error_handler_t AGT_stdcall agt_set_error_handler(agt_instance_t instance, agt_error_handler_t errorHandlerCallback) AGT_noexcept;


/**
 * \brief Reserves a name to be subsequently bound to some agate object. On success,
 *        an opaque name token is returned. In the case of a name-clash, some basic
 *        info about the bound object is returned instead.
 *
 * \details This is primarily intended to allow for early detection of name-clash errors
 *          during object construction, and for enabling lazy construction of unique
 *          objects.
 *          In pseudocode, the intended idiom looks something along the lines of
 *
 *              reserve(name)
 *              error = initialize(state)
 *              if error
 *                  release(name)
 *              else
 *                  error = agt_create_some_object(state, name, ...)
 *                  if error
 *                      release(name)
 *
 *          This is particularly useful when object creation is potentially expensive, or
 *          when the named object should be unique within the specified scope. It may also
 *          be used to retroactively name objects that were created anonymously.
 *
 * \returns \n
 *              AGT_SUCCESS: Successfully reserved the specified name within the specified scope,
 *                           and a token has been written to pResult->token that may be subsequently
 *                           bound to some object. \n
 *              AGT_DEFERRED: The specified name has already been reserved, but has not yet been
 *                            bound. A deferred operation has been bound to pReservationDesc->async,
 *                            which will reattempt the reservation if the name is released, or will
 *                            return the binding info the bound object upon binding. \n
 *              AGT_ERROR_RESERVATION_FAILED: The specified name has already been reserved, but has
 *                                            not yet been bound, and the operation could not be
 *                                            deferred because pReservationDesc->async was null. \n
 *              AGT_ERROR_INVALID_ARGUMENT: Indicates either pReservationDesc or pResult was null,
 *                                          which ideally, shouldn't ever be the case. \n
 *              AGT_ERROR_NAME_ALREADY_IN_USE: The requested name has already been bound within the
 *                                             specified scope, and a pointer to a struct containing
 *                                             information about the bound object has been written to
 *                                             pResult->bindingInfo. \n
 *              AGT_ERROR_BAD_UTF8_ENCODING: The requested name was not valid UTF8. \n
 *              AGT_ERROR_NAME_TOO_LONG: The requested name exceeded the maximum name length limit.
 *                                       The maximum name length may be queried with agt_query_attributes. \n
 *              AGT_ERROR_BAD_NAME: The requested name contained characters not allowed in a valid agate
 *                                  name. As of right now, there aren't any, but that may change in the
 *                                  future.
 *
 * */
AGT_api agt_status_t        AGT_stdcall agt_reserve_name(agt_ctx_t ctx, const agt_reservation_desc_t* pReservationDesc, agt_reserve_name_result_t* pResult) AGT_noexcept;

/**
 * \brief Releases a name previously reserved by a call to agt_reserve_name.
 *
 * \details This only needs to be called when a previously reserved name will not be
 *          bound to anything. Primarily intended for use in cases where a name is
 *          reserved prior to object creation, and then at some point during
 *          construction, an error is encountered and the creation routine is
 *          cancelled.
 *          Note that attempts to bind a name token (eg. agt_bind_name, agt_new_agent, ...)
 *          that result in failure do NOT automatically release the reserved name. This
 *          is so that the name token may be reused in subsequent attempts, but it also
 *          means that if a user wishes to propagate that failure upwards, they MUST
 *          release the reserved name token. Failure to do so results in a memory leak.
 *
 * \note nameToken must be a valid token obtained from a call to agt_reserve_name that
 *       has not yet been bound to anything.
 * */
AGT_api void                AGT_stdcall agt_release_name(agt_ctx_t ctx, agt_name_token_t nameToken) AGT_noexcept;

/**
 * \brief Binds an object to a previously reserved name.
 *
 * \note nameToken must have been previously obtained from a call to agt_reserve_name,
 *       and must not have already been bound.
 * */
AGT_api agt_status_t        AGT_stdcall agt_bind_name(agt_ctx_t ctx, agt_name_token_t nameToken, void* object) AGT_noexcept;



AGT_end_c_namespace


#endif//AGATE_CORE_H
