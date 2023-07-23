//
// Created by maxwe on 2022-12-20.
//

#include "config.hpp"
#include "async.hpp"



#define AGT_return_type            agt_status_t


#undef  AGT_exported_function_name
#define AGT_exported_function_name async_status


AGT_export_family {

  AGT_function_entry(p_a40)(agt_async_t* async) {

  }

  AGT_function_entry(s_a40)(agt_async_t* async) {

  }

  AGT_function_entry(g_p_a40)(agt_async_t* async) {

  }

  AGT_function_entry(g_s_a40)(agt_async_t* async) {

  }

}




#undef  AGT_exported_function_name
#define AGT_exported_function_name wait


AGT_export_family {

  AGT_function_entry(p_a40)(agt_async_t* async_,  agt_timeout_t timeout) {
    if (!async_) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;
    // AGT_assert( async_ != nullptr );
    /// TODO: Decide whether or not we wish to check for null agt_async_t* pointers,
    ///       or if we want to let it simply be undefined behaviour, placing the burden
    ///       on the caller to ensure async_ is never null

    return agt::async_wait<false, false, 40>((agt::async&)*async_, timeout);
  }

  AGT_function_entry(s_a40)(agt_async_t* async_,  agt_timeout_t timeout) {
    if (!async_) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;
    return agt::async_wait<false, true, 40>((agt::async&)*async_, timeout);
  }

  AGT_function_entry(g_p_a40)(agt_async_t* async_, agt_timeout_t timeout) {
    if (!async_) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;
    return agt::async_wait<true, false, 40>((agt::async&)*async_, timeout);
  }

  AGT_function_entry(g_s_a40)(agt_async_t* async_, agt_timeout_t timeout) {
    if (!async_) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;
    return agt::async_wait<true, true, 40>((agt::async&)*async_, timeout);
  }

}


#undef  AGT_exported_function_name
#define AGT_exported_function_name wait_all


AGT_export_family {

  AGT_function_entry(p_a40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_timeout_t timeout) {

  }

  AGT_function_entry(s_a40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_timeout_t timeout) {

  }

  AGT_function_entry(gpa40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_timeout_t timeout) {

  }

  AGT_function_entry(gsa40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_timeout_t timeout) {

  }

}


#undef  AGT_exported_function_name
#define AGT_exported_function_name wait_any


AGT_export_family {

  AGT_function_entry(p_a40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) {

  }

  AGT_function_entry(s_a40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) {

  }

  AGT_function_entry(g_p_a40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) {

  }

  AGT_function_entry(g_s_a40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) {

  }

}