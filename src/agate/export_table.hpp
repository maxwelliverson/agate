//
// Created by maxwe on 2022-12-21.
//

#ifndef AGATE_EXPORT_TABLE_HPP
#define AGATE_EXPORT_TABLE_HPP

#include "config.hpp"
#include "version.hpp"

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

  struct module_table;

  struct AGT_cache_aligned export_table {

    version                     _header_version;
    version                     _effective_library_version;
    module_table*               _modules;
    agt_instance_t              _instance;
    agt_ctx_t                   _ctx;
    size_t                      _size_of_async_struct;
    size_t                      _size_of_signal_struct;


    // Version 0.01

    AGT_cache_aligned
    agt_status_t (* AGT_stdcall _pfn_finalize)(agt_ctx_t ctx);

    /* ========================= [ Async 0.01 ] ========================= */

    agt_status_t (* AGT_stdcall _pfn_new_async)(agt_ctx_t ctx, agt_async_t* pAsync, agt_async_flags_t flags);
    void         (* AGT_stdcall _pfn_copy_async)(const agt_async_t* pFrom, agt_async_t* pTo);
    void         (* AGT_stdcall _pfn_move_async)(agt_async_t* from, agt_async_t* to);
    void         (* AGT_stdcall _pfn_clear_async)(agt_async_t* async);
    void         (* AGT_stdcall _pfn_destroy_async)(agt_async_t* async);
    agt_status_t (* AGT_stdcall _pfn_async_status)(agt_async_t* async);

    agt_status_t (* AGT_stdcall _pfn_wait)(agt_async_t* async, agt_timeout_t timeout);
    agt_status_t (* AGT_stdcall _pfn_wait_all)(agt_async_t* const *, agt_size_t, agt_timeout_t);
    agt_status_t (* AGT_stdcall _pfn_wait_any)(agt_async_t* const *, agt_size_t, agt_size_t*, agt_timeout_t);

    agt_status_t (* AGT_stdcall _pfn_new_signal)(agt_ctx_t ctx, agt_signal_t* pSignal);
    void         (* AGT_stdcall _pfn_attach_signal)(agt_signal_t*, agt_async_t* async);
    void         (* AGT_stdcall _pfn_attach_many_signals)(agt_signal_t* const * pSignals, agt_size_t signalCount, agt_async_t* async, size_t waitForN);
    void         (* AGT_stdcall _pfn_raise_signal)(agt_signal_t* signal);
    void         (* AGT_stdcall _pfn_raise_many_signals)(agt_signal_t* const * pSignals, agt_size_t signalCount);
    void         (* AGT_stdcall _pfn_destroy_signal)(agt_signal_t* signal);


    /* ========================= [ Agents 0.01 ] ========================= */

    agt_status_t (* AGT_stdcall _pfn_register_type)(agt_ctx_t ctx, const agt_agent_type_create_info_t* cpCreateInfo, agt_typeid_t* pId);
    size_t       (* AGT_stdcall _pfn_enumerate_types)(agt_ctx_t ctx, const char* pattern, size_t patternLength, agt_type_enumerator_t enumerator, void* userData);


    agt_status_t (* AGT_stdcall _pfn_create_busy_agent)(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo, agt_agent_t* pAgent);
    void         (* AGT_stdcall _pfn_create_busy_agent_on_current_thread)(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo);
    agt_status_t (* AGT_stdcall _pfn_create_event_agent)(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo, agt_agent_t* pAgent);
    agt_status_t (* AGT_stdcall _pfn_create_free_event_agent)(agt_ctx_t ctx, const agt_agent_create_info_t* cpCreateInfo, agt_agent_t* pAgent);
    agt_status_t (* AGT_stdcall _pfn_transfer_owner)(agt_ctx_t ctx, agt_agent_t agentHandle, agt_agent_t newOwner);

    agt_agent_handle_t (* AGT_stdcall _pfn_export_self)();
    agt_status_t (* AGT_stdcall _pfn_export_agent)(agt_ctx_t ctx, agt_agent_t agent, agt_agent_handle_t* pHandle);
    agt_status_t (* AGT_stdcall _pfn_import)(agt_ctx_t ctx, agt_agent_handle_t importHandle, agt_agent_t* pAgent);


    // agt_ctx_t    (* AGT_stdcall _pfn_current_context)();
    agt_agent_t  (* AGT_stdcall _pfn_self)();
    agt_agent_t  (* AGT_stdcall _pfn_retain_sender)();
    agt_status_t (* AGT_stdcall _pfn_retain)(agt_agent_t* pNewAgent, agt_agent_t agent);

    agt_status_t (* AGT_stdcall _pfn_send)(agt_agent_t recipient, const agt_send_info_t* pSendInfo);
    agt_status_t (* AGT_stdcall _pfn_send_as)(agt_agent_t spoofSender, agt_agent_t recipient, const agt_send_info_t* pSendInfo);
    agt_status_t (* AGT_stdcall _pfn_send_many)(const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo);
    agt_status_t (* AGT_stdcall _pfn_send_many_as)(agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_send_info_t* pSendInfo);
    agt_status_t (* AGT_stdcall _pfn_reply)(const agt_send_info_t* pSendInfo);
    agt_status_t (* AGT_stdcall _pfn_reply_as)(agt_agent_t spoofReplier, const agt_send_info_t* pSendInfo);


    agt_status_t (* AGT_stdcall _pfn_raw_acquire)(agt_agent_t recipient, size_t desiredMessageSize, agt_raw_msg_t* pRawMsg, void** ppRawBuffer);
    agt_status_t (* AGT_stdcall _pfn_raw_send)(agt_agent_t recipient, const agt_raw_send_info_t* pRawSendInfo);
    agt_status_t (* AGT_stdcall _pfn_raw_send_as)(agt_agent_t spoofSender, agt_agent_t recipient, const agt_raw_send_info_t* pRawSendInfo);
    agt_status_t (* AGT_stdcall _pfn_raw_send_many)(const agt_agent_t* recipients, agt_size_t agentCount, const agt_raw_send_info_t* pRawSendInfo);
    agt_status_t (* AGT_stdcall _pfn_raw_send_many_as)(agt_agent_t spoofSender, const agt_agent_t* recipients, agt_size_t agentCount, const agt_raw_send_info_t* pRawSendInfo);
    agt_status_t (* AGT_stdcall _pfn_raw_reply)(const agt_raw_send_info_t* pRawSendInfo);
    agt_status_t (* AGT_stdcall _pfn_raw_reply_as)(agt_agent_t spoofSender, const agt_raw_send_info_t* pRawSendInfo);


    void         (* AGT_stdcall _pfn_delegate)(agt_agent_t recipient);

    void         (* AGT_stdcall _pfn_complete)();
    void         (* AGT_stdcall _pfn_release)(agt_agent_t agent);

    agt_pinned_msg_t (* AGT_stdcall _pfn_pin)();
    void             (* AGT_stdcall _pfn_unpin)(agt_pinned_msg_t pinnedMsg);


    void (* AGT_stdcall _pfn_exit)(int exitCode);
    void (* AGT_stdcall _pfn_abort)();

    void (* AGT_stdcall _pfn_resume_coroutine)(agt_agent_t receiver, void* coroutine, agt_async_t* asyncHandle);


  };
}

#endif//AGATE_EXPORT_TABLE_HPP
