//
// Created by maxwe on 2023-01-13.
//

#ifndef AGATE_INIT_H
#define AGATE_INIT_H

#include <agate/core.h>

/* =================[ Types ]================= */

AGT_begin_c_namespace


typedef struct agt_allocator_params_t {
  const agt_size_t*    blockSizes;              ///< length: blockSizeCount
  const agt_size_t*    blocksPerChunk;          ///< length: blockSizeCount
  size_t               blockSizeCount;
  const agt_size_t (*  partneredBlockSizes)[2]; ///< length: partneredBlockSizeCount
  size_t               partneredBlockSizeCount;
  const agt_size_t*    soloBlockSizes;          ///< length: soloBlockSizeCount
  size_t               soloBlockSizeCount;
  size_t               maxChunkSize;
} agt_allocator_params_t;


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

typedef agt_flags64_t agt_init_flags_t;

typedef enum agt_init_attr_flag_bits_t {
  AGT_INIT_ATTR_ALLOW_ENVIRONMENT_OVERRIDE = 0x1,
  AGT_INIT_ATTR_HINT                       = 0x10,
  AGT_INIT_ATTR_REQUIRED                   = 0x20,
  AGT_INIT_ATTR_EQUALS                     = 0x100,
  AGT_INIT_ATTR_GREATER_THAN               = 0x200,
  AGT_INIT_ATTR_LESS_THAN                  = 0x400,
  AGT_INIT_ATTR_WITHIN_RANGE               = 0x800
} agt_init_attr_flag_bits_t;
typedef agt_flags32_t agt_init_attr_flags_t;

typedef struct agt_init_attr_t {
  agt_attr_t            value;
  agt_init_attr_flags_t flags;
} agt_init_attr_t;

typedef struct agt_user_module_info_t {
  agt_name_t      name;
  const void*     searchPath;
  agt_attr_type_t searchPathType; ///< Either STRING or WIDE_STRING
  int             minVersion;
  int             maxVersion;
  const void*     userData;
} agt_user_module_info_t;

typedef struct agt_init_info_t {
  agt_init_flags_t              flags;
  int                           headerVersion;          ///< \b must be set to AGT_API_VERSION
  const agt_init_attr_t*        attributes;             ///< [optional] If not null, attributeCount must not be 0
  agt_size_t                    attributeCount;         ///< [optional] Ignored if attributes is null
  const agt_user_module_info_t* userModuleInfos;        ///< [optional] If not null, userModuleInfoCount must not be 0
  agt_size_t                    userModuleInfoCount;    ///< [optional] Ignored if userModuleInfos is null
  const agt_allocator_params_t* defaultAllocatorParams; ///< [optional] If not null,
  agt_log_callback_t            logCallback;            ///< [optional] Set the global logging callback. If this is not null, the AGT_INIT_LOG_MODULE bit of flags must be set. Individual contexts may set their own log callback. Only handles internal logging
  void*                         logCallbackUserData;
} agt_init_info_t;


/* =================[ Library Instance ]================= */


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
AGT_api agt_status_t        AGT_stdcall agt_init(agt_ctx_t* pLocalContext, const agt_init_info_t* pInitInfo) AGT_noexcept;


/**
 * Uses an already initialized library instance to initialize Agate. Useful for
 * dynamically loaded libraries that wish to use the same instance as their clients
 * */
AGT_api agt_status_t        AGT_stdcall agt_init_from_instance(agt_instance_t instance, const agt_init_info_t* pInitInfo) AGT_noexcept;


/**
 * Closes the provided context. Behaviour of this function depends on how the library was configured
 *
 * TODO: Decide whether to provide another API call with differing behaviour depending on whether or not
 *       one wishes to wait for processing to finish.
 * */
AGT_api agt_status_t        AGT_stdcall agt_finalize(agt_ctx_t context) AGT_noexcept;



AGT_end_c_namespace

#endif//AGATE_INIT_H
