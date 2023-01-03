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


#define AGT_ASYNC_STRUCT_SIZE 40
#define AGT_ASYNC_STRUCT_ALIGNMENT 8
#define AGT_SIGNAL_STRUCT_SIZE 24
#define AGT_SIGNAL_STRUCT_ALIGNMENT 8


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

// Version format:
//                 [    Major   ][    Minor   ][    Patch     ]
//                 [   10 bits  ][   10 bits  ][    12 bits   ]
//                 [   31 - 22  ][   21 - 12  ][    11 - 00   ]
//                 [ 0000000000 ][ 0000000000 ][ 000000000000 ]




/* =================[ Types ]================= */

AGT_begin_c_namespace


typedef unsigned char       agt_u8_t;
typedef   signed char       agt_i8_t;
typedef          char       agt_char_t;
typedef unsigned short      agt_u16_t;
typedef   signed short      agt_i16_t;
typedef unsigned int        agt_u32_t;
typedef   signed int        agt_i32_t;
typedef unsigned long long  agt_u64_t;
typedef   signed long long  agt_i64_t;



typedef size_t              agt_size_t;
typedef uintptr_t           agt_address_t;
typedef ptrdiff_t           agt_ptrdiff_t;


typedef agt_u32_t           agt_flags32_t;
typedef agt_u64_t           agt_flags64_t;


typedef agt_i32_t           agt_bool_t;

typedef agt_u64_t           agt_timeout_t;

typedef agt_u64_t           agt_message_id_t;
typedef agt_u64_t           agt_type_id_t;
typedef agt_u64_t           agt_object_id_t;

typedef agt_u64_t           agt_send_token_t;
typedef agt_u64_t           agt_name_token_t;

typedef struct agt_ctx_st*  agt_ctx_t;
typedef struct agt_async_t  agt_async_t;
typedef struct agt_signal_t agt_signal_t;


typedef struct agt_pool_st*   agt_pool_t;
typedef struct agt_rcpool_st* agt_rcpool_t;

typedef agt_u64_t agt_weak_ref_t;
typedef agt_u32_t agt_epoch_t;



typedef enum agt_error_handler_status_t {
  AGT_ERROR_HANDLED,
  AGT_ERROR_IGNORED,
  AGT_ERROR_NOT_HANDLED
} agt_error_handler_status_t;

typedef enum agt_scope_t {
  AGT_LOCAL_SCOPE,
  AGT_SHARED_SCOPE,
  AGT_PRIVATE_SCOPE
} agt_scope_t;

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
  AGT_ERROR_BAD_ENCODING_IN_NAME,
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


typedef struct agt_name_t {
  const char* data;   ///< Name data
  agt_size_t  length; ///< Name length; optional. If 0, data must be null terminated.
} agt_name_t;


typedef agt_error_handler_status_t (AGT_stdcall *agt_error_handler_t)(agt_status_t errorCode, void* errorData);



typedef enum agt_pool_flag_bits_t {
  AGT_POOL_IS_THREAD_SAFE = 0x1
} agt_pool_flag_bits_t;
typedef agt_flags32_t agt_pool_flags_t;

typedef enum agt_weak_ref_flag_bits_t {
  AGT_WEAK_REF_RETAIN_IF_ACQUIRED = 0x1
} agt_weak_ref_flag_bits_t;
typedef agt_flags32_t agt_weak_ref_flags_t;


/*typedef enum agt_init_flag_bits_t {
  AGT_INIT_AGENTS_MODULE   = 0x1,
  AGT_INIT_ASYNC_MODULE    = 0x2,
  AGT_INIT_CHANNELS_MODULE = 0x4,
  AGT_INIT_ALL_MODULES     = AGT_INIT_AGENTS_MODULE | AGT_INIT_ASYNC_MODULE | AGT_INIT_CHANNELS_MODULE,
  AGT_INIT_SINGLE_THREADED = 0x100000000ULL
} agt_init_flag_bits_t;*/

typedef agt_flags64_t agt_init_flag_bits_t;

static const agt_init_flag_bits_t AGT_INIT_AGENTS_MODULE = 0x1;
static const agt_init_flag_bits_t AGT_INIT_ASYNC_MODULE  = 0x2;
static const agt_init_flag_bits_t AGT_INIT_CHANNELS_MODULE = 0x4;
static const agt_init_flag_bits_t AGT_INIT_ALL_MODULES     = AGT_INIT_AGENTS_MODULE | AGT_INIT_ASYNC_MODULE | AGT_INIT_CHANNELS_MODULE;
static const agt_init_flag_bits_t AGT_INIT_SINGLE_THREADED = 0x100000000;

#define AGT_MODULE_BITMASK 0x00000000FFFFFFFF

typedef agt_flags64_t agt_init_flags_t;

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
  AGT_ATTR_LIBRARY_PATH,                 //< type: STRING or WIDE_STRING
  AGT_ATTR_MIN_LIBRARY_VERSION,          //< type: INT32
  AGT_ATTR_SHARED_CONTEXT,               //< type: BOOLEAN
  AGT_ATTR_SHARED_NAMESPACE,             //< type: STRING or WIDE_STRING
  AGT_ATTR_CHANNEL_DEFAULT_CAPACITY,     //> type: UINT32
  AGT_ATTR_CHANNEL_DEFAULT_MESSAGE_SIZE, //> type: UINT32
  AGT_ATTR_CHANNEL_DEFAULT_TIMEOUT_MS    //> type: UINT32
} agt_attr_id_t;

typedef struct agt_attr_t {
  agt_attr_id_t   id;
  agt_attr_type_t type;
  agt_bool_t      allowEnvironmentOverride;
  union {
    const void* ptr;
    agt_bool_t  boolean;
    agt_u32_t   u32;
    agt_i32_t   i32;
    agt_u64_t   u64;
    agt_i64_t   i64;
  } value;
} agt_attr_t;

typedef struct agt_init_info_t {
  const agt_attr_t* attributes;
  agt_size_t        attributeCount;
  agt_init_flags_t  flags;
  int               headerVersion;  //< must be set to AGT_API_VERSION
} agt_init_info_t;



/* =================[ Functions ]================= */


/**
 * Initializes a library context.
 *
 * Can be configured via environment variables.
 *
 * @param apiVersion Must be AGT_API_VERSION
 *
 *
 * Initialization Variables:
 *
 * Types:
 *      - bool: value is parsed as being either true or false, with "true", "1", "yes", or "on" interpreted as true,
 *              and "false", "0", "no", or "off" interpreted as false. Not case sensitive.
 *              If the variable is defined, but has no value, this is interpreted as true.
 *              If the variable is not defined at all, this is interpreted as false.
 *      - integer: value is parsed as an integer. With no prefix, value is parsed in base 10.
 *                 The prefix "0x" causes the value to be interpreted in hexadecimal.
 *                 The prefix "00" causes the value to be interpreted in octal.
 *                 The prefix "0b" causes the value to be interpreted in binary.
 *      - number: value is parsed as a real number. Can be an integer or a float.
 *      - string: value is parsed as is. As such, the value is case sensitive.
 *
 * |                               Name |    Type   |   Default Value   | Description
 * |------------------------------------|-----------|-------------------|--------------
 * |              AGATE_PRIVATE_CONTEXT |  bool     |       false       | If true, all interprocess capabilities will be disabled for this context. Any attempt to create a shared entity from this context will result in a return code of AGT_ERROR_CANNOT_CREATE_SHARED.
 * |             AGATE_SHARED_NAMESPACE |  string   | agate-default-key | If interprocess capabilities are enabled (as they are by default), this context will be able to communicate with any other contexts that were created with the same key.
 * |     AGATE_CHANNEL_DEFAULT_CAPACITY |  integer  |        255        | When creating a channel, a capacity is specified. The created channel is guaranteed to be able to concurrently store at least that many messages. If the specified capacity is 0, then the default capacity is used, as defined by this variable.
 * | AGATE_CHANNEL_DEFAULT_MESSAGE_SIZE |  integer  |        196        | When creating a channel, a message size is specified. The created channel is guaranteed to be able to send messages of up to the specified size in bytes without dynamic allocation. If the specified size is 0, then the default size is used, as defined by this variable.
 * |   AGATE_CHANNEL_DEFAULT_TIMEOUT_MS |  integer  |       30000       | When creating a channel, a timeout in milliseconds is specified. Any handles to the created channel will be disconnected if there is no activity after the duration specified in this variable. Used as a safeguard against denial of service-esque attacks.
 *
 * TODO: Decide whether or not to allow customized initialization via function parameters
 * */
AGT_api agt_status_t        AGT_stdcall agt_init_(agt_ctx_t* pContext, int apiVersion) AGT_noexcept;

// #define agt_init(...) (agt_init_(__VA_ARGS__, AGT_API_VERSION))

AGT_api agt_status_t        AGT_stdcall agt_init(agt_ctx_t* pContext, const agt_init_info_t* pInitInfo) AGT_noexcept;



/**
 * Closes the provided context. Behaviour of this function depends on how the library was configured
 *
 * TODO: Decide whether to provide another API call with differing behaviour depending on whether or not
 *       one wishes to wait for processing to finish.
 * */
AGT_api agt_status_t        AGT_stdcall agt_finalize(agt_ctx_t context) AGT_noexcept;

/**
 * Returns the API version of the linked library.
 * */
AGT_api int                 AGT_stdcall agt_get_library_version() AGT_noexcept;



AGT_api agt_error_handler_t AGT_stdcall agt_get_error_handler(agt_ctx_t context) AGT_noexcept;

AGT_api agt_error_handler_t AGT_stdcall agt_set_error_handler(agt_ctx_t context, agt_error_handler_t errorHandlerCallback) AGT_noexcept;





/* ============[ Fixed Size Memory Pool ]============ */


AGT_api agt_status_t        AGT_stdcall agt_new_pool(agt_ctx_t ctx, agt_pool_t* pPool, agt_size_t fixedSize, agt_pool_flags_t flags) AGT_noexcept;
AGT_api agt_status_t        AGT_stdcall agt_reset_pool(agt_pool_t pool) AGT_noexcept;
AGT_api void                AGT_stdcall agt_destroy_pool(agt_pool_t pool) AGT_noexcept;

AGT_api void*               AGT_stdcall agt_pool_alloc(agt_pool_t pool) AGT_noexcept;
AGT_api void                AGT_stdcall agt_pool_free(agt_pool_t pool, void* allocation) AGT_noexcept;





/* ============[ Reference Counted Memory Pool ]============ */

/*
 * Reference Counted Memory Pool API Guide
 *
 * agt_rcpool_t is a memory pool similar to agt_pool_t,
 * but where allocations are flexibly reference counted.
 *
 * The ability to reset the memory pool en-mass is lost in exchange
 * for reference counting with both strong and weak reference semantics.
 *
 * A weak reference consists of a pair of opaque values;
 *    - agt_weak_ref_t: an opaque reference token; refers to a reference
 *                      counted allocation that may or may not be valid.
 *    - agt_epoch_t:    an opaque epoch value; acts as a verifier for
 *                      the token.
 *
 *
 * agt_rc_alloc(pool, initialCount):
 *      Creates a new allocation from pool with an
 *      initial reference count of $initialCount.
 *
 * agt_rc_retain($alloc, $count):
 *      Increases the reference count of $alloc by $count
 *
 * agt_rc_release($alloc, $count):
 *      Decreases the reference count of $alloc by $count,
 *      releasing the allocation back to the pool from
 *      which it was allocated if the reference count has
 *      been reduced to zero.
 *
 * agt_rc_recycle($alloc, $releaseCount, $initialCount):
 *      Releases $releaseCounts references, and returns a
 *      new allocation from the same pool if the count was
 *      reduced to zero. This is an optimization primarily
 *      intended for the case where it is expected that
 *      the reference count will be reduced to zero, in
 *      which case the allocation may be reused without
 *      any interaction with the underlying pool. In
 *      either case, the reference count of the returned
 *      allocation is $initialCount.
 *
 * agt_weak_ref_take($alloc, $pEpoch, $count):
 *      Returns $count weak references referring to $alloc.
 *      This does not increase the reference count at all,
 *      and the returned weak reference must be
 *      successfully reacquired with
 *      agt_acquire_from_weak_ref to be accessed. $pEpoch
 *      must not be null, as the weak references' epoch
 *      is returned by writing to the memory pointed to
 *      by $pEpoch.
 *      NOTE: Despite "returning" $count weak references,
 *      only a single token/epoch pair is actually
 *      returned. In effect, each weak reference shares
 *      the same values for the token and epoch.
 *
 * agt_weak_ref_retain($ref, $epoch, $count):
 *      Acquires $count more weak references to the same
 *      allocation referred to by $ref and $epoch. If
 *      AGT_NULL_WEAK_REF is returned, the underlying
 *      allocation has already been invalidated, and the
 *      weak reference is dropped (otherwise it'd be
 *      necessary to call agt_weak_ref_drop immediately
 *      after anyways). As such, the return value should
 *      always be checked.
 *
 * agt_weak_ref_drop($ref, $epoch, $count):
 *      Drops $count weak references to the allocation
 *      referred to by $ref and $epoch. Take note that
 *      other API calls may cause a weak reference to be
 *      dropped, so only call when the owner of a weak
 *      reference no longer needs to reacquire it, or when
 *      multiple weak references are to be dropped at once.
 *
 * agt_acquire_from_weak_ref($ref, $epoch, $count, $flags):
 *      Try to reacquire a strong reference to a ref
 *      counted allocation referred to by $ref and $epoch.
 *      If successful, a pointer to the allocation is
 *      returned, and its reference count is increased by
 *      $count. Also if successful, one weak reference is
 *      dropped unless AGT_WEAK_REF_RETAIN_IF_ACQUIRED is
 *      specified in $flags. If unsuccessful, NULL is
 *      returned, and one weak reference is dropped.
 *      NOTE: While the reference count on success is
 *      incremented by $count, only one weak reference is
 *      dropped in either case.
 *
 *
 * */




AGT_api agt_status_t        AGT_stdcall agt_new_rcpool(agt_ctx_t ctx, agt_rcpool_t* pPool, agt_size_t fixedSize, agt_pool_flags_t flags) AGT_noexcept;
AGT_api void                AGT_stdcall agt_destroy_rcpool(agt_rcpool_t pool) AGT_noexcept;

/**
 * Acquire ownership over a new rc allocation from the specified pool.
 * */
AGT_api void*               AGT_stdcall agt_rc_alloc(agt_rcpool_t pool, agt_u32_t initialRefCount) AGT_noexcept;
/**
 * Release ownership of the specified allocation, and acquire another from the pool from which it was allocated
 *
 * Semantically equivalent to calling
 *
 * In many cases, this is much more efficient than calling
 * agt_rc_release(allocation);
 * result = agt_rc_alloc(pool);
 * */
AGT_api void*               AGT_stdcall agt_rc_recycle(void* allocation, agt_u32_t releasedCount, agt_u32_t acquiredCount) AGT_noexcept;
AGT_api void*               AGT_stdcall agt_rc_retain(void* allocation, agt_u32_t count) AGT_noexcept;
AGT_api void                AGT_stdcall agt_rc_release(void* allocation, agt_u32_t count) AGT_noexcept;


AGT_api agt_weak_ref_t      AGT_stdcall agt_weak_ref_take(void* rcObj, agt_epoch_t* pEpoch, agt_u32_t count) AGT_noexcept;
AGT_api agt_weak_ref_t      AGT_stdcall agt_weak_ref_retain(agt_weak_ref_t ref, agt_epoch_t epoch, agt_u32_t count) AGT_noexcept;
AGT_api void                AGT_stdcall agt_weak_ref_drop(agt_weak_ref_t token, agt_u32_t count) AGT_noexcept;
AGT_api void*               AGT_stdcall agt_acquire_from_weak_ref(agt_weak_ref_t token, agt_epoch_t epoch, agt_weak_ref_flags_t flags) AGT_noexcept;




AGT_end_c_namespace

#endif//AGATE_CORE_H
