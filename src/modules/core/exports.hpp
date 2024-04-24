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


  agt_async_t  AGT_stdcall make_new_async(agt_ctx_t ctx, agt_async_flags_t flags);
  void         AGT_stdcall copy_async_private(const async* from, async* to);
  void         AGT_stdcall copy_async_shared(const async* from, async* to);
  void         AGT_stdcall move_async(async* from, async* to);
  void         AGT_stdcall clear_async(agt_async_t async);
  void         AGT_stdcall destroy_async(agt_async_t async);

  void AGT_stdcall         destroy_async_local(agt_async_t async) noexcept;

  agt_status_t AGT_stdcall async_get_status_private(agt_async_t async, agt_u64_t* pResult);
  agt_status_t AGT_stdcall async_get_status_shared(agt_async_t async, agt_u64_t* pResult);
  agt_status_t AGT_stdcall async_wait_native_unit_private(agt_async_t async, agt_u64_t* pResult, agt_timeout_t timeout);
  agt_status_t AGT_stdcall async_wait_foreign_unit_private(agt_async_t async, agt_u64_t* pResult, agt_timeout_t timeout);
  agt_status_t AGT_stdcall async_wait_native_unit_shared(agt_async_t async, agt_u64_t* pResult, agt_timeout_t timeout);
  agt_status_t AGT_stdcall async_wait_foreign_unit_shared(agt_async_t async, agt_u64_t* pResult, agt_timeout_t timeout);

  async_data_t AGT_stdcall async_attach_local(async* async, agt_u32_t expectedCount, agt_u32_t attachedCount, async_key_t& key);
  async_data_t AGT_stdcall async_attach_shared(async* async, agt_u32_t expectedCount, agt_u32_t attachedCount, async_key_t& key);


  agt_status_t AGT_stdcall reserve_name_local(agt_ctx_t ctx, const agt_name_desc_t* pNameDesc, agt_name_result_t* pResult);
  agt_status_t AGT_stdcall reserve_name_shared(agt_ctx_t ctx, const agt_name_desc_t* pNameDesc, agt_name_result_t* pResult);

  void         AGT_stdcall release_name_local(agt_ctx_t ctx, agt_name_t name);
  void         AGT_stdcall release_name_shared(agt_ctx_t ctx, agt_name_t name);

  agt_status_t AGT_stdcall bind_name_local(agt_ctx_t ctx, agt_name_t name, agt_object_t object);
  agt_status_t AGT_stdcall bind_name_shared(agt_ctx_t ctx, agt_name_t name, agt_object_t object);


  agt_status_t AGT_stdcall open_channel(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t* pChannelDesc) noexcept;
  agt_status_t AGT_stdcall acquire_msg_local(agt_sender_t sender, size_t desiredMessageSize, void** ppMsgBuffer) noexcept;
  agt_status_t AGT_stdcall send_msg_local(agt_sender_t sender, void* msgBuffer, size_t size, agt_async_t async) noexcept;
  agt_status_t AGT_stdcall receive_msg_local(agt_receiver_t receiver, void** pMsgBuffer, size_t* pMsgSize, agt_timeout_t timeout) noexcept;
  void         AGT_stdcall retire_msg_local(agt_receiver_t receiver, void* msgBuffer, agt_u64_t response) noexcept;


  void         AGT_stdcall close_local(agt_object_t object) noexcept;
  void         AGT_stdcall close_shared(agt_object_t object) noexcept;


  agt_status_t AGT_stdcall new_user_uexec(agt_ctx_t ctx, agt_uexec_t* pExec, const agt_uexec_desc_t* pExecDesc) noexcept;

  void         AGT_stdcall destroy_user_uexec(agt_ctx_t ctx, agt_uexec_t exec) noexcept;

  agt_status_t AGT_stdcall bind_uexec_to_ctx(agt_ctx_t ctx, agt_uexec_t exec, void* execCtxData) noexcept;

}

#endif//AGATE_CORE_EXPORTS_HPP
