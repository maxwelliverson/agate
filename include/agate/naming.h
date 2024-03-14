//
// Created by maxwe on 2023-01-23.
//

#ifndef AGATE_NAMING_H
#define AGATE_NAMING_H

#include <agate/core.h>



AGT_begin_c_namespace


typedef enum agt_name_flag_bits_t {
  // AGT_NAME_ASYNC_IS_UNINITIALIZED = 0x1,
  AGT_NAME_RETAIN_OBJECT = 0x8,
} agt_name_flag_bits_t;
typedef agt_flags32_t agt_name_flags_t;

typedef union agt_name_result_t {
  agt_name_t               name;
  const agt_object_desc_t* object;
} agt_name_result_t;

typedef struct agt_name_desc_t {
  agt_async_t            async;       ///< [optional]
  agt_string_t           name;        ///< See documentation for @refitem agt_string_t
  agt_name_flags_t       flags;       ///< May be any valid bitwise combination of agt_name_flag_bits_t values and an optional (valid) agt_scope_t value.
  agt_object_type_mask_t retainMask;  ///< If flags does not contain AGT_NAME_RETAIN_OBJECT, this field is ignored. Otheriwse, this must be a set of bitwise-or'd agt_object_type_t values, which act as a filter on which types of objects will be retained (important to be careful here, as any retained object must be subsequently released)
  void*                  paramBuffer; ///< If
} agt_name_desc_t;

typedef struct agt_name_filter_info_t {
  agt_async_t            async;
  agt_string_t           filterString;
  agt_scope_mask_t       scopes;
  agt_object_type_mask_t types;
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
*              \code
*              function new_object(desiredName, params)
*                  name = reserve_name(desiredName)
*                  if name is null:
*                      throw name_clash_exception
*                  try:
*                      state = initialize(params)           // expensive state initialization
*                      object = agt_new_object(state, name) // note that this internally calls agt_bind_name
*                  catch:
*                      release_name(name)
*                      rethrow exception
*                  return object
*              \endcode
*
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
*                            bound. A deferred operation has been bound to pNameDesc->async,
*                            which will reattempt the reservation if the name is released, or will
*                            return the binding info the bound object upon binding. \n
*              AGT_ERROR_INVALID_ARGUMENT: Indicates either pReservationDesc or pResult was null,
*                                          which ideally, shouldn't ever be the case. Is also
*                                          returned if pReservationDesc->name.data is null. \n
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
AGT_core_api agt_status_t AGT_stdcall agt_reserve_name(agt_ctx_t ctx, const agt_name_desc_t* pNameDesc, agt_name_result_t* pResult) AGT_noexcept;

// AGT_core_api agt_status_t AGT_stdcall agt_lookup_named_object(agt_ctx_t ctx, ) AGT_noexcept;


/**
 * \brief Releases a name previously reserved by a call to agt_reserve_name.
 *
 * \details This only needs to be called when a previously reserved name will not be
 *          bound to anything. Primarily intended for use in cases where a name is
 *          reserved prior to object creation, and then at some point during
 *          construction, an error is encountered and the creation routine is
 *          abandonned.
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
AGT_core_api agt_status_t AGT_stdcall agt_bind_name(agt_ctx_t ctx, agt_name_t name, agt_object_t object) AGT_noexcept;


// TODO: Figure out a good API for named object lookup/enumeration with pattern matching.
//  This allows for all sorts of very useful behaviours/usage patterns. User defined namespaces,
//  filtering by type-tags, named object collections, etc.

// AGT_core_api agt_status_t AGT_stdcall agt_lookup_named_object(agt_ctx_t ctx, ) AGT_noexcept;


AGT_end_c_namespace

#endif//AGATE_NAMING_H
