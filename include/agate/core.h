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




/* =================[ Constants ]================= */


#define AGT_DONT_CARE          ((agt_u64_t)-1)
#define AGT_NULL_HANDLE        ((void*)0)

#define AGT_CACHE_LINE         ((agt_size_t)1 << 6)
#define AGT_PHYSICAL_PAGE_SIZE ((agt_size_t)1 << 12)
#define AGT_VIRTUAL_PAGE_SIZE  ((agt_size_t)(1 << 16))


#define AGT_ASYNC_STRUCT_SIZE 40
#define AGT_SIGNAL_STRUCT_SIZE 24


#define AGT_INVALID_OBJECT_ID ((agt_object_id_t)-1)
#define AGT_SYNCHRONIZE ((agt_async_t)AGT_NULL_HANDLE)



#if AGT_system_windows
# define AGT_NATIVE_TIMEOUT_CONVERSION 10
#else
# define AGT_NATIVE_TIMEOUT_CONVERSION 1000
#endif

#define AGT_TIMEOUT(microseconds) ((agt_timeout_t)(microseconds * AGT_NATIVE_TIMEOUT_CONVERSION))
#define AGT_DO_NOT_WAIT ((agt_timeout_t)0)
#define AGT_WAIT ((agt_timeout_t)-1)

#define AGT_VERSION_MAJOR 0
#define AGT_VERSION_MINOR 1
#define AGT_VERSION_PATCH 0
#define AGT_API_VERSION ((AGT_VERSION_MAJOR << 22) | (AGT_VERSION_MINOR << 12) | AGT_VERSION_PATCH)

// Version format:
//                 [    Major   ][    Minor   ][    Patch     ]
//                 [   10 bits  ][   10 bits  ][    12 bits   ]
//                 [   31 - 22  ][   21 - 12  ][    11 - 00   ]
//                 [ 0000000000 ][ 0000000000 ][ 000000000000 ]




/* =================[ Types ]================= */

AGT_begin_c_namespace


typedef unsigned char      agt_u8_t;
typedef   signed char      agt_i8_t;
typedef          char      agt_char_t;
typedef unsigned short     agt_u16_t;
typedef   signed short     agt_i16_t;
typedef unsigned int       agt_u32_t;
typedef   signed int       agt_i32_t;
typedef unsigned long long agt_u64_t;
typedef   signed long long agt_i64_t;



typedef size_t             agt_size_t;
typedef uintptr_t          agt_address_t;
typedef ptrdiff_t          agt_ptrdiff_t;


typedef agt_u32_t          agt_flags32_t;
typedef agt_u64_t          agt_flags64_t;


typedef agt_i32_t agt_bool_t;

typedef agt_u64_t agt_timeout_t;

typedef agt_u64_t agt_message_id_t;
typedef agt_u64_t agt_type_id_t;
typedef agt_u64_t agt_object_id_t;

typedef agt_u64_t agt_send_token_t;
typedef agt_u64_t agt_name_token_t;


typedef agt_u64_t agt_deptok_t;


typedef agt_u32_t agt_local_id_t;
typedef agt_u64_t agt_global_id_t;

typedef struct agt_ctx_st*  agt_ctx_t;
typedef struct agt_async_t  agt_async_t;
typedef struct agt_signal_t agt_signal_t;



typedef enum agt_error_handler_status_t {
  AGT_ERROR_HANDLED,
  AGT_ERROR_IGNORED,
  AGT_ERROR_NOT_HANDLED
} agt_error_handler_status_t;

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
  AGT_ERROR_INVALID_CMDERATION /** < The given handle does not implement the called function. eg. agt_receive_message cannot be called on a channel sender*/,
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
  AGT_ERROR_CORRUPTED_MESSAGE
} agt_status_t;


typedef agt_error_handler_status_t (AGT_stdcall *agt_error_handler_t)(agt_status_t errorCode, void* errorData);



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
 * |                   AGATE_SHARED_KEY |  string   | agate-default-key | If interprocess capabilities are enabled (as they are by default), this context will be able to communicate with any other contexts that were created with the same key.
 * |     AGATE_CHANNEL_DEFAULT_CAPACITY |  integer  |        255        | When creating a channel, a capacity is specified. The created channel is guaranteed to be able to concurrently store at least that many messages. If the specified capacity is 0, then the default capacity is used, as defined by this variable.
 * | AGATE_CHANNEL_DEFAULT_MESSAGE_SIZE |  integer  |        196        | When creating a channel, a message size is specified. The created channel is guaranteed to be able to send messages of up to the specified size in bytes without dynamic allocation. If the specified size is 0, then the default size is used, as defined by this variable.
 * |   AGATE_CHANNEL_DEFAULT_TIMEOUT_MS |  integer  |       30000       | When creating a channel, a timeout in milliseconds is specified. Any handles to the created channel will be disconnected if there is no activity after the duration specified in this variable. Used as a safeguard against denial of service-esque attacks.
 *
 * TODO: Decide whether or not to allow customized initialization via function parameters
 * */
AGT_api agt_status_t AGT_stdcall agt_init_(agt_ctx_t* pContext, int apiVersion) AGT_noexcept;

#define agt_init(...) (agt_init_(__VA_ARGS__, AGT_API_VERSION))


/**
 * Closes the provided context. Behaviour of this function depends on how the library was configured
 *
 * TODO: Decide whether to provide another API call with differing behaviour depending on whether or not
 *       one wishes to wait for processing to finish.
 * */
AGT_api agt_status_t AGT_stdcall agt_finalize(agt_ctx_t context) AGT_noexcept;

/**
 * Returns the API version of the linked library.
 * */
AGT_api int          AGT_stdcall agt_get_library_version() AGT_noexcept;



AGT_api agt_error_handler_t AGT_stdcall agt_get_error_handler(agt_ctx_t context) AGT_noexcept;

AGT_api agt_error_handler_t AGT_stdcall agt_set_error_handler(agt_ctx_t context, agt_error_handler_t errorHandlerCallback) AGT_noexcept;


AGT_end_c_namespace

#endif//AGATE_CORE_H
