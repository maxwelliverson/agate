//
// Created by maxwe on 2024-10-13.
//

#include "scheduler.hpp"


namespace agt {
  void bind_default_sched(agt_ctx_t ctx) noexcept {
    // ctx->
    // If ctx->parentCtx is nonnull, check parentCtx's sched. If it has the auto_inherit flag, then inherit that sched.
    // Otherwise, bind a default busy_executor that may be overwritten!
    AGT_unreachable;
  }
}

namespace agt::api {
  agt_status_t AGT_stdcall new_sched_local(agt_ctx_t ctx, agt_sched_t* pSched, const agt_sched_desc_t* pSchedDesc) noexcept {
    AGT_try_resolve_ctx(ctx);

    if (!pSched || !pSchedDesc || !pSchedDesc->vtable)
      return AGT_ERROR_INVALID_ARGUMENT; // Can't be null...

    auto vptr = pSchedDesc->vtable;

    if (!vptr->resume || !vptr->suspend || !vptr->suspend_until) // these functions are required.
      return AGT_ERROR_INVALID_ARGUMENT; // Can't be null...

    auto entity = alloc<agt_sched_st>(ctx);
    agt_status_t status = AGT_SUCCESS;


    entity->bindData = pSchedDesc->bindData;
    entity->vptr     = pSchedDesc->vtable;
    entity->flags    = pSchedDesc->flags;

    if (entity->flags & AGT_SCHED_IS_CROSS_CONTEXT)
      entity->multiThreadedSched = alloc<multi_thread_sched>(ctx);
    else
      entity->multiThreadedSched = nullptr;

    *pSched = entity;

    return status;
  }

  void         AGT_stdcall close_sched_local(agt_ctx_t ctx, agt_sched_t sched) noexcept {
    (void)ctx;
    if (sched->multiThreadedSched)
      release(sched->multiThreadedSched);
    release(sched);
  }

  agt_status_t AGT_stdcall bind_sched_local(agt_ctx_t ctx, agt_sched_t sched) noexcept {
    
  }

  agt_bool_t   AGT_stdcall ctx_has_same_sched_local(agt_ctx_t ctx, agt_ctx_t otherCtx) noexcept {

  }
}