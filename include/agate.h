#ifndef AGATE_INCLUDE_H
#define AGATE_INCLUDE_H

/* ====================[ Portability Config ]================== */


#if defined(__cplusplus)
#define AGT_begin_c_namespace extern "C" {
#define AGT_end_c_namespace }

#include <cassert>

#else
#define AGT_begin_c_namespace
#define AGT_end_c_namespace

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
# define AGT_assume_aligned(p, align) (AGT_assume(((jem_u64_t)p) % align == 0), p)
#else
# define AGT_assume_aligned(p, align) p
#endif




/* =================[ Constants ]================= */


#define AGT_DONT_CARE          ((jem_u64_t)-1)
#define AGT_NULL_HANDLE        ((void*)0)

#define AGT_CACHE_LINE         ((jem_size_t)1 << 6)
#define AGT_PHYSICAL_PAGE_SIZE ((jem_size_t)1 << 12)
#define AGT_VIRTUAL_PAGE_SIZE  ((jem_size_t)(1 << 16))



/* =================[ Types ]================= */


typedef unsigned char      jem_u8_t;
typedef   signed char      jem_i8_t;
typedef          char      jem_char_t;
typedef unsigned short     jem_u16_t;
typedef   signed short     jem_i16_t;
typedef unsigned int       jem_u32_t;
typedef   signed int       jem_i32_t;
typedef unsigned long long jem_u64_t;
typedef   signed long long jem_i64_t;



typedef size_t             jem_size_t;
typedef size_t             jem_address_t;
typedef ptrdiff_t          jem_ptrdiff_t;


typedef jem_u32_t          jem_flags32_t;
typedef jem_u64_t          jem_flags64_t;


typedef jem_u32_t          jem_local_id_t;
typedef jem_u64_t          jem_global_id_t;


typedef void*              jem_handle_t;



#define AGT_INVALID_OBJECT_ID ((AgtObjectId)-1)
#define AGT_SYNCHRONIZE ((AgtAsync)AGT_NULL_HANDLE)



#if AGT_system_windows
# define AGT_NATIVE_TIMEOUT_CONVERSION 10
#else
# define AGT_NATIVE_TIMEOUT_CONVERSION 1000
#endif

#define AGT_TIMEOUT(microseconds) ((AgtTimeout)(microseconds * AGT_NATIVE_TIMEOUT_CONVERSION))
#define AGT_DO_NOT_WAIT ((AgtTimeout)0)
#define AGT_WAIT ((AgtTimeout)-1)


#define AGT_DEFINE_HANDLE(type) typedef struct type##_st* type
#define AGT_DEFINE_DISPATCH_HANDLE(type) typedef struct type##_st* type


AGT_begin_c_namespace


typedef jem_i32_t AgtBool;

typedef jem_u8_t  AgtUInt8;
typedef jem_i8_t  AgtInt8;
typedef jem_u16_t AgtUInt16;
typedef jem_i16_t AgtInt16;
typedef jem_u32_t AgtUInt32;
typedef jem_i32_t AgtInt32;
typedef jem_u64_t AgtUInt64;
typedef jem_i64_t AgtInt64;

typedef jem_u64_t AgtSize;
typedef jem_u64_t AgtTimeout;

typedef jem_u32_t AgtProtocolId;
typedef jem_u64_t AgtMessageId;
typedef jem_u64_t AgtTypeId;
typedef jem_u64_t AgtObjectId;

typedef jem_u64_t AgtSendToken;
typedef jem_u64_t AgtNameToken;



typedef void*     AgtHandle;

AGT_DEFINE_HANDLE(AgtContext);
AGT_DEFINE_HANDLE(AgtAsync);
AGT_DEFINE_HANDLE(AgtSignal);
AGT_DEFINE_HANDLE(AgtMessage);
AGT_DEFINE_HANDLE(AgtDynamicMessage);









typedef enum AgtStatus {
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
  AGT_ERROR_INVALID_OPERATION /** < The given handle does not implement the called function. eg. agtReceiveMessage cannot be called on a channel sender*/,
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
} AgtStatus;

typedef enum AgtHandleType {
  AGT_CHANNEL,
  AGT_AGENT,
  AGT_SOCKET,
  AGT_SENDER,
  AGT_RECEIVER,
  AGT_THREAD,
  AGT_AGENCY
} AgtHandleType;

typedef enum AgtHandleFlagBits {
  AGT_OBJECT_IS_SHARED = 0x1,
  AGT_HANDLE_FLAGS_MAX
} AgtHandleFlagBits;
typedef jem_flags32_t AgtHandleFlags;

typedef enum AgtSendFlagBits {
  AGT_SEND_WAIT_FOR_ANY      = 0x00 /**< The asynchronous operation will be considered complete as soon as a single consumer processes the message (Default behaviour)*/,
  AGT_SEND_WAIT_FOR_ALL      = 0x01 /**< The asynchronous operation will be considered complete once all consumers have processed the message */,
  AGT_SEND_BROADCAST_MESSAGE = 0x02 /**< The message will be received by all consumers rather than just one*/,
  AGT_SEND_DATA_CSTRING      = 0x04 /**< The message data is in the form of a null terminated string. When this flag is specified, messageSize can be 0, in which case the messageSize is interpreted to be strlen(data) + 1*/,
  AGT_SEND_WITH_TIMESTAMP    = 0x08 /**< */,
  AGT_SEND_FAST_CLEANUP      = 0x10 /**< */,
} AgtSendFlagBits;
typedef jem_flags32_t AgtSendFlags;

typedef enum AgtMessageFlagBits {
  AGT_MESSAGE_DATA_IS_CSTRING   = 0x1,
  AGT_MESSAGE_HAS_ID            = 0x2,
  AGT_MESSAGE_IS_FOREIGN        = 0x4,
  AGT_MESSAGE_HAS_RETURN_HANDLE = 0x8
} AgtMessageFlagBits;
typedef jem_flags32_t AgtMessageFlags;

typedef enum AgtConnectFlagBits {
  AGT_CONNECT_FORWARD   = 0x1,
} AgtConnectFlagBits;
typedef jem_flags32_t AgtConnectFlags;

typedef enum AgtBlockingThreadCreateFlagBits {
  AGT_BLOCKING_THREAD_COPY_USER_DATA   = 0x1,
  AGT_BLOCKING_THREAD_USER_DATA_STRING = 0x2
} AgtBlockingThreadCreateFlagBits;
typedef jem_flags32_t AgtBlockingThreadCreateFlags;

typedef enum AgtScope {
  AGT_SCOPE_LOCAL,
  AGT_SCOPE_SHARED,
  AGT_SCOPE_PRIVATE
} AgtScope;

typedef enum AgtMessageAction {
  AGT_COMPLETE_MESSAGE,
  AGT_DISCARD_MESSAGE // idk
} AgtMessageAction;

typedef enum AgtAgencyAction {
  AGT_AGENCY_ACTION_CONTINUE,
  AGT_AGENCY_ACTION_DEFER,
  AGT_AGENCY_ACTION_YIELD,
  AGT_AGENCY_ACTION_CLOSE
} AgtAgencyAction;



typedef struct AgtSendInfo {
  AgtSendFlags flags;
  AgtAsync     asyncHandle;
  AgtHandle    returnHandle;
  AgtMessageId messageId;
  AgtSize      messageSize;
  const void*  pMessageBuffer;
} AgtSendInfo;

typedef struct AgtStagedMessage {
  void*        reserved[4];
  AgtHandle    returnHandle;
  AgtSize      messageSize;
  AgtMessageId id;
  void*        payload;
} AgtStagedMessage;

typedef struct AgtMessageInfo {
  AgtMessage      message;
  AgtSize         size;
  AgtMessageFlags flags;
} AgtMessageInfo;

typedef struct AgtMultiFrameMessageInfo {
  void*      reserved[5];
  AgtSize    messageSize;
  AgtSize    frameSize;
  AgtSize    frameCount;
} AgtMultiFrameMessageInfo;

typedef struct AgtMessageFrame {
  AgtSize index;
  AgtSize size;
  void*   data;
} AgtMessageFrame;

typedef struct AgtObjectInfo {
  AgtObjectId    id;
  AgtHandle      handle;
  AgtHandleType  type;
  AgtHandleFlags flags;
  AgtObjectId    exportId;
} AgtObjectInfo;

typedef struct AgtProxyObject {
  void* reserved[4];

} AgtProxyObject;

typedef struct AgtProxyObjectFunctions {
  AgtStatus (* const acquireMessage)(void* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept;
  void      (* const pushQueue)(void* object, AgtMessage message, AgtSendFlags flags) noexcept;
  AgtStatus (* const popQueue)(void* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept;
  void      (* const releaseMessage)(void* object, AgtMessage message) noexcept;
  AgtStatus (* const connect)(void* object, AgtHandle handle, AgtConnectFlags flags) noexcept;
  AgtStatus (* const acquireRef)(void* object) noexcept;
  AgtSize   (* const releaseRef)(void* object) noexcept;
  void      (* const destroy)(void* object) noexcept;
} AgtProxyObjectFunctions;


typedef AgtStatus (*PFN_agtActorMessageProc)(void* actorState, const AgtMessageInfo* message);
typedef void      (*PFN_agtActorDtor)(void* actorState);


// TODO: Add a function to the public API that extends the lifetime of a message, and one that ends that extension.
//       Having these functions in place, messages can be automatically returned after use. Possibly with reference counting??
typedef void      (*PFN_agtBlockingThreadMessageProc)(AgtHandle threadHandle, const AgtMessageInfo* message, void* pUserData);
typedef void      (*PFN_agtUserDataDtor)(void* pUserData);

typedef struct AgtActor {
  AgtTypeId               type;
  PFN_agtActorMessageProc pfnMessageProc;
  PFN_agtActorDtor        pfnDtor;
  void*                   state;
} AgtActor;






typedef struct AgtChannelCreateInfo {
  AgtSize     minCapacity;
  AgtSize     maxMessageSize;
  AgtSize     maxSenders;
  AgtSize     maxReceivers;
  AgtScope    scope;
  AgtAsync    asyncHandle;
  const char* name;
  AgtSize     nameLength;
} AgtChannelCreateInfo;
typedef struct AgtSocketCreateInfo {

} AgtSocketCreateInfo;
typedef struct AgtAgentCreateInfo {

} AgtAgentCreateInfo;
typedef struct AgtAgencyCreateInfo {

} AgtAgencyCreateInfo;
typedef struct AgtBlockingThreadCreateInfo {
  PFN_agtBlockingThreadMessageProc pfnMessageProc;
  AgtScope                         scope;
  AgtBlockingThreadCreateFlags     flags;
  AgtSize                          minCapacity;
  AgtSize                          maxMessageSize;
  AgtSize                          maxSenders;
  union {
    AgtSize                        dataSize;
    PFN_agtUserDataDtor            pfnUserDataDtor;
  };
  void*                            pUserData;
  const char*                      name;
  AgtSize                          nameLength;
} AgtBlockingThreadCreateInfo;
typedef struct AgtThreadCreateInfo {

} AgtThreadCreateInfo;



/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/* ========================= [ Context ] ========================= */

/**
 *
 * */
AGT_api AgtStatus     AGT_stdcall agtNewContext(AgtContext* pContext) AGT_noexcept;
/**
 *
 * */
AGT_api AgtStatus     AGT_stdcall agtDestroyContext(AgtContext context) AGT_noexcept;




/* ========================= [ Handles ] ========================= */

/**
 *
 * */
AGT_api AgtStatus     AGT_stdcall agtGetObjectInfo(AgtContext context, AgtObjectInfo* pObjectInfo) AGT_noexcept;
/**
 *
 * */
AGT_api AgtStatus     AGT_stdcall agtDuplicateHandle(AgtHandle inHandle, AgtHandle* pOutHandle) AGT_noexcept;
/**
 *
 * */
AGT_api void          AGT_stdcall agtCloseHandle(AgtHandle handle) AGT_noexcept;


/**
 *
 * */
AGT_api AgtStatus     AGT_stdcall agtCreateChannel(AgtContext context, const AgtChannelCreateInfo* cpCreateInfo, AgtHandle* pSender, AgtHandle* pReceiver) AGT_noexcept;
AGT_api AgtStatus     AGT_stdcall agtCreateAgent(AgtContext context, const AgtAgentCreateInfo* cpCreateInfo, AgtHandle* pAgent) AGT_noexcept;
AGT_api AgtStatus     AGT_stdcall agtCreateAgency(AgtContext context, const AgtAgencyCreateInfo* cpCreateInfo, AgtHandle* pAgency) AGT_noexcept;
AGT_api AgtStatus     AGT_stdcall agtCreateThread(AgtContext context, const AgtThreadCreateInfo* cpCreateInfo, AgtHandle* pThread) AGT_noexcept;


/**
 * pSendInfo must not be null.
 * If sender is null, returns AGT_ERROR_NULL_HANDLE
 * If pSendInfo->messageSize is zero, agtGetSendToken will write the handle's maximum message size to pSendInfo->messageSize.
 * If pSendInfo->messageSize is not zero, its value will not be modified. In that case, if the value of pSendInfo->messageSize is greater than the handle's maximum message size, AGT_ERROR_MESSAGE_TOO_LARGE is returned.
 * If timeout is AGT_DO_NOT_WAIT, the call immediately returns AGT_ERROR_MAILBOX_IS_FULL if no send tokens are available.
 * If timeout is AGT_WAIT, the call waits indefinitely until a send token is available. In this case, AGT_ERROR_MAILBOX_IS_FULL is never returned.
 * If the timeout is some positive integer N, if after N microseconds, no send tokens have been made available, AGT_ERROR_MAILBOX_IS_FULL is returned.
 *
 * If AGT_SUCCESS is returned, a valid send token will have been written to *pSendToken
 * This send token must then be used in a call to agtSendWithToken.
 */
AGT_api AgtStatus     AGT_stdcall agtGetSendToken(AgtHandle sender, AgtSendInfo* pSendInfo, AgtSendToken* pSendToken, AgtTimeout timeout) AGT_noexcept;
/**
 * The value of sendToken must have previously been acquired from a successful call to agtGetSendToken.
 * If the value of pSendInfo->messageSize has changed since the call to agtGetSendToken, N bytes will be copied from the buffer pointed to by cpSendInfo->pMessageBuffer,
 *     where N is equal to the minimum of cpSendInfo->messageSize and the sender handle's maximum supported message size.
 * For speed purposes, this function does not do any error checking. It is up to the caller to ensure proper usage.
 *
 * sender must be the same handle used in the prior call to agtGetSendToken from which sendToken was returned.
 * cpSendInfo must point to the same struct pointed to by the pSendInfo parameter of the prior call to agtGetSendToken.
 * sendToken must have been returned from a successful prior call to agtGetSendToken.
 * cpSendInfo->messageSize must not be greater than the maximum message size supported by sender.
 * cpSendInfo->pMessageBuffer must point to a valid, readable buffer of at least cpSendInfo->messageSize bytes.
 * cpSendInfo->messageId is optional.
 * cpSendInfo->asyncHandle must either be null, or be a valid handle obtained from a call to agtNewAsync. If the asyncHandle was already attached to an asynchronous operation, that operation is dropped and replaced.
 *
 * If the sender object has set a "returnHandle" property, cpSendInfo->returnHandle is ignored.
 * Otherwise, if the caller does not wish to provide a return handle (or simply has no need), ensure cpSendInfo->returnHandle is set to AGT_NULL_HANDLE.
 *
 * This call "consumes" sendToken, and sendToken must not be used for any other calls.
 */
AGT_api void          AGT_stdcall agtSendWithToken(AgtHandle sender, const AgtSendInfo* cpSendInfo, AgtSendToken sendToken) AGT_noexcept;
/**
 * Essentially performs agtGetSendToken and agtSendWithToken in a single call, allowing for fewer API calls and simpler control flow,
 * but removing the ability to ensure the send operation will be successful before preparing the buffer. All the same
 * conditions for use as agtSendWithToken are true here.
 *
 * In the case that the operation fails, the async handle will not be attached (and moreover, won't be detatched from a prior operation),
 * no buffer write will occur, etc. etc.
 *
 */
AGT_api AgtStatus     AGT_stdcall agtSend(AgtHandle sender, const AgtSendInfo* cpSendInfo, AgtTimeout timeout) AGT_noexcept;


/**
 *
 */
AGT_api AgtStatus     AGT_stdcall agtReceiveMessage(AgtHandle receiver, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) AGT_noexcept;

/**
 * Prerequisites:
 *  - message is a valid handle obtained from a prior call to agtReceiveMessage
 *  - buffer is a valid pointer to a memory buffer with at least bufferLength bytes of data
 *
 * If bufferLength is less than the value returned in pMessageInfo->size from the call to agtReceiveMessage,
 * this will return AGT_ERROR_INSUFFICIENT_SIZE (?).
 */
AGT_api AgtStatus     AGT_stdcall agtReadMessageData(AgtMessage message, AgtSize bufferLength, void* buffer) AGT_noexcept;



/**
 * Close message
 */
AGT_api void          AGT_stdcall agtCloseMessage(AgtHandle owner, AgtMessage message) AGT_noexcept;

/**
 *
 */
AGT_api AgtStatus     AGT_stdcall agtConnect(AgtHandle to, AgtHandle from, AgtConnectFlags flags) AGT_noexcept;






/* ========================= [ Messages ] ========================= */

AGT_api AgtStatus     AGT_stdcall agtGetMultiFrameMessage(AgtMessage message, AgtMultiFrameMessageInfo* pMultiFrameInfo) AGT_noexcept;
AGT_api AgtStatus     AGT_stdcall agtGetNextFrame(AgtMultiFrameMessageInfo* pMultiFrameInfo, AgtMessageFrame* pFrame) AGT_noexcept;


AGT_api void          AGT_stdcall agtReturn(AgtMessage message, AgtStatus status) AGT_noexcept;




AGT_api AgtStatus     AGT_stdcall agtDispatchMessage(const AgtActor* pActor, const AgtMessageInfo* pMessageInfo) AGT_noexcept;
// AGT_api AgtStatus     AGT_stdcall agtExecuteOnThread(AgtThread thread, ) AGT_noexcept;





AGT_api AgtStatus     AGT_stdcall agtGetSenderHandle(AgtMessage message, AgtHandle* pSenderHandle) AGT_noexcept;






/* ========================= [ Async ] ========================= */

AGT_api AgtStatus     AGT_stdcall agtNewAsync(AgtContext ctx, AgtAsync* pAsync) AGT_noexcept;
AGT_api void          AGT_stdcall agtCopyAsync(AgtAsync from, AgtAsync to) AGT_noexcept;
AGT_api void          AGT_stdcall agtClearAsync(AgtAsync async) AGT_noexcept;
AGT_api void          AGT_stdcall agtDestroyAsync(AgtAsync async) AGT_noexcept;

AGT_api AgtStatus     AGT_stdcall agtWait(AgtAsync async, AgtTimeout timeout) AGT_noexcept;
AGT_api AgtStatus     AGT_stdcall agtWaitMany(const AgtAsync* pAsyncs, AgtSize asyncCount, AgtTimeout timeout) AGT_noexcept;





/* ========================= [ Signal ] ========================= */

AGT_api AgtStatus     AGT_stdcall agtNewSignal(AgtContext ctx, AgtSignal* pSignal) AGT_noexcept;
AGT_api void          AGT_stdcall agtAttachSignal(AgtSignal signal, AgtAsync async) AGT_noexcept;
AGT_api void          AGT_stdcall agtRaiseSignal(AgtSignal signal) AGT_noexcept;
AGT_api void          AGT_stdcall agtRaiseManySignals(const AgtSignal* pSignals, AgtSize signalCount) AGT_noexcept;
AGT_api void          AGT_stdcall agtDestroySignal(AgtSignal signal) AGT_noexcept;



// AGT_api AgtStatus     AGT_stdcall agtGetAsync



// AGT_api void          AGT_stdcall agtYieldExecution() AGT_noexcept;




AGT_end_c_namespace

#endif//AGATE_INCLUDE_H
