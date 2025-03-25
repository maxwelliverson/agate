//
// Created by maxwe on 2024-10-13.
//

#ifndef AGATE_CORE_SCHEDULER_HPP
#define AGATE_CORE_SCHEDULER_HPP

#include "config.hpp"

#include "agate/atomic.hpp"
#include "agate/cast.hpp"
#include "agate/set.hpp"

#include "core/object.hpp"
#include "core/ref.hpp"

AGT_api_object(agt_sched_st, sched_entity, ref_counted) {
  void*                     bindData;
  const agt_sched_vtable_t* vptr;
  agt::sched_instance*      multiThreadedSched;
};

namespace agt {

  void bind_default_sched(agt_ctx_t ctx) noexcept;

  AGT_abstract_object(sched_instance, ref_counted) {
    agt_sched_t entity;
  };

  AGT_object(single_thread_sched, extends(sched_instance)) {
    agt_ctx_t ctx;
  };

  AGT_object(multi_thread_sched, extends(sched_instance)) {
    set<agt_ctx_t> ctxSet;
  };

  AGT_forceinline void destroy(sched_instance* sched) noexcept {
    if (auto multiThreadSched = nonnull_object_cast<multi_thread_sched>(sched)) {
      std::destroy_at(&multiThreadSched->ctxSet);
    }
    release(sched->entity);
  }

  AGT_forceinline void destroy(agt_sched_t sched) noexcept {
    sched->multiThreadedSched
  }

}

#endif//AGATE_CORE_SCHEDULER_HPP
