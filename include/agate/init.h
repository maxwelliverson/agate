//
// Created by maxwe on 2023-01-13.
//

#ifndef AGATE_INIT_H
#define AGATE_INIT_H

#include <agate/core.h>

/* =================[ Types ]================= */


#define AGT_ROOT_CONFIG ((agt_config_t)AGT_NULL_HANDLE)

#define AGT_CORE_MODULE_NAME "core"
#define AGT_LOG_MODULE_NAME "log"
#define AGT_NETWORK_MODULE_NAME "network"
#define AGT_METHODS_MODULE_NAME "methods"
#define AGT_SHMEM_MODULE_NAME "shmem"
#define AGT_POOL_MODULE_NAME "pool"


AGT_begin_c_namespace





typedef agt_flags64_t agt_init_flag_bits_t;


#define AGT_INIT_FLAG_BIT(bit) (((agt_init_flag_bits_t)(bit)) << 32)

static const agt_init_flag_bits_t AGT_INIT_AGENTS_MODULE   = 0x01;
static const agt_init_flag_bits_t AGT_INIT_ASYNC_MODULE    = 0x02;
static const agt_init_flag_bits_t AGT_INIT_CHANNELS_MODULE = 0x04;
static const agt_init_flag_bits_t AGT_INIT_LOG_MODULE      = 0x08;
static const agt_init_flag_bits_t AGT_INIT_NETWORK_MODULE  = 0x10;
static const agt_init_flag_bits_t AGT_INIT_POOL_MODULE     = 0x20;
static const agt_init_flag_bits_t AGT_INIT_BASIC_MODULES   = AGT_INIT_ASYNC_MODULE | AGT_INIT_AGENTS_MODULE | AGT_INIT_CHANNELS_MODULE | AGT_INIT_POOL_MODULE;
static const agt_init_flag_bits_t AGT_INIT_ALL_MODULES     = AGT_INIT_AGENTS_MODULE | AGT_INIT_ASYNC_MODULE | AGT_INIT_CHANNELS_MODULE | AGT_INIT_LOG_MODULE | AGT_INIT_NETWORK_MODULE | AGT_INIT_POOL_MODULE;
static const agt_init_flag_bits_t AGT_INIT_SINGLE_THREADED = AGT_INIT_FLAG_BIT(0x1);

#define AGT_MODULE_BITMASK 0x00000000FFFFFFFF


typedef enum agt_init_scope_t {
  AGT_INIT_SCOPE_DEFAULT,
  AGT_INIT_SCOPE_PROCESS_WIDE = 0,
  AGT_INIT_SCOPE_LIBRARY_ONLY
} agt_init_scope_t;

typedef enum agt_init_necessity_t {
  AGT_INIT_REQUIRED, // If the desired option/setting is not available/possible, initialization fails.
  AGT_INIT_OPTIONAL, // If the desired configuration is not possible, initialization continues, but an error message is emitted. If the requested configuration is possible, it must be used.
  AGT_INIT_HINT,     // If the desired configuration is not possible, initialization continues and a warning message is emitted. Even if the desired configuration is possible, the implementation may choose to ignore the hint if it is determined to be suboptimal.
  AGT_INIT_UNNECESSARY
} agt_init_necessity_t;

typedef agt_flags64_t agt_init_flags_t;

typedef enum agt_config_id_t {
  AGT_CONFIG_ASYNC_STRUCT_SIZE,              ///< type: UINT32
  AGT_CONFIG_THREAD_COUNT,                   ///< type: UINT32
  AGT_CONFIG_SEARCH_PATH,
  AGT_CONFIG_LIBRARY_VERSION,
  AGT_CONFIG_IS_SHARED,
  AGT_CONFIG_SHARED_NAMESPACE,
  AGT_CONFIG_CHANNEL_DEFAULT_CAPACITY,
  AGT_CONFIG_CHANNEL_DEFAULT_MESSAGE_SIZE,
  AGT_CONFIG_CHANNEL_DEFAULT_TIMEOUT_MS,
  AGT_CONFIG_DURATION_GRANULARITY_NS,
  AGT_CONFIG_FIXED_CHANNEL_SIZE_GRANULARITY,
  AGT_CONFIG_STACK_SIZE_ALIGNMENT,
  AGT_CONFIG_DEFAULT_THREAD_STACK_SIZE,
  AGT_CONFIG_DEFAULT_FIBER_STACK_SIZE,
  AGT_CONFIG_STATE_SAVE_MASK,
  AGT_CONFIG_DEFAULT_ALLOCATOR_PARAMS
} agt_config_id_t;

typedef enum agt_config_flag_bits_t {
  AGT_VALUE_EQUALS                 = 0x0001, ///< This is the default if no other relational flags are specified.
  AGT_VALUE_LESS_THAN              = 0x0002,
  AGT_VALUE_LESS_THAN_OR_EQUALS    = 0x0003,
  AGT_VALUE_GREATER_THAN           = 0x0004,
  AGT_VALUE_GREATER_THAN_OR_EQUALS = 0x0005,
  AGT_VALUE_NOT_EQUALS             = 0x0006,
  AGT_ALLOW_ENVIRONMENT_OVERRIDE   = 0x10,
  AGT_ALLOW_SUBMODULE_OVERRIDE     = 0x20,
} agt_config_flag_bits_t;
typedef agt_flags32_t agt_config_flags_t;

typedef struct agt_config_option_t {
  agt_config_id_t      id;
  agt_config_flags_t   flags;
  agt_init_necessity_t necessity;
  agt_value_type_t     type;
  agt_value_t          value;
} agt_config_option_t;

typedef struct agt_user_module_info_t {
  agt_string_t         name;
  const void*          searchPath;
  agt_value_type_t     searchPathType; ///< Either STRING or WIDE_STRING
  agt_init_necessity_t necessity;
  int                  minVersion;
  int                  maxVersion;
  const void*          userData;
} agt_user_module_info_t;

/*typedef struct agt_init_info_t {
  agt_init_flags_t              flags;
  int                           headerVersion;          ///< \b must be set to AGT_API_VERSION
  const agt_init_attr_t*        attributes;             ///< [optional] If not null, attributeCount must not be 0
  agt_size_t                    attributeCount;         ///< [optional] Ignored if attributes is null
  const agt_user_module_info_t* userModuleInfos;        ///< [optional] If not null, userModuleInfoCount must not be 0
  agt_size_t                    userModuleInfoCount;    ///< [optional] Ignored if userModuleInfos is null
  const agt_allocator_params_t* defaultAllocatorParams; ///< [optional] If not null,
  agt_internal_log_handler_t    internalLogHandler;     ///< [optional] Set the global internal log handler. If this is not null, the AGT_INIT_LOG_MODULE bit of flags must be set. Individual contexts may set their own log callback. Only handles internal logging
  void*                         logHandlerUserData;     ///< [optional] Ignored if internalLogHandler is null
} agt_init_info_t;*/



typedef struct agt_config_st* agt_config_t;



typedef void (* agt_init_callback_t)(agt_ctx_t ctx, void* pUserData);


/* =================[ Config ]================= */




/**
 * Returns the API version of the static loader.
 *
 * \note See \ref AGT_MAKE_VERSION for the version format
 */
AGT_static_api int          AGT_stdcall agt_get_loader_version() AGT_noexcept;


/**
 * \defgroup ConfigFuncs Library Configuration
 * @{
 */

/// \cond HIDDEN_API
/**
 * Equivalent to \code agt_init(pCtx, agt_get_config(AGT_ROOT_CONFIG, headerVersion)) \endcode
 *
 * @param [in] config Either a parent config object (provided by the component to which this component is linked),
 *                    or AGT_ROOT_CONFIG if there is no parent (ie. this is being called by the executable)
 * @param [in] headerVersion Must be AGT_API_VERSION
 * */
AGT_static_api agt_config_t AGT_stdcall _agt_get_config(agt_config_t config, int headerVersion) AGT_noexcept;
/// \endcond
/**
 * Get a configuration object that may be used to configure the library, and subsequently
 * passed to either \ref agt_init or \ref agt_init_with_callback.
 *
 * @param [in] config Either a parent config object (provided by the component to which this component is linked),
 *                    or AGT_ROOT_CONFIG if there is no parent (ie. this is being called by the executable)
 */
#define agt_get_config(cfg) _agt_get_config(cfg, AGT_API_VERSION)


/**
 *
 */
AGT_static_api void         AGT_stdcall agt_config_init_modules(agt_config_t config, agt_init_necessity_t necessity, size_t moduleCount, const char* const* pModules) AGT_noexcept;

/**
 *
 */
AGT_static_api void         AGT_stdcall agt_config_init_user_modules(agt_config_t config, size_t userModuleCount, const agt_user_module_info_t* pUserModuleInfos) AGT_noexcept;

/**
 *
 */
AGT_static_api void         AGT_stdcall agt_config_set_options(agt_config_t config, size_t optionCount, const agt_config_option_t* pConfigOptions) AGT_noexcept;

/**
 *
 */
AGT_static_api void         AGT_stdcall agt_config_set_log_handler(agt_config_t config, agt_init_scope_t scope, agt_internal_log_handler_t handlerProc, void* userData) AGT_noexcept;


/**
 * @}
 */


/*!
 * \defgroup LibraryInitFunctions Initialization
 * These functions each initialize the agate library, but with slightly different interfaces.
 * @{
 */


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
 * */
AGT_static_api agt_status_t AGT_stdcall agt_init(agt_ctx_t* pLocalContext, agt_config_t config) AGT_noexcept;


/**
 * Initializes agate for the local component, installing a callback to be invoked on upon successful initialization.
 */
AGT_static_api agt_status_t AGT_stdcall agt_init_with_callback(agt_config_t config, agt_init_callback_t callback, void* userData) AGT_noexcept;



/// \cond HIDDEN_API
/**
 * Equivalent to \code agt_init(pCtx, agt_get_config(AGT_ROOT_CONFIG, headerVersion)) \endcode
 *
 * @param [out] pCtx pointer to which
 * @param [in]  headerVersion Must be AGT_API_VERSION
 * */
AGT_static_api agt_status_t AGT_stdcall _agt_default_init(agt_ctx_t* pCtx, int headerVersion) AGT_noexcept;
/// \endcond
/**
 *
 *
 *
 * @param [out] pCtx
 *
 * @result A value of type agt_status_t indicating the result of the initialization
 */
#define agt_default_init(pCtx) _agt_default_init(pCtx, AGT_API_VERSION)



/// \cond HIDDEN_API
/**
 * Equivalent to \code agt_init(pCtx, _agt_get_config(AGT_ROOT_CONFIG, headerVersion)) \endcode
 *
 * @param [in]  headerVersion Must be AGT_API_VERSION
 * @param [in]  callback A callback function that is invoked on success
 * @param [in]  userData An opaque pointer that is passed to the callback
 * */
AGT_static_api agt_status_t AGT_stdcall _agt_default_init_with_callback(int headerVersion, agt_init_callback_t callback, void* userData) AGT_noexcept;
/// \endcond
/**
 *
 */
#define agt_default_init_with_callback(callback, userData) _agt_default_init_with_callback(AGT_API_VERSION, callback, userData)


/**
 * @}
 */


AGT_end_c_namespace

#endif//AGATE_INIT_H
