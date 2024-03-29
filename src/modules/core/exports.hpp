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


  agt_instance_t       AGT_stdcall create_instance_private(agt_config_t config, init::init_manager& manager);
  agt_instance_t       AGT_stdcall create_instance_shared(agt_config_t config, init::init_manager& manager);

  void                 AGT_stdcall destroy_instance_private(agt_instance_t instance, bool invokedOnThreadExit);
  void                 AGT_stdcall destroy_instance_shared(agt_instance_t instance, bool invokedOnThreadExit);

  agt_ctx_t            AGT_stdcall create_ctx_private(agt_instance_t instance, const agt_allocator_params_t* pAllocParams);
  agt_ctx_t            AGT_stdcall create_ctx_shared(agt_instance_t instance, const agt_allocator_params_t* pAllocParams);

  bool                 AGT_stdcall release_ctx_private(agt_ctx_t ctx);
  bool                 AGT_stdcall release_ctx_shared(agt_ctx_t ctx);

  // agt_status_t         AGT_stdcall finalize_ctx_private(agt_ctx_t ctx);
  // agt_status_t         AGT_stdcall finalize_ctx_shared(agt_ctx_t ctx);


  agt_status_t AGT_stdcall enter_fctx(agt_ctx_t ctx, const agt_fctx_desc_t* pFCtxDesc, int* pExitCode) AGT_noexcept;
  void         AGT_stdcall exit_fctx(agt_ctx_t ctx, int exitCode) AGT_noexcept;

  agt_status_t AGT_stdcall new_fiber(agt_fiber_t* pFiber, agt_fiber_proc_t proc, void* userData) AGT_noexcept;
  agt_status_t AGT_stdcall destroy_fiber(agt_fiber_t fiber) AGT_noexcept;
  void*        AGT_stdcall set_fiber_data(agt_fiber_t fiber, void* userData) AGT_noexcept;
  void*        AGT_stdcall get_fiber_data(agt_fiber_t fiber) AGT_noexcept;

  void                 AGT_stdcall fiber_init_noexcept(agt_fiber_t fiber, agt_fiber_proc_t proc, bool isConvertingThread);
  void                 AGT_stdcall fiber_init_except(agt_fiber_t fiber, agt_fiber_proc_t proc, bool isConvertingThread);

  agt_fiber_transfer_t AGT_stdcall fiber_fork_noexcept(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags);
  agt_fiber_transfer_t AGT_stdcall fiber_fork_except(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags);

  agt_fiber_transfer_t AGT_stdcall fiber_loop_noexcept(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags);
  agt_fiber_transfer_t AGT_stdcall fiber_loop_except(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags);

  [[noreturn]] void    AGT_stdcall fiber_jump(agt_fiber_t toFiber, agt_fiber_param_t param);

  agt_fiber_transfer_t AGT_stdcall fiber_switch_noexcept(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags) noexcept;
  agt_fiber_transfer_t AGT_stdcall fiber_switch_except(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags) noexcept;


  async*       AGT_stdcall alloc_async(agt_ctx_t ctx);
  void         AGT_stdcall free_async(async* async);

  void         AGT_stdcall init_async_private(async* pAsyncBuffer);
  void         AGT_stdcall init_async_shared(async* pAsyncBuffer);
  void         AGT_stdcall copy_async_private(const async* from, async* to);
  void         AGT_stdcall copy_async_shared(const async* from, async* to);
  void         AGT_stdcall move_async(async* from, async* to);
  void         AGT_stdcall clear_async(async* async);
  void         AGT_stdcall destroy_async(async* async);

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
