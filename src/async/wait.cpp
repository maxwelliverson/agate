//
// Created by maxwe on 2022-12-20.
//

#include "config.hpp"
#include "async.hpp"



#define AGT_return_type            agt_status_t


#undef  AGT_exported_function_name
#define AGT_exported_function_name async_status


AGT_export_family {

  AGT_function_entry(pa40)(agt_async_t* async) {

  }

  AGT_function_entry(sa40)(agt_async_t* async) {

  }

  AGT_function_entry(gpa40)(agt_async_t* async) {

  }

  AGT_function_entry(gsa40)(agt_async_t* async) {

  }

}




#undef  AGT_exported_function_name
#define AGT_exported_function_name wait


AGT_export_family {

  AGT_function_entry(pa40)(agt_async_t* async,   agt_timeout_t timeout) {

  }

  AGT_function_entry(sa40)(agt_async_t* async,   agt_timeout_t timeout) {

  }

  AGT_function_entry(gpa40)(agt_async_t* async, agt_timeout_t timeout) {

  }

  AGT_function_entry(gsa40)(agt_async_t* async, agt_timeout_t timeout) {

  }

}


#undef  AGT_exported_function_name
#define AGT_exported_function_name wait_all


AGT_export_family {

  AGT_function_entry(pa40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_timeout_t timeout) {

  }

  AGT_function_entry(sa40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_timeout_t timeout) {

  }

  AGT_function_entry(gpa40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_timeout_t timeout) {

  }

  AGT_function_entry(gsa40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_timeout_t timeout) {

  }

}


#undef  AGT_exported_function_name
#define AGT_exported_function_name wait_any


AGT_export_family {

  AGT_function_entry(pa40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) {

  }

  AGT_function_entry(sa40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) {

  }

  AGT_function_entry(gpa40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) {

  }

  AGT_function_entry(gsa40)(agt_async_t* const * ppAsync, agt_size_t asyncCount, agt_size_t* pIndex, agt_timeout_t timeout) {

  }

}