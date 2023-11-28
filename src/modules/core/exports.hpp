//
// Created by maxwe on 2023-08-23.
//

#ifndef AGATE_CORE_EXPORTS_HPP
#define AGATE_CORE_EXPORTS_HPP

#include "config.hpp"
#include "export_table.hpp"


#include <fwd.hpp>

namespace agt {

  struct async;


  void                 AGT_stdcall fiber_init_noexcept(agt_fiber_t fiber, agt_fiber_proc_t proc, bool isConvertingThread);
  void                 AGT_stdcall fiber_init_except(agt_fiber_t fiber, agt_fiber_proc_t proc, bool isConvertingThread);

  agt_fiber_transfer_t AGT_stdcall fiber_fork_noexcept(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags);
  agt_fiber_transfer_t AGT_stdcall fiber_fork_except(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags);

  agt_fiber_transfer_t AGT_stdcall fiber_loop_noexcept(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags);
  agt_fiber_transfer_t AGT_stdcall fiber_loop_except(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags);

  [[noreturn]] void    AGT_stdcall fiber_jump(agt_fiber_t toFiber, agt_fiber_param_t param);

  agt_fiber_transfer_t AGT_stdcall fiber_switch_noexcept(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags) noexcept;
  agt_fiber_transfer_t AGT_stdcall fiber_switch_except(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags) noexcept;


  async_base*  AGT_stdcall alloc_async(agt_ctx_t ctx);
  void         AGT_stdcall free_async(async_base* async);

  void         AGT_stdcall init_async_private(async* pAsyncBuffer);
  void         AGT_stdcall init_async_shared(async* pAsyncBuffer);
  void         AGT_stdcall copy_async_private(const async_base* from, async_base* to);
  void         AGT_stdcall copy_async_shared(const async_base* from, async_base* to);
  void         AGT_stdcall move_async(async_base* from, async_base* to);
  void         AGT_stdcall clear_async(async_base* async);
  void         AGT_stdcall destroy_async(async_base* async);

  agt_status_t AGT_stdcall async_get_status_private(async* async, agt_u64_t* pResult);
  agt_status_t AGT_stdcall async_get_status_shared(async* async, agt_u64_t* pResult);
  agt_status_t AGT_stdcall async_wait_native_unit_private(async* async, agt_u64_t* pResult, agt_timeout_t timeout);
  agt_status_t AGT_stdcall async_wait_foreign_unit_private(async* async, agt_u64_t* pResult, agt_timeout_t timeout);
  agt_status_t AGT_stdcall async_wait_native_unit_shared(async* async, agt_u64_t* pResult, agt_timeout_t timeout);
  agt_status_t AGT_stdcall async_wait_foreign_unit_shared(async* async, agt_u64_t* pResult, agt_timeout_t timeout);

  async_data_t AGT_stdcall async_attach_local(async* async, agt_u32_t expectedCount, agt_u32_t attachedCount, async_key_t& key);
  async_data_t AGT_stdcall async_attach_shared(async* async, agt_u32_t expectedCount, agt_u32_t attachedCount, async_key_t& key);
}

#endif//AGATE_CORE_EXPORTS_HPP
