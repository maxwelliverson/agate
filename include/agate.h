//
// Created by maxwe on 2022-03-04.
//

#ifndef AGATE_AGATE_LOWER_H
#define AGATE_AGATE_LOWER_H


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


#define AGT_ASYNC_STRUCT_SIZE 32
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
#define AGT_VERSION ((AGT_VERSION_MAJOR << 22) | (AGT_VERSION_MINOR << 12) | AGT_VERSION_PATCH)

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
typedef size_t             agt_address_t;
typedef ptrdiff_t          agt_ptrdiff_t;


typedef agt_u32_t          agt_flags32_t;
typedef agt_u64_t          agt_flags64_t;


typedef agt_u32_t          agt_local_id_t;
typedef agt_u64_t          agt_global_id_t;


typedef void*              agt_handle_t;







typedef agt_i32_t agt_bool_t;

typedef agt_u64_t agt_timeout_t;

typedef agt_u32_t agt_protocol_id_t;
typedef agt_u64_t agt_message_id_t;
typedef agt_u64_t agt_type_id_t;
typedef agt_u64_t agt_object_id_t;

typedef agt_u64_t agt_send_token_t;
typedef agt_u64_t agt_name_token_t;



typedef void*                  agt_handle_t;

typedef struct agt_ctx_st*     agt_ctx_t;
// typedef struct agt_async_st*   agt_async_t;
// typedef struct agt_signal_st*  agt_signal_t;
typedef struct agt_message_st* agt_message_t;









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
  AGT_ERROR_INVALID_OPERATION /** < The given handle does not implement the called function. eg. agt_receive_message cannot be called on a channel sender*/,
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
  AGT_ERROR_BAD_ALLOC,
  AGT_ERROR_MAILBOX_IS_FULL,
  AGT_ERROR_MAILBOX_IS_EMPTY,
  AGT_ERROR_TOO_MANY_SENDERS,
  AGT_ERROR_NO_SENDERS,
  AGT_ERROR_TOO_MANY_RECEIVERS,
  AGT_ERROR_NO_RECEIVERS,
  AGT_ERROR_ALREADY_RECEIVED,
  AGT_ERROR_INITIALIZATION_FAILED,
  AGT_ERROR_NOT_YET_IMPLEMENTED
} agt_status_t;

typedef enum agt_handle_type_t {
  AGT_CHANNEL,
  AGT_AGENT,
  AGT_SOCKET,
  AGT_SENDER,
  AGT_RECEIVER,
  AGT_THREAD,
  AGT_AGENCY
} agt_handle_type_t;

typedef enum agt_handle_flag_bits_t {
  AGT_OBJECT_IS_SHARED = 0x1,
  AGT_HANDLE_FLAGS_MAX
} agt_handle_flag_bits_t;
typedef agt_flags32_t agt_handle_flags_t;

typedef enum agt_send_flag_bits_t {
  AGT_SEND_WAIT_FOR_ANY      = 0x00 /**< The asynchronous operation will be considered complete as soon as a single consumer processes the message (Default behaviour)*/,
  AGT_SEND_WAIT_FOR_ALL      = 0x01 /**< The asynchronous operation will be considered complete once all consumers have processed the message */,
  AGT_SEND_BROADCAST_MESSAGE = 0x02 /**< The message will be received by all consumers rather than just one*/,
  AGT_SEND_DATA_CSTRING      = 0x04 /**< The message data is in the form of a null terminated string. When this flag is specified, messageSize can be 0, in which case the messageSize is interpreted to be strlen(data) + 1*/,
  AGT_SEND_WITH_TIMESTAMP    = 0x08 /**< */,
  AGT_SEND_FAST_CLEANUP      = 0x10 /**< */,
} agt_send_flag_bits_t;
typedef agt_flags32_t agt_send_flags_t;

typedef enum agt_message_flag_bits_t {
  AGT_MESSAGE_DATA_IS_CSTRING   = 0x1,
  AGT_MESSAGE_HAS_ID            = 0x2,
  AGT_MESSAGE_IS_FOREIGN        = 0x4,
  AGT_MESSAGE_HAS_RETURN_HANDLE = 0x8
} agt_message_flag_bits_t;
typedef agt_flags32_t agt_message_flags_t;

typedef enum agt_connect_flag_bits_t {
  AGT_CONNECT_FORWARD   = 0x1,
} agt_connect_flag_bits_t;
typedef agt_flags32_t agt_connect_flags_t;

typedef enum agt_blocking_thread_create_flag_bits_t {
  AGT_BLOCKING_THREAD_COPY_USER_DATA   = 0x1,
  AGT_BLOCKING_THREAD_USER_DATA_STRING = 0x2
} agt_blocking_thread_create_flag_bits_t;
typedef agt_flags32_t agt_blocking_thread_create_flags_t;

typedef enum agt_scope_t {
  AGT_SCOPE_LOCAL,
  AGT_SCOPE_SHARED,
  AGT_SCOPE_PRIVATE
} agt_scope_t;

typedef enum agt_message_action_t {
  AGT_COMPLETE_MESSAGE,
  AGT_DISCARD_MESSAGE // idk
} agt_message_action_t;

typedef enum agt_agency_action_t {
  AGT_AGENCY_ACTION_CONTINUE,
  AGT_AGENCY_ACTION_DEFER,
  AGT_AGENCY_ACTION_YIELD,
  AGT_AGENCY_ACTION_CLOSE
} agt_agency_action_t;




typedef struct agt_async_t {
  agt_u8_t reserved[AGT_ASYNC_STRUCT_SIZE];
} agt_async_t;

typedef struct agt_signal_t {
  agt_u8_t reserved[AGT_SIGNAL_STRUCT_SIZE];
} agt_signal_t;

typedef struct agt_send_info_t {
  agt_send_flags_t flags;
  agt_async_t*     asyncHandle;
  agt_handle_t     returnHandle;
  agt_message_id_t messageId;
  agt_size_t       messageSize;
  const void*      pMessageBuffer;
} agt_send_info_t;

typedef struct agt_message_info_t {
  agt_message_t      message;
  agt_size_t         size;
  agt_message_flags_t flags;
} agt_message_info_t;

typedef struct agt_object_info_t {
  agt_object_id_t    id;
  agt_handle_t      handle;
  agt_handle_type_t  type;
  agt_handle_flags_t flags;
  agt_object_id_t    exportId;
} agt_object_info_t;

typedef struct agt_proxy_object_functions_t {
  agt_status_t (* const acquireMessage)(void* object, agt_staged_message_t* pStagedMessage, agt_timeout_t timeout) noexcept;
  void         (* const pushQueue)(void* object, agt_message_t message, agt_send_flags_t flags) noexcept;
  agt_status_t (* const popQueue)(void* object, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) noexcept;
  void         (* const releaseMessage)(void* object, agt_message_t message) noexcept;
  agt_status_t (* const connect)(void* object, agt_handle_t handle, agt_connect_flags_t flags) noexcept;
  agt_status_t (* const acquireRef)(void* object) noexcept;
  agt_size_t   (* const releaseRef)(void* object) noexcept;
  void         (* const destroy)(void* object) noexcept;
} agt_proxy_object_functions_t;




typedef agt_status_t (*agt_actor_message_proc_t)(void* actorState, const agt_message_info_t* message);
typedef void         (*agt_actor_dtor_t)(void* actorState);


// TODO: Add a function to the public API that extends the lifetime of a message, and one that ends that extension.
//       Having these functions in place, messages can be automatically returned after use. Possibly with reference counting??
typedef void      (*agt_blocking_thread_message_proc_t)(agt_handle_t threadHandle, const agt_message_info_t* message, void* pUserData);
typedef void      (*agt_user_data_dtor_t)(void* pUserData);

typedef struct agt_actor_t {
  agt_type_id_t            type;
  agt_actor_message_proc_t proc;
  agt_actor_dtor_t         dtor;
  void*                    state;
} agt_actor_t;






typedef struct agt_channel_create_info_t {
  size_t      minCapacity;
  size_t      maxMessageSize;
  size_t      maxSenders;
  size_t      maxReceivers;
  agt_scope_t scope;
  agt_async_t asyncHandle;
  const char* name;
  size_t      nameLength;
} agt_channel_create_info_t;
typedef struct agt_socket_create_info_t {

} agt_socket_create_info_t;
typedef struct agt_agent_create_info_t {

} agt_agent_create_info_t;
typedef struct agt_agency_create_info_t {

} agt_agency_create_info_t;
typedef struct agt_blocking_thread_create_info_t {
  agt_blocking_thread_message_proc_t pfnMessageProc;
  agt_scope_t                        scope;
  agt_blocking_thread_create_flags_t flags;
  size_t                             minCapacity;
  size_t                             maxMessageSize;
  size_t                             maxSenders;
  union {
    size_t                           dataSize;
    agt_user_data_dtor_t             pfnUserDataDtor;
  };
  void*                              pUserData;
  const char*                        name;
  size_t                             nameLength;
} agt_blocking_thread_create_info_t;
typedef struct agt_thread_create_info_t {

} agt_thread_create_info_t;



/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/* ========================= [ Context ] ========================= */

/**
 * Initializes a library context.
 *
 * Can be configured via environment variables.
 *
 * TODO: Decide whether or not to allow customized initialization via function parameters
 * */
AGT_api agt_status_t AGT_stdcall agt_init(agt_ctx_t* pContext) AGT_noexcept;

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
AGT_api agt_u32_t    AGT_stdcall agt_get_version() AGT_noexcept;



/* ========================= [ Handles ] ========================= */

/**
 *
 * */
AGT_api agt_status_t AGT_stdcall agt_get_object_info(agt_ctx_t context, agt_object_info_t* pObjectInfo) AGT_noexcept;
/**
 *
 * */
AGT_api agt_status_t AGT_stdcall agt_duplicate_handle(agt_handle_t inHandle, agt_handle_t* pOutHandle) AGT_noexcept;
/**
 *
 * */
AGT_api void         AGT_stdcall agt_close_handle(agt_handle_t handle) AGT_noexcept;


/**
 *
 * */
AGT_api agt_status_t AGT_stdcall agt_create_channel(agt_ctx_t context, const agt_channel_create_info_t* cpCreateInfo, agt_handle_t* pSender, agt_handle_t* pReceiver) AGT_noexcept;
AGT_api agt_status_t AGT_stdcall agt_create_agent(agt_ctx_t context, const agt_agent_create_info_t* cpCreateInfo, agt_handle_t* pAgent) AGT_noexcept;
AGT_api agt_status_t AGT_stdcall agt_create_agency(agt_ctx_t context, const agt_agency_create_info_t* cpCreateInfo, agt_handle_t* pAgency) AGT_noexcept;
AGT_api agt_status_t AGT_stdcall agt_create_thread(agt_ctx_t context, const agt_thread_create_info_t* cpCreateInfo, agt_handle_t* pThread) AGT_noexcept;


/**
 * pSendInfo must not be null.
 * If sender is null, returns AGT_ERROR_NULL_HANDLE
 * If pSendInfo->messageSize is zero, agt_get_send_token will write the handle's maximum message size to pSendInfo->messageSize.
 * If pSendInfo->messageSize is not zero, its value will not be modified. In that case, if the value of pSendInfo->messageSize is greater than the handle's maximum message size, AGT_ERROR_MESSAGE_TOO_LARGE is returned.
 * If timeout is AGT_DO_NOT_WAIT, the call immediately returns AGT_ERROR_MAILBOX_IS_FULL if no send tokens are available.
 * If timeout is AGT_WAIT, the call waits indefinitely until a send token is available. In this case, AGT_ERROR_MAILBOX_IS_FULL is never returned.
 * If the timeout is some positive integer N, if after N microseconds, no send tokens have been made available, AGT_ERROR_MAILBOX_IS_FULL is returned.
 *
 * If AGT_SUCCESS is returned, a valid send token will have been written to *pSendToken
 * This send token must then be used in a call to agt_send_with_token.
 */
AGT_api agt_status_t AGT_stdcall agt_get_send_token(agt_handle_t sender, size_t* messageSize, agt_send_token_t* pSendToken, agt_timeout_t timeout) AGT_noexcept;
/**
 * The value of sendToken must have previously been acquired from a successful call to agt_get_send_token.
 * If the value of pSendInfo->messageSize has changed since the call to agt_get_send_token, N bytes will be copied from the buffer pointed to by cpSendInfo->pMessageBuffer,
 *     where N is equal to the minimum of cpSendInfo->messageSize and the sender handle's maximum supported message size.
 * For speed purposes, this function does not do any error checking. It is up to the caller to ensure proper usage.
 *
 * sender must be the same handle used in the prior call to agt_get_send_token from which sendToken was returned.
 * cpSendInfo must point to the same struct pointed to by the pSendInfo parameter of the prior call to agt_get_send_token.
 * sendToken must have been returned from a successful prior call to agt_get_send_token.
 * cpSendInfo->messageSize must not be greater than the maximum message size supported by sender.
 * cpSendInfo->pMessageBuffer must point to a valid, readable buffer of at least cpSendInfo->messageSize bytes.
 * cpSendInfo->messageId is optional.
 * cpSendInfo->asyncHandle must either be null, or be a valid handle obtained from a call to agt_new_async. If the asyncHandle was already attached to an asynchronous operation, that operation is dropped and replaced.
 *
 * If the sender object has set a "returnHandle" property, cpSendInfo->returnHandle is ignored.
 * Otherwise, if the caller does not wish to provide a return handle (or simply has no need), ensure cpSendInfo->returnHandle is set to AGT_NULL_HANDLE.
 *
 * This call "consumes" sendToken, and sendToken must not be used for any other calls.
 */
AGT_api void         AGT_stdcall agt_send_with_token(agt_handle_t sender, const agt_send_info_t* cpSendInfo, agt_send_token_t sendToken) AGT_noexcept;
/**
 * Essentially performs agt_get_send_token and agt_send_with_token in a single call, allowing for fewer API calls and simpler control flow,
 * but removing the ability to ensure the send operation will be successful before preparing the buffer. All the same
 * conditions for use as agt_send_with_token are true here.
 *
 * In the case that the operation fails, the async handle will not be attached (and moreover, won't be detatched from a prior operation),
 * no buffer write will occur, etc. etc.
 *
 */
AGT_api agt_status_t AGT_stdcall agt_send(agt_handle_t sender, const agt_send_info_t* cpSendInfo, agt_timeout_t timeout) AGT_noexcept;


/**
 *
 */
AGT_api agt_status_t AGT_stdcall agt_receive_message(agt_handle_t receiver, agt_message_info_t* pMessageInfo, agt_timeout_t timeout) AGT_noexcept;

/**
 * Prerequisites:
 *  - message is a valid handle obtained from a prior call to agt_receive_message
 *  - buffer is a valid pointer to a memory buffer with at least bufferLength bytes of data
 *
 * If bufferLength is less than the value returned in pMessageInfo->size from the call to agt_receive_message,
 * this will return AGT_ERROR_INSUFFICIENT_SIZE (?).
 */
AGT_api agt_status_t AGT_stdcall agt_read_message_data(agt_message_t message, agt_size_t bufferLength, void* buffer) AGT_noexcept;



/**
 * Close message
 */
AGT_api void         AGT_stdcall agt_close_message(agt_handle_t owner, agt_message_t message) AGT_noexcept;

/**
 *
 */
AGT_api agt_status_t AGT_stdcall agt_connect(agt_handle_t to, agt_handle_t from, agt_connect_flags_t flags) AGT_noexcept;






/* ========================= [ Messages ] ========================= */

AGT_api void         AGT_stdcall agt_return(agt_message_t message, agt_status_t status) AGT_noexcept;




AGT_api agt_status_t AGT_stdcall agt_dispatch_message(const agt_actor_t* pActor, const agt_message_info_t* pMessageInfo) AGT_noexcept;
// AGT_api agt_status_t     AGT_stdcall agt_execute_on_thread(agt_thread_t thread, ) AGT_noexcept;





AGT_api agt_status_t AGT_stdcall agt_get_sender_handle(agt_message_t message, agt_handle_t* pSenderHandle) AGT_noexcept;






/* ========================= [ Async ] ========================= */

AGT_api agt_status_t AGT_stdcall agt_new_async(agt_ctx_t ctx, agt_async_t* pAsync) AGT_noexcept;
AGT_api void         AGT_stdcall agt_copy_async(const agt_async_t* from, agt_async_t* to) AGT_noexcept;
AGT_api void         AGT_stdcall agt_clear_async(agt_async_t* async) AGT_noexcept;
AGT_api void         AGT_stdcall agt_destroy_async(agt_async_t* async) AGT_noexcept;

AGT_api agt_status_t AGT_stdcall agt_wait(agt_async_t* async, agt_timeout_t timeout) AGT_noexcept;
AGT_api agt_status_t AGT_stdcall agt_wait_all(agt_async_t* const * pAsyncs, agt_size_t asyncCount, agt_timeout_t timeout) AGT_noexcept;
AGT_api agt_status_t AGT_stdcall agt_wait_any(agt_async_t* const * pAsyncs, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) AGT_noexcept;




/* ========================= [ Signal ] ========================= */

AGT_api agt_status_t AGT_stdcall agt_new_signal(agt_ctx_t ctx, agt_signal_t* pSignal) AGT_noexcept;
AGT_api void         AGT_stdcall agt_attach_signal(agt_signal_t* signal, agt_async_t* async) AGT_noexcept;
AGT_api void         AGT_stdcall agt_raise_signal(agt_signal_t* signal) AGT_noexcept;
AGT_api void         AGT_stdcall agt_raise_many_signals(agt_signal_t* const * pSignals, agt_size_t signalCount) AGT_noexcept;
AGT_api void         AGT_stdcall agt_destroy_signal(agt_signal_t* signal) AGT_noexcept;



// AGT_api agt_status_t     AGT_stdcall agt_get_async



// AGT_api void          AGT_stdcall agt_yield_execution() AGT_noexcept;




AGT_end_c_namespace


#endif//AGATE_AGATE_LOWER_H
