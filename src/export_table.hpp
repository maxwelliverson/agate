//
// Created by maxwe on 2022-12-21.
//

#ifndef AGATE_EXPORT_TABLE_HPP
#define AGATE_EXPORT_TABLE_HPP

#include "agate/version.hpp"
#include "config.hpp"

namespace agt {

  /*struct module_info {
    module_id id;
    version   moduleVersion;
    HMODULE   libraryHandle;
    char      moduleName[16];
    wchar_t   libraryPath[260];
  };

  struct module_table {
    module_info* table;
    size_t       count;
    agt_u32_t    refCount;
  };*/

  struct fiber;

  struct fiber_pool;

  struct async;

  /*struct alignas(AGT_ASYNC_STRUCT_ALIGNMENT) async_base {
    agt_ctx_t         ctx;
    agt_u32_t         structSize;
    agt_async_flags_t flags;
    agt_u64_t         resultValue;
    fiber*            boundFiber;
    agt_status_t      status;
    agt_u8_t          reserved[AGT_ASYNC_STRUCT_SIZE - 36];
  };*/


  /**
   * The agate library loader may only be linked statically,
   * so each library that links to agate will have their own
   * internal dispatch table. This dispatch table may be
   * initialized either by a call to
   *
   * */

  struct user_plugin_info;
  struct module_info;

  struct module_table;
  struct user_plugin_table;


  class proc_set;


  namespace init {
    class attributes;
    class init_manager;
  }

  namespace impl {
    struct fiber_stack_info;
  }


  inline constexpr static size_t MaxAPINameLength = AGT_CACHE_LINE - sizeof(agt_proc_t);

  struct AGT_cache_aligned proc_entry {
    char       name[MaxAPINameLength];
    agt_proc_t address;
  };


  struct AGT_page_aligned export_table {
    agt_u32_t               refCount;                ///< If parentExportTable is not null, refCount is ignored
    version                 headerVersion;           ///< The value of agt_init_info_t::apiVersion with which this library was initialized
    version                 loaderVersion;           ///< Version of the loader library used
    version                 effectiveLibraryVersion; ///< For shared builds, version of the linked object files found dynamically. For static builds, this is equal to loaderVersion
    module_table*           modules;                 ///< Pointer to a table that lists information about the library modules loaded
    user_plugin_table*      userModules;             ///< Pointer to a table that lists user defined plugin modules loaded. If null, no user defined modules have been loaded.
    agt_instance_t          instance;                ///< Pointer to

    const proc_entry*       pProcSet;                 ///< A database of all the loaded API functions and their addresses.
    agt_u32_t               procSetSize;

    export_table*           parentExportTable;       ///< If not null, points to the export table local to the process executable.

    const uintptr_t*        attrValues;              ///< 64 bit values indexed by agt_attr_id_t. length: attrCount
    const agt_value_type_t* attrTypes;               ///< agt_attr_type_t values indexed by agt_attr_id_t. length: attrCount
    agt_u32_t               attrCount;               ///< Size of attribute table

    agt_internal_log_handler_t logHandler;           ///< Local log handler
    void*                      logHandlerData;       ///< Log handler user data.

    agt_u32_t               tableSize;
    agt_u32_t               maxValidOffset;



    // Version 0.01

    AGT_cache_aligned

    agt_instance_t (* AGT_stdcall _pfn_create_instance)(agt_config_t config, init::init_manager& manager);
    agt_ctx_t      (* AGT_stdcall _pfn_create_ctx)(agt_instance_t instance, const agt_allocator_params_t* pAllocParams); // Do not call this directly except in a few select cases. Usually, you want to use acquire_ctx, which, if a ctx has already been created for the current thread, returns that, and only creates a new ctx if one doesn't already exist.

    // invokedOnThreadExit is true if the instance is being automatically cleaned up.
    void           (* AGT_stdcall _pfn_destroy_instance)(agt_instance_t instance, bool invokedOnThreadExit);  // Only used by init routines to rollback upon failure!! Never directly destroy an instance object.
    // returns true if the instance should be destroyed
    bool           (* AGT_stdcall _pfn_release_ctx)(agt_ctx_t ctx);                 // Only used by init routines to rollback upon failure!! Use ctx finalize to destroy a context object.

    // void           (* AGT_stdcall _pfn_free_modules)(module_table* table);

    void           (* AGT_stdcall _pfn_free_modules_on_thread_exit)(module_table* table);

    agt_ctx_t      (* AGT_stdcall _pfn_ctx)();

    agt_ctx_t      (* AGT_stdcall _pfn_do_acquire_ctx)(agt_instance_t instance, const agt_allocator_params_t* pAllocParams);



    agt_status_t (* AGT_stdcall _pfn_reserve_name)(agt_ctx_t ctx, const agt_name_desc_t* pNameDesc, agt_name_result_t* pResult);
    void         (* AGT_stdcall _pfn_release_name)(agt_ctx_t ctx, agt_name_t name);
    agt_status_t (* AGT_stdcall _pfn_bind_name)(agt_ctx_t ctx, agt_name_t name, agt_object_t object);


    async*       (* AGT_stdcall _pfn_alloc_async)(agt_ctx_t ctx);
    void         (* AGT_stdcall _pfn_free_async)(agt_ctx_t ctx, agt_async_t async);


    void         (* AGT_stdcall _pfn_raise)(agt_ctx_t ctx, agt_status_t errorCode, void* errorInfo);

    void         (* AGT_stdcall _pfn_close)(agt_object_t object) AGT_noexcept;


    agt_status_t (* AGT_stdcall _pfn_new_uexec)(agt_ctx_t ctx, agt_uexec_t* pExec, const agt_uexec_desc_t* pExecDesc) noexcept;
    void         (* AGT_stdcall _pfn_destroy_uexec)(agt_ctx_t ctx, agt_uexec_t exec) noexcept;

    agt_status_t (* AGT_stdcall _pfn_bind_uexec)(agt_ctx_t ctx, agt_uexec_t exec, agt_ctxexec_t ctxexec) noexcept;

    /* ========================= [ Async 0.01 ] ========================= */

    agt_async_t  (* AGT_stdcall _pfn_new_async)(agt_ctx_t ctx, agt_async_flags_t flags);

    void         (* AGT_stdcall _pfn_copy_async)(const async* from, async* to);
    void         (* AGT_stdcall _pfn_move_async)(async* from, async* to);

    void         (* AGT_stdcall _pfn_clear_async)(agt_async_t async);
    void         (* AGT_stdcall _pfn_destroy_async)(agt_async_t async);

    agt_status_t (* AGT_stdcall _pfn_async_status)(agt_async_t async, agt_u64_t* pResult);

    agt_status_t (* AGT_stdcall _pfn_wait)(agt_async_t async, agt_u64_t* pResult, agt_timeout_t timeout);
    agt_status_t (* AGT_stdcall _pfn_wait_all)(const agt_async_t*, agt_size_t, agt_timeout_t);
    agt_status_t (* AGT_stdcall _pfn_wait_any)(const agt_async_t*, agt_size_t, agt_size_t*, agt_timeout_t);

    agt_status_t (* AGT_stdcall _pfn_new_signal)(agt_ctx_t ctx, agt_signal_t* pSignal);
    void         (* AGT_stdcall _pfn_attach_signal)(agt_signal_t*, agt_async_t async);
    void         (* AGT_stdcall _pfn_attach_many_signals)(agt_signal_t* const * pSignals, agt_size_t signalCount, agt_async_t async, size_t waitForN);
    void         (* AGT_stdcall _pfn_raise_signal)(agt_signal_t* signal);
    void         (* AGT_stdcall _pfn_raise_many_signals)(agt_signal_t* const * pSignals, agt_size_t signalCount);
    void         (* AGT_stdcall _pfn_destroy_signal)(agt_signal_t* signal);


    agt_status_t (* AGT_stdcall _pfn_open_channel)(agt_ctx_t ctx, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t* pChannelDesc) AGT_noexcept;
    agt_status_t (* AGT_stdcall _pfn_acquire_msg)(agt_sender_t sender, size_t desiredMessageSize, void** ppMsgBuffer) AGT_noexcept;
    agt_status_t (* AGT_stdcall _pfn_send_msg)(agt_sender_t sender, void* msgBuffer, size_t size, agt_async_t async) AGT_noexcept;
    agt_status_t (* AGT_stdcall _pfn_receive_msg)(agt_receiver_t receiver, void** pMsgBuffer, size_t* pMsgSize, agt_timeout_t timeout) AGT_noexcept;
    void         (* AGT_stdcall _pfn_retire_msg)(agt_receiver_t receiver, void* msgBuffer, agt_u64_t response) AGT_noexcept;


    /* ========================= [ Agents 0.01 ] ========================= */


    agt_status_t (* AGT_stdcall _pfn_create_agent)(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo, agt_agent_t* pAgent);
    void         (* AGT_stdcall _pfn_create_busy_agent_on_current_thread)(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo);
    agt_status_t (* AGT_stdcall _pfn_transfer_owner)(agt_ctx_t ctx, agt_agent_t agentHandle, agt_agent_t newOwner);

    agt_handle_t (* AGT_stdcall _pfn_export_self)(agt_self_t self);
    agt_status_t (* AGT_stdcall _pfn_export_agent)(agt_ctx_t ctx, agt_agent_t agent, agt_agent_handle_t* pHandle);
    agt_status_t (* AGT_stdcall _pfn_import)(agt_ctx_t ctx, agt_agent_handle_t importHandle, agt_agent_t* pAgent);


    agt_agent_t  (* AGT_stdcall _pfn_retain_sender)(agt_self_t self);
    agt_status_t (* AGT_stdcall _pfn_retain_self)(agt_self_t self, agt_agent_t* pResult);
    agt_status_t (* AGT_stdcall _pfn_retain)(agt_agent_t agent, agt_agent_t* pNewAgent);

    agt_status_t (* AGT_stdcall _pfn_send)(agt_self_t self, agt_agent_t recipient, const agt_send_info_t* pSendInfo);
    agt_status_t (* AGT_stdcall _pfn_send_as)(agt_self_t self, agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo);
    agt_status_t (* AGT_stdcall _pfn_send_many)(agt_self_t self, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo);
    agt_status_t (* AGT_stdcall _pfn_send_many_as)(agt_self_t self, agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo);
    agt_status_t (* AGT_stdcall _pfn_reply)(agt_self_t self, const agt_send_info_t* pSendInfo);
    agt_status_t (* AGT_stdcall _pfn_reply_as)(agt_self_t self, agt_agent_t spoofReplier, const agt_send_info_t* pSendInfo);


    agt_status_t (* AGT_stdcall _pfn_raw_acquire)(agt_self_t self, agt_agent_t recipient, size_t desiredMessageSize, agt_raw_send_info_t* pRawSendInfo, void** ppRawBuffer);
    agt_status_t (* AGT_stdcall _pfn_raw_send)(agt_self_t self, agt_agent_t recipient, const agt_raw_send_info_t* pRawSendInfo);
    agt_status_t (* AGT_stdcall _pfn_raw_send_as)(agt_self_t self, agt_agent_t spoofSender, agt_agent_t recipient, const agt_raw_send_info_t* pRawSendInfo);
    agt_status_t (* AGT_stdcall _pfn_raw_send_many)(agt_self_t self, const agt_agent_t* recipients, agt_size_t agentCount, const agt_raw_send_info_t* pRawSendInfo);
    agt_status_t (* AGT_stdcall _pfn_raw_send_many_as)(agt_self_t self, agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_raw_send_info_t* pRawSendInfo);
    agt_status_t (* AGT_stdcall _pfn_raw_reply)(agt_self_t self, const agt_raw_send_info_t* pRawSendInfo);
    agt_status_t (* AGT_stdcall _pfn_raw_reply_as)(agt_self_t self, agt_agent_t spoofSender, const agt_raw_send_info_t* pRawSendInfo);


    void         (* AGT_stdcall _pfn_delegate)(agt_self_t self, agt_agent_t recipient);

    void         (* AGT_stdcall _pfn_return)(agt_self_t self, agt_u64_t value);
    void         (* AGT_stdcall _pfn_release)(agt_self_t self, agt_agent_t agent);


    void         (* AGT_stdcall _pfn_exit)(agt_self_t self, int exitCode);
    void         (* AGT_stdcall _pfn_abort)(agt_self_t self);

    void         (* AGT_stdcall _pfn_resume_coroutine)(agt_self_t self, agt_agent_t receiver, void* coroutine, agt_async_t* asyncHandle);




    agt_status_t         (* AGT_stdcall _pfn_new_thread)(agt_ctx_t ctx, agt_thread_t* pThread, const agt_thread_desc_t* pThreadDesc);



    fiber_pool*          (* AGT_stdcall _pfn_new_fiber_pool)(agt_ctx_t ctx, const agt_fctx_desc_t* fiberDesc);
    void                 (* AGT_stdcall _pfn_retain_fiber_pool)(agt_ctx_t ctx, fiber_pool* pool);
    void                 (* AGT_stdcall _pfn_release_fiber_pool)(agt_ctx_t ctx, fiber_pool* pool);

    agt_status_t         (* AGT_stdcall _pfn_fiber_pool_alloc)(agt_ctx_t ctx, fiber_pool* pool, agt_fiber_proc_t proc, void* userData, agt_fiber_t* pResult);


    agt_status_t         (* AGT_stdcall _pfn_enter_fctx)(agt_ctx_t ctx, const agt_fctx_desc_t* pFCtxDesc, int* pExitCode);
    void                 (* AGT_stdcall _pfn_exit_fctx)(agt_ctx_t ctx, int exitCode);

    agt_status_t         (* AGT_stdcall _pfn_new_fiber)(agt_fiber_t* pFiber, agt_fiber_proc_t proc, void* userData);
    agt_status_t         (* AGT_stdcall _pfn_destroy_fiber)(agt_fiber_t fiber);

    void                 (* AGT_stdcall _pfn_fiber_init)(agt_fiber_t fiber, agt_fiber_proc_t proc, const impl::fiber_stack_info& stackInfo);
    agt_fiber_transfer_t (* AGT_stdcall _pfn_fiber_switch)(agt_fiber_t fiber, agt_fiber_param_t param, agt_fiber_flags_t flags);
    void                 (* AGT_stdcall _pfn_fiber_jump)(agt_fiber_t fiber, agt_fiber_param_t param);
    agt_fiber_transfer_t (* AGT_stdcall _pfn_fiber_fork)(agt_fiber_proc_t proc, agt_fiber_param_t param, agt_fiber_flags_t flags);
    agt_fiber_param_t    (* AGT_stdcall _pfn_fiber_loop)(agt_fiber_proc_t proc, agt_u64_t param, agt_fiber_flags_t flags);



    /** ====================[ agate-log ]======================= **/

    AGT_log_api void     (* AGT_stdcall _pfn_log)(agt_self_t self, agt_u32_t category, const void* msg, size_t msgLength);
  };
}

#endif//AGATE_EXPORT_TABLE_HPP
