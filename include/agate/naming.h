//
// Created by maxwe on 2023-01-23.
//

#ifndef AGATE_NAMING_H
#define AGATE_NAMING_H

#include <agate/core.h>



AGT_begin_c_namespace


typedef enum agt_name_flag_bits_t {
  // AGT_NAME_ASYNC_IS_UNINITIALIZED = 0x1,
  AGT_NAME_RETAIN_OBJECT          = 0x1,
} agt_name_flag_bits_t;
typedef agt_flags32_t agt_name_flags_t;

#define AGT_NAME_RETAIN_OBJECT_IF_IS(typeMask) ((((agt_name_flags_t)(typeMask)) << 32) | AGT_NAME_RETAIN_OBJECT)
#define AGT_NAME_RETAIN_ANY_OBJECT AGT_NAME_RETAIN_OBJECT_IF_IS(AGT_ANY_TYPE)

typedef struct agt_name_desc_t {
  agt_async_t*           async;      ///< [optional]
  const char*            pName;      ///< UTF8 encoding, length is equal to nameLength, or if nameLength is 0, should be null terminated
  size_t                 nameLength;
  agt_name_flags_t       flags;      ///< Valid
  agt_object_type_mask_t retainMask; ///< If flags contains AGT_NAME_RETAIN_OBJECT, this must be a set of bitwise-or'd agt_object_type_t values, which act as a filter on which types of objects will be retained (important to be careful here, as any retained object must be subsequently released)
  agt_scope_t            scope;      ///< As always, AGT_SHARED_SCOPE is only valid if AGT_ATTR_SHARED_CONTEXT is true
} agt_name_desc_t;

typedef struct agt_name_filter_info_t {
  agt_async_t*     async;
  const char*      pFilter;
  size_t           filterStringLength;
  agt_scope_mask_t scopes;
} agt_name_filter_info_t;



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
AGT_core_api agt_status_t AGT_stdcall agt_reserve_name(agt_ctx_t ctx, const agt_name_desc_t* pNameDesc, agt_name_t* pResult) AGT_noexcept;

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
AGT_core_api void         AGT_stdcall agt_release_name(agt_ctx_t ctx, agt_name_t name) AGT_noexcept;

/**
 * \brief Binds an object to a previously reserved name.
 *
 * \note nameToken must have been previously obtained from a call to agt_reserve_name,
 *       and must not have already been bound.
 * */
AGT_core_api agt_status_t AGT_stdcall agt_bind_name(agt_ctx_t ctx, agt_name_t name, void* object) AGT_noexcept;


// AGT_core_api agt_status_t AGT_stdcall agt_lookup_named_object(agt_ctx_t ctx, ) AGT_noexcept;


AGT_end_c_namespace

#endif//AGATE_NAMING_H
