//
// Created by maxwe on 2023-01-04.
//

#ifndef AGATE_LOG_H
#define AGATE_LOG_H

#include <agate/core.h>

AGT_begin_c_namespace



typedef void(* AGT_stdcall agt_log_handler_t)(void* userData,
                                              const void* msgBuffer,
                                              size_t      msgLength,
                                              const struct agt_log_info_t* pLogInfo);


typedef struct agt_log_object_info_t {
  agt_object_type_t   type;
  agt_object_id_t     id;
  const void*         address;
  const agt_string_t* name;
  agt_scope_t         scope;
} agt_log_object_info_t;

typedef enum agt_log_severity_t {
  AGT_SEVERITY_FATAL,
  AGT_SEVERITY_ERROR,
  AGT_SEVERITY_WARNING,
  AGT_SEVERITY_INFO,
  AGT_SEVERITY_DEBUG
} agt_log_severity_t;

typedef struct agt_internal_log_info_t {
  agt_ctx_t                    ctx;
  agt_status_t                 status;
  agt_log_severity_t           severity;
  const char*                  message;
  agt_size_t                   messageLength;
  const agt_log_object_info_t* pObjects;
  agt_size_t                   objectCount;
  agt_timestamp_t              timestamp;
  void*                        userData;
} agt_internal_log_info_t;

typedef struct agt_log_info_t {
  agt_ctx_t       ctx;
  agt_u32_t       category;
  agt_timestamp_t timestamp;
  agt_agent_t     agent;
  agt_string_t    agentName;
} agt_log_info_t;




AGT_log_api void                       AGT_stdcall agt_log(agt_self_t self, agt_u32_t category, const void* msg, size_t msgLength) AGT_noexcept;



AGT_log_api agt_internal_log_handler_t AGT_stdcall agt_set_internal_log_handler(agt_ctx_t ctx, agt_internal_log_handler_t callback, void* userData) AGT_noexcept;

AGT_log_api agt_internal_log_handler_t AGT_stdcall agt_get_internal_log_handler(agt_ctx_t ctx, void** pUserData) AGT_noexcept;


AGT_end_c_namespace

#endif//AGATE_LOG_H
