//
// Created by maxwe on 2022-06-03.
//

#ifndef AGATE_AGATE_H
#define AGATE_AGATE_H


#include <agate/core.h>
#include <agate/agents.h>
#include <agate/async.h>
#include <agate/channels.h>

//
//
///* ====================[ Portability Config ]================== */
//
//
//#if defined(__cplusplus)
//#define AGT_begin_c_namespace extern "C" {
//#define AGT_end_c_namespace }
//
//#include <cassert>
//#include <cstddef>
//
//#else
//#define AGT_begin_c_namespace
//#define AGT_end_c_namespace
//
//#include <stddef.h>
//#include <assert.h>
//#include <stdbool.h>
//#endif
//
//#if defined(NDEBUG)
//#define AGT_debug_build false
//#else
//#define AGT_debug_build true
//#endif
//
//#if defined(_WIN64) || defined(_WIN32)
//#define AGT_system_windows true
//#define AGT_system_linux false
//#define AGT_system_macos false
//#if defined(_WIN64)
//#define AGT_64bit true
//#define AGT_32bit false
//#else
//#define AGT_64bit false
//#define AGT_32bit true
//#endif
//#else
//#define AGT_system_windows false
//#if defined(__LP64__)
//#define AGT_64bit true
//#define AGT_32bit false
//#else
//#define AGT_64bit false
//#define AGT_32bit true
//#endif
//#if defined(__linux__)
//
//#define AGT_system_linux true
//#define AGT_system_macos false
//#elif defined(__OSX__)
//#define AGT_system_linux false
//#define AGT_system_macos true
//#else
//#define AGT_system_linux false
//#define AGT_system_macos false
//#endif
//#endif
//
//
//#if defined(_MSC_VER)
//#define AGT_compiler_msvc true
//#else
//#define AGT_compiler_msvc false
//#endif
//
//
//#if defined(__clang__)
//#define AGT_compiler_clang true
//#else
//#define AGT_compiler_clang false
//#endif
//#if defined(__GNUC__)
//#define AGT_compiler_gcc true
//#else
//#define AGT_compiler_gcc false
//#endif
//
//
//
//
///* =================[ Portable Attributes ]================= */
//
//
//
//#if defined(__cplusplus)
//# if AGT_compiler_msvc
//#  define AGT_cplusplus _MSVC_LANG
//# else
//#  define AGT_cplusplus __cplusplus
//# endif
//# if AGT_cplusplus >= 201103L  // C++11
//#  define AGT_noexcept   noexcept
//#  define AGT_noreturn [[noreturn]]
//# else
//#  define AGT_noexcept   throw()
//# endif
//# if (AGT_cplusplus >= 201703)
//#  define AGT_nodiscard [[nodiscard]]
//# else
//#  define AGT_nodiscard
//# endif
//#else
//# define AGT_cplusplus 0
//# define AGT_noexcept
//# if (__GNUC__ >= 4) || defined(__clang__)
//#  define AGT_nodiscard    __attribute__((warn_unused_result))
//# elif (_MSC_VER >= 1700)
//#  define AGT_nodiscard    _Check_return_
//# else
//#  define AGT_nodiscard
//# endif
//#endif
//
//
//# if defined(__has_attribute)
//#  define AGT_has_attribute(...) __has_attribute(__VA_ARGS__)
//# else
//#  define AGT_has_attribute(...) false
//# endif
//
//
//#if defined(_MSC_VER) || defined(__MINGW32__)
//# if !defined(AGT_SHARED_LIB)
//#  define AGT_api
//# elif defined(AGT_SHARED_LIB_EXPORT)
//#  define AGT_api __declspec(dllexport)
//# else
//#  define AGT_api __declspec(dllimport)
//# endif
//#
//# if defined(__MINGW32__)
//#  define AGT_restricted
//# else
//#  define AGT_restricted __declspec(restrict)
//# endif
//# pragma warning(disable:4127)   // suppress constant conditional warning
//# define AGT_noinline          __declspec(noinline)
//# define AGT_forceinline       __forceinline
//# define AGT_noalias           __declspec(noalias)
//# define AGT_thread_local      __declspec(thread)
//# define AGT_alignas(n)        __declspec(align(n))
//# define AGT_restrict          __restrict
//#
//# define AGT_cdecl __cdecl
//# define AGT_stdcall __stdcall
//# define AGT_vectorcall __vectorcall
//# define AGT_may_alias
//# define AGT_alloc_size(...)
//# define AGT_alloc_align(p)
//#
//# define AGT_assume(...) __assume(__VA_ARGS__)
//# define AGT_assert(expr) assert(expr)
//# define AGT_unreachable AGT_assert(false); AGT_assume(false)
//#
//# if !defined(AGT_noreturn)
//#  define AGT_noreturn __declspec(noreturn)
//# endif
//#
//#elif defined(__GNUC__)                 // includes clang and icc
//#
//# if defined(__cplusplus)
//# define AGT_restrict __restrict
//# else
//# define AGT_restrict restrict
//# endif
//# define AGT_cdecl                      // leads to warnings... __attribute__((cdecl))
//# define AGT_stdcall
//# define AGT_vectorcall
//# define AGT_may_alias    __attribute__((may_alias))
//# define AGT_api          __attribute__((visibility("default")))
//# define AGT_restricted
//# define AGT_malloc       __attribute__((malloc))
//# define AGT_noinline     __attribute__((noinline))
//# define AGT_noalias
//# define AGT_thread_local __thread
//# define AGT_alignas(n)   __attribute__((aligned(n)))
//# define AGT_assume(...)
//# define AGT_forceinline  __attribute__((always_inline))
//# define AGT_unreachable  __builtin_unreachable()
//#
//# if (defined(__clang_major__) && (__clang_major__ < 4)) || (__GNUC__ < 5)
//#  define AGT_alloc_size(...)
//#  define AGT_alloc_align(p)
//# elif defined(__INTEL_COMPILER)
//#  define AGT_alloc_size(...)       __attribute__((alloc_size(__VA_ARGS__)))
//#  define AGT_alloc_align(p)
//# else
//#  define AGT_alloc_size(...)       __attribute__((alloc_size(__VA_ARGS__)))
//#  define AGT_alloc_align(p)      __attribute__((alloc_align(p)))
//# endif
//#
//# if !defined(AGT_noreturn)
//#  define AGT_noreturn __attribute__((noreturn))
//# endif
//#
//#else
//# define AGT_restrict
//# define AGT_cdecl
//# define AGT_stdcall
//# define AGT_vectorcall
//# define AGT_api
//# define AGT_may_alias
//# define AGT_restricted
//# define AGT_malloc
//# define AGT_alloc_size(...)
//# define AGT_alloc_align(p)
//# define AGT_noinline
//# define AGT_noalias
//# define AGT_forceinline
//# define AGT_thread_local            __thread        // hope for the best :-)
//# define AGT_alignas(n)
//# define AGT_assume(...)
//# define AGT_unreachable AGT_assume(false)
//#endif
//
//#if AGT_has_attribute(transparent_union)
//#define AGT_transparent_union_2(a, b) union __attribute__((transparent_union)) { a m_##a; b m_##b; }
//#define AGT_transparent_union_3(a, b, c) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; }
//#define AGT_transparent_union_4(a, b, c, d) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; }
//#define AGT_transparent_union_5(a, b, c, d, e) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; }
//#define AGT_transparent_union_6(a, b, c, d, e, f) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; }
//#define AGT_transparent_union_7(a, b, c, d, e, f, g) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; }
//#define AGT_transparent_union_8(a, b, c, d, e, f, g, h) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; }
//#define AGT_transparent_union_9(a, b, c, d, e, f, g, h, i) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; }
//#define AGT_transparent_union_10(a, b, c, d, e, f, g, h, i, j) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j;  }
//#define AGT_transparent_union_18(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j; k m_##k; l m_##l; m m_##m; n m_##n; o m_##o; p m_##p; q m_##q; r m_##r; }
//#else
//#define AGT_transparent_union_2(a, b) void*
//#define AGT_transparent_union_3(a, b, c) void*
//#define AGT_transparent_union_4(a, b, c, d) void*
//#define AGT_transparent_union_5(a, b, c, d, e) void*
//#define AGT_transparent_union_6(a, b, c, d, e, f) void*
//#define AGT_transparent_union_7(a, b, c, d, e, f, g) void*
//#define AGT_transparent_union_8(a, b, c, d, e, f, g, h) void*
//#define AGT_transparent_union_9(a, b, c, d, e, f, g, h, i) void*
//#define AGT_transparent_union_10(a, b, c, d, e, f, g, h, i, j) void*
//#define AGT_transparent_union_18(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r) void*
//#endif
//
//
//
//#define AGT_no_default default: AGT_unreachable
//
//#define AGT_cache_aligned     AGT_alignas(AGT_CACHE_LINE)
//#define AGT_page_aligned      AGT_alignas(AGT_PHYSICAL_PAGE_SIZE)
//
//
//#define AGT_agent_api AGT_api
//
//
//
//
///* =================[ Macro Functions ]================= */
//
//
//#if AGT_compiler_gcc || AGT_compiler_clang
//# define AGT_assume_aligned(p, align) __builtin_assume_aligned(p, align)
//#elif AGT_compiler_msvc
//# define AGT_assume_aligned(p, align) (AGT_assume(((agt_u64_t)p) % align == 0), p)
//#else
//# define AGT_assume_aligned(p, align) p
//#endif
//
//
//
//
///* =================[ Constants ]================= */
//
//
//#define AGT_DONT_CARE          ((agt_u64_t)-1)
//#define AGT_NULL_HANDLE        ((void*)0)
//
//#define AGT_CACHE_LINE         ((agt_size_t)1 << 6)
//#define AGT_PHYSICAL_PAGE_SIZE ((agt_size_t)1 << 12)
//#define AGT_VIRTUAL_PAGE_SIZE  ((agt_size_t)(1 << 16))
//
//
//#define AGT_ASYNC_STRUCT_SIZE 40
//#define AGT_SIGNAL_STRUCT_SIZE 24
//
//
//#define AGT_INVALID_OBJECT_ID ((agt_object_id_t)-1)
//#define AGT_SYNCHRONIZE ((agt_async_t)AGT_NULL_HANDLE)
//
//
//
//#if AGT_system_windows
//# define AGT_NATIVE_TIMEOUT_CONVERSION 10
//#else
//# define AGT_NATIVE_TIMEOUT_CONVERSION 1000
//#endif
//
//#define AGT_TIMEOUT(microseconds) ((agt_timeout_t)(microseconds * AGT_NATIVE_TIMEOUT_CONVERSION))
//#define AGT_DO_NOT_WAIT ((agt_timeout_t)0)
//#define AGT_WAIT ((agt_timeout_t)-1)
//
//#define AGT_VERSION_MAJOR 0
//#define AGT_VERSION_MINOR 1
//#define AGT_VERSION_PATCH 0
//#define AGT_API_VERSION ((AGT_VERSION_MAJOR << 22) | (AGT_VERSION_MINOR << 12) | AGT_VERSION_PATCH)
//
//// Version format:
////                 [    Major   ][    Minor   ][    Patch     ]
////                 [   10 bits  ][   10 bits  ][    12 bits   ]
////                 [   31 - 22  ][   21 - 12  ][    11 - 00   ]
////                 [ 0000000000 ][ 0000000000 ][ 000000000000 ]
//
//
//
///* =================[ Types ]================= */
//
//AGT_begin_c_namespace
//
//
//typedef unsigned char      agt_u8_t;
//typedef   signed char      agt_i8_t;
//typedef          char      agt_char_t;
//typedef unsigned short     agt_u16_t;
//typedef   signed short     agt_i16_t;
//typedef unsigned int       agt_u32_t;
//typedef   signed int       agt_i32_t;
//typedef unsigned long long agt_u64_t;
//typedef   signed long long agt_i64_t;
//
//
//
//typedef size_t             agt_size_t;
//typedef uintptr_t          agt_address_t;
//typedef ptrdiff_t          agt_ptrdiff_t;
//
//
//typedef agt_u32_t          agt_flags32_t;
//typedef agt_u64_t          agt_flags64_t;
//
//
//typedef agt_i32_t agt_bool_t;
//
//typedef agt_u64_t agt_timeout_t;
//
//typedef agt_u64_t agt_message_id_t;
//typedef agt_u64_t agt_type_id_t;
//typedef agt_u64_t agt_object_id_t;
//
//typedef agt_u64_t agt_send_token_t;
//typedef agt_u64_t agt_name_token_t;
//
//
//typedef agt_u64_t agt_deptok_t;
//
//
//typedef agt_u32_t                    agt_local_id_t;
//typedef agt_u64_t                    agt_global_id_t;
//
//
//typedef struct agt_ctx_st*           agt_ctx_t;
//typedef struct agt_channel_st*       agt_channel_t;
//typedef struct agt_agent_st*         agt_agent_t;
//typedef struct agt_semaphore_st*     agt_semaphore_t;
//typedef struct agt_barrier_st*       agt_barrier_t;
//typedef const struct agt_message_st* agt_message_t;
//
//typedef struct agt_agent_group_st*   agt_agent_group_t;
//
//
//
//
//typedef enum agt_status_t {
//  AGT_SUCCESS   /** < No errors */,
//  AGT_NOT_READY /** < An asynchronous operation is not yet complete */,
//  AGT_DEFERRED  /** < An operation has been deferred, and code that requires the completion of the operation must wait on the async handle provided */,
//  AGT_CANCELLED /** < An asynchronous operation has been cancelled. This will only be returned if an operation is cancelled before it is complete*/,
//  AGT_TIMED_OUT /** < A wait operation exceeded the specified timeout duration */,
//  AGT_INCOMPLETE_MESSAGE,
//  AGT_ERROR_UNKNOWN,
//  AGT_ERROR_UNKNOWN_FOREIGN_OBJECT,
//  AGT_ERROR_INVALID_OBJECT_ID,
//  AGT_ERROR_EXPIRED_OBJECT_ID,
//  AGT_ERROR_NULL_HANDLE,
//  AGT_ERROR_INVALID_HANDLE_TYPE,
//  AGT_ERROR_NOT_BOUND,
//  AGT_ERROR_FOREIGN_SENDER,
//  AGT_ERROR_STATUS_NOT_SET,
//  AGT_ERROR_UNKNOWN_MESSAGE_TYPE,
//  AGT_ERROR_INVALID_CMDERATION /** < The given handle does not implement the called function. eg. agt_receive_message cannot be called on a channel sender*/,
//  AGT_ERROR_INVALID_FLAGS,
//  AGT_ERROR_INVALID_MESSAGE,
//  AGT_ERROR_INVALID_SIGNAL,
//  AGT_ERROR_CANNOT_DISCARD,
//  AGT_ERROR_MESSAGE_TOO_LARGE,
//  AGT_ERROR_INSUFFICIENT_SLOTS,
//  AGT_ERROR_NOT_MULTIFRAME,
//  AGT_ERROR_BAD_SIZE,
//  AGT_ERROR_INVALID_ARGUMENT,
//  AGT_ERROR_BAD_ENCODING_IN_NAME,
//  AGT_ERROR_NAME_TOO_LONG,
//  AGT_ERROR_NAME_ALREADY_IN_USE,
//  AGT_ERROR_BAD_DISPATCH_KIND,
//  AGT_ERROR_BAD_ALLOC,
//  AGT_ERROR_MAILBOX_IS_FULL,
//  AGT_ERROR_MAILBOX_IS_EMPTY,
//  AGT_ERROR_TOO_MANY_SENDERS,
//  AGT_ERROR_NO_SENDERS,
//  AGT_ERROR_PARTNER_DISCONNECTED,
//  AGT_ERROR_TOO_MANY_RECEIVERS,
//  AGT_ERROR_NO_RECEIVERS,
//  AGT_ERROR_ALREADY_RECEIVED,
//  AGT_ERROR_INITIALIZATION_FAILED,
//  AGT_ERROR_CANNOT_CREATE_SHARED,
//  AGT_ERROR_NOT_YET_IMPLEMENTED,
//  AGT_ERROR_UNKNOWN_MESSAGE_CMD
//} agt_status_t;
//
//
//
//
//
//
//
//
//typedef struct agt_async_t {
//  agt_u8_t reserved[AGT_ASYNC_STRUCT_SIZE];
//} agt_async_t;
//
//typedef struct agt_signal_t {
//  agt_u8_t reserved[AGT_SIGNAL_STRUCT_SIZE];
//} agt_signal_t;
//
//
//
//typedef void* (*agt_agent_ctor_t)(void* userData);
//typedef void  (*agt_agent_dtor_t)(void* agentState);
//typedef void  (*agt_agent_start_t)(void* agentState);
//typedef void  (*agt_agent_proc_t)(void* agentState, agt_message_id_t msgId, const void* message, agt_size_t messageSize);
//
//
//typedef void  (*agt_free_agent_proc_t)(agt_message_id_t msgId, void* message, agt_size_t messageSize);
//
//
//typedef enum agt_agent_create_flag_bits_t {
//  AGT_AGENT_CREATE_SHARED = 0x1
//} agt_agent_create_flag_bits_t;
//typedef agt_flags32_t agt_agent_create_flags_t;
//
//
//typedef enum agt_agent_flag_bits_t {
//  AGT_AGENT_IS_NAMED               = 0x1,
//  AGT_AGENT_IS_SHARED              = 0x2,
//  AGT_AGENT_HAS_DYNAMIC_PROPERTIES = 0x1000
//} agt_agent_flag_bits_t;
//typedef agt_flags32_t agt_agent_flags_t;
//
//typedef enum agt_agent_cmd_t {
//  AGT_CMD_NOOP,                    ///< As the name would imply, this is a noop.
//  AGT_CMD_KILL,                    ///< Command sent on abnormal termination. Minimal cleanup is performed, typically indicates some unhandled error
//
//  AGT_CMD_PROC_MESSAGE,            ///< Process message
//
//  AGT_CMD_CLOSE_QUEUE,             ///< Normal termination command. Any messages sent before this will be processed as normal, but no new messages will be sent. As soon as the queue is empty, the agent is destroyed.
//  AGT_CMD_INVALIDATE_QUEUE,        ///< Current message queue is discarded without having been processed, but the queue is not closed, nor is the agent destroyed.
//
//  AGT_CMD_BARRIER_ARRIVE,          ///< When this message is dequeued, the arrival count of the provided barrier is incremented. If the post-increment arrival count is equal to the expected arrival count, any agents waiting on the barrier are unblocked, and if the barrier was set with a continuation callback, it is called (in the context of the last agent to arrive).
//  AGT_CMD_BARRIER_WAIT,            ///< If the arrival count of the provided barrier is less than the expected arrival count, the agent is blocked until they are equal. Otherwise, this is a noop.
//  AGT_CMD_BARRIER_ARRIVE_AND_WAIT, ///< Equivalent to sending AGT_CMD_BARRIER_ARRIVE immediately followed by AGT_CMD_BARRIER_WAIT
//  AGT_CMD_BARRIER_ARRIVE_AND_DROP, ///<
//  AGT_CMD_ACQUIRE_SEMAPHORE,       ///<
//  AGT_CMD_RELEASE_SEMAPHORE,       ///<
//
//  AGT_CMD_QUERY_UUID,              ///<
//  AGT_CMD_QUERY_NAME,              ///<
//  AGT_CMD_QUERY_DESCRIPTION,       ///<
//  AGT_CMD_QUERY_PRODUCER_COUNT,    ///<
//  AGT_CMD_QUERY_METHOD,            ///<
//
//  AGT_CMD_QUERY_PROPERTY,          ///<
//  AGT_CMD_QUERY_SUPPORT,           ///<
//  AGT_CMD_SET_PROPERTY,            ///<
//  AGT_CMD_WRITE,                   ///<
//  AGT_CMD_READ,                    ///<
//
//  AGT_CMD_FLUSH,                   ///<
//  AGT_CMD_START,                   ///< Sent as the initial message to an eager agent; invokes an agent's start routine
//  AGT_CMD_INVOKE_METHOD_BY_ID,     ///<
//  AGT_CMD_INVOKE_METHOD_BY_NAME,   ///<
//  AGT_CMD_REGISTER_METHOD,         ///<
//  AGT_CMD_UNREGISTER_METHOD,       ///<
//  AGT_CMD_INVOKE_CALLBACK,         ///<
//  AGT_CMD_INVOKE_COROUTINE,        ///<
//  AGT_CMD_REGISTER_HOOK,           ///<
//  AGT_CMD_UNREGISTER_HOOK          ///<
//} agt_agent_cmd_t;
//
//
//
//typedef struct agt_send_info_t {
//
//  agt_message_id_t id;
//  agt_size_t       size;
//  const void*      buffer;
//  agt_async_t*     asyncHandle;
//  agt_send_flags_t flags;
//} agt_send_info_t;
//
//
//
//typedef struct agt_agent_create_info_t {
//  agt_agent_create_flags_t flags;
//  const char*              name;             ///< Optional name. Points to a char buffer of at least nameLength bytes, or, if nameLength=0, is null-terminated. If this parameter is null, the new agent will be anonymous.
//  size_t                   nameLength;       ///< Length of name in characters. If 0, name must be null terminated.
//  size_t                   fixedMessageSize; ///< If not 0, all messages sent to this agent must be equal to or less than fixedMessageSize. If 0, messages of any size may be sent.
//  agt_agent_start_t        initializer;      ///< Initial callback executed in the agent's context before it starts receiving messages.
//  void*                    userData;         ///< The argument passed to the agent's constructor. If the agent does not have a constructor, then this points to the agent state (NOTE: in this case, the memory pointed to by userData must remain valid for the entire lifetime of the agent. May be null if the agent type has no state).
//} agt_agent_create_info_t;
//
//
//
//
//
///**
// * Initializes a library context.
// *
// * Can be configured via environment variables.
// *
// * @param apiVersion Must be AGT_API_VERSION
// *
// *
// * Initialization Variables:
// *
// * Types:
// *      - bool: value is parsed as being either true or false, with "true", "1", "yes", or "on" interpreted as true,
// *              and "false", "0", "no", or "off" interpreted as false. Not case sensitive.
// *              If the variable is defined, but has no value, this is interpreted as true.
// *              If the variable is not defined at all, this is interpreted as false.
// *      - integer: value is parsed as an integer. With no prefix, value is parsed in base 10.
// *                 The prefix "0x" causes the value to be interpreted in hexadecimal.
// *                 The prefix "00" causes the value to be interpreted in octal.
// *                 The prefix "0b" causes the value to be interpreted in binary.
// *      - number: value is parsed as a real number. Can be an integer or a float.
// *      - string: value is parsed as is. As such, the value is case sensitive.
// *
// * |                               Name |    Type   |   Default Value   | Description
// * |------------------------------------|-----------|-------------------|--------------
// * |              AGATE_PRIVATE_CONTEXT |  bool     |       false       | If true, all interprocess capabilities will be disabled for this context. Any attempt to create a shared entity from this context will result in a return code of AGT_ERROR_CANNOT_CREATE_SHARED.
// * |                   AGATE_SHARED_KEY |  string   | agate-default-key | If interprocess capabilities are enabled (as they are by default), this context will be able to communicate with any other contexts that were created with the same key.
// * |     AGATE_CHANNEL_DEFAULT_CAPACITY |  integer  |        255        | When creating a channel, a capacity is specified. The created channel is guaranteed to be able to concurrently store at least that many messages. If the specified capacity is 0, then the default capacity is used, as defined by this variable.
// * | AGATE_CHANNEL_DEFAULT_MESSAGE_SIZE |  integer  |        196        | When creating a channel, a message size is specified. The created channel is guaranteed to be able to send messages of up to the specified size in bytes without dynamic allocation. If the specified size is 0, then the default size is used, as defined by this variable.
// * |   AGATE_CHANNEL_DEFAULT_TIMEOUT_MS |  integer  |       30000       | When creating a channel, a timeout in milliseconds is specified. Any handles to the created channel will be disconnected if there is no activity after the duration specified in this variable. Used as a safeguard against denial of service-esque attacks.
// *
// * TODO: Decide whether or not to allow customized initialization via function parameters
// * */
//AGT_api agt_status_t AGT_stdcall agt_init_(agt_ctx_t* pContext, int apiVersion) AGT_noexcept;
//
//#define agt_init(...) (agt_init_(__VA_ARGS__, AGT_API_VERSION))
//
//
///**
// * Closes the provided context. Behaviour of this function depends on how the library was configured
// *
// * TODO: Decide whether to provide another API call with differing behaviour depending on whether or not
// *       one wishes to wait for processing to finish.
// * */
//AGT_api agt_status_t AGT_stdcall agt_finalize(agt_ctx_t context) AGT_noexcept;
//
///**
// * Returns the API version of the linked library.
// * */
//AGT_api int          AGT_stdcall agt_get_library_version() AGT_noexcept;
//
//
//
//
//
//
//
///***/
//
//AGT_api agt_status_t AGT_stdcall agt_create_busy_agent(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo, agt_agent_t* pAgent) AGT_noexcept;
//
///**
// * Surrenders the current thread to a newly created BusyAgent.
// *
// * A typical pattern would involve creating the initial ecosystem of agents in main,
// * followed by a call to this function to start the execution of the system.
// * */
//AGT_api AGT_noreturn void AGT_stdcall agt_create_busy_agent_on_current_thread(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo) AGT_noexcept;
//
//AGT_api agt_status_t AGT_stdcall agt_create_event_agent(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo, agt_agent_t* pAgent) AGT_noexcept;
//AGT_api agt_status_t AGT_stdcall agt_create_free_event_agent(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo, agt_agent_t* pAgent) AGT_noexcept;
//
//
//
///* ========================= [ Agents ] ========================= */
//
//AGT_agent_api agt_ctx_t    AGT_stdcall agt_current_context() AGT_noexcept;
//
//AGT_agent_api agt_agent_t  AGT_stdcall agt_self() AGT_noexcept;
//
//AGT_agent_api agt_agent_t  AGT_stdcall agt_retain(agt_agent_t agent) AGT_noexcept;
//
//AGT_agent_api agt_status_t AGT_stdcall agt_send(agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept;
//
//AGT_agent_api agt_status_t AGT_stdcall agt_send_as(agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo) AGT_noexcept;
//
//AGT_agent_api agt_status_t AGT_stdcall agt_send_many(const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept;
//
//AGT_agent_api agt_status_t AGT_stdcall agt_send_many_as(agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo) AGT_noexcept;
//
//AGT_agent_api void         AGT_stdcall agt_reply(const agt_send_info_t* pSendInfo) AGT_noexcept;
//
//AGT_agent_api void         AGT_stdcall agt_delegate(agt_agent_t recipient) AGT_noexcept;
//
//AGT_agent_api void         AGT_stdcall agt_release(agt_agent_t agent) AGT_noexcept;
//
//
///**
// * Signals normal termination of an agent.
// *
// * */
//AGT_agent_api void         AGT_stdcall agt_exit(int exitCode) AGT_noexcept;
//
///**
// * Signals abnormal termination
// * */
//AGT_agent_api void         AGT_stdcall agt_abort() AGT_noexcept;
//
//
//
///* ======================= [ Coroutine ] ======================= */
//
//AGT_agent_api void         AGT_stdcall agt_resume_coroutine(agt_agent_t receiver, void* coroutine, agt_async_t* asyncHandle) AGT_noexcept;
//
//
//
///* ======================= [ Semaphore ] ======================= */
//
//
//AGT_api agt_status_t AGT_stdcall agt_new_semaphore(agt_semaphore_t* ) AGT_noexcept;
//
///* ======================== [ Barrier ] ======================== */
//
//
///* ========================= [ Async ] ========================= */
//
///**
// * Initializes a new async object that can store/track the state of asynchronous operations.
// * */
//AGT_api agt_status_t AGT_stdcall agt_new_async(agt_ctx_t ctx, agt_async_t* pAsync, agt_async_flags_t flags) AGT_noexcept;
//
///**
// * Sets pTo to track the same asynchronous operation as pFrom.
// * \param pFrom The source object
// * \param pTo   The destination object
// * \pre - pFrom must be initialized. \n
// *      - pTo does not need to have been initialized, but may be. \n
// *      - If pTo is uninitialized, the memory pointed to by pTo must have been zeroed. \n
// *      \code memset(pTo, 0, sizeof(*pTo));\endcode
// * \post - pTo is initialized. \n
// *       - If before the call, pTo had been bound to a different operation, that operation is abandonned as though by calling agt_clear_async. \n
// *       - If pFrom is bound to an operation, pTo is now bound to the same operation. \n
// *       - pFrom is unmodified
// * \note While this function copies state from pFrom to pTo, any further changes made to pFrom after this call returns are not reflected in pTo.
// * */
//AGT_api void         AGT_stdcall agt_copy_async(const agt_async_t* pFrom, agt_async_t* pTo) AGT_noexcept;
//
///**
// * Optimized equivalent to
// *  \code
// *  agt_copy_async(from, to);
// *  agt_destroy_async(from);
// *  \endcode
// *
// *  \note Eases API mapping to modern C++, where move constructors are important and common.
// * */
//AGT_api void         AGT_stdcall agt_move_async(agt_async_t* from, agt_async_t* to) AGT_noexcept;
//
//AGT_api void         AGT_stdcall agt_clear_async(agt_async_t* async) AGT_noexcept;
//
//AGT_api void         AGT_stdcall agt_destroy_async(agt_async_t* async) AGT_noexcept;
//
//
//
//
///**
// * Optimized equivalent to
// *  \code
// *  agt_wait(async, AGT_DO_NOT_WAIT);
// *  \endcode
// * */
//AGT_api agt_status_t AGT_stdcall agt_async_status(agt_async_t* async) AGT_noexcept;
//
//AGT_api agt_status_t AGT_stdcall agt_wait(agt_async_t* async, agt_timeout_t timeout) AGT_noexcept;
//AGT_api agt_status_t AGT_stdcall agt_wait_all(agt_async_t* const * pAsyncs, agt_size_t asyncCount, agt_timeout_t timeout) AGT_noexcept;
//AGT_api agt_status_t AGT_stdcall agt_wait_any(agt_async_t* const * pAsyncs, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) AGT_noexcept;
//
//
//
//
///* ========================= [ Signal ] ========================= */
//
//AGT_api agt_status_t AGT_stdcall agt_new_signal(agt_ctx_t ctx, agt_signal_t* pSignal) AGT_noexcept;
//AGT_api void         AGT_stdcall agt_attach_signal(agt_signal_t* signal, agt_async_t* async) AGT_noexcept;
///**
// * if waitForN is 0,
// * */
//AGT_api void         AGT_stdcall agt_attach_many_signals(agt_signal_t* const * pSignals, agt_size_t signalCount, agt_async_t* async, size_t waitForN) AGT_noexcept;
//AGT_api void         AGT_stdcall agt_raise_signal(agt_signal_t* signal) AGT_noexcept;
//
///**
// * Equivalent to
// *
// * \code
// * for (agt_size_t i = 0; i < signalCount; ++i)
// *   agt_raise_signal(pSignals[i]);
// * \endcode
// * */
//AGT_api void         AGT_stdcall agt_raise_many_signals(agt_signal_t* const * pSignals, agt_size_t signalCount) AGT_noexcept;
//AGT_api void         AGT_stdcall agt_destroy_signal(agt_signal_t* signal) AGT_noexcept;
//
//
//
//
//
//AGT_end_c_namespace
//
//


#endif//AGATE_AGATE_H
