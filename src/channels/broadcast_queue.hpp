//
// Created by maxwe on 2022-12-02.
//

#ifndef AGATE_BROADCAST_QUEUE_HPP
#define AGATE_BROADCAST_QUEUE_HPP

#include "config.hpp"

namespace agt {

  struct broadcast_message;
  struct sp_shared_bqueue_state;
  struct mp_shared_bqueue_state;

  using bqueue_t = void*;

  enum bqueue_kind_t {
    sp_local_bqueue_kind,
    mp_local_bqueue_kind,
    sp_shared_bqueue_kind,
    mp_shared_bqueue_kind,
    bqueue_kind_max_enum
    // private_bqueue_kind,
  };

  struct bqueue_header {
    bqueue_kind_t kind;
  };

  struct bqueue_state_header {};

  struct mp_local_bqueue  : bqueue_header {
    agt_u32_t          producerCount;
    broadcast_message* head;
  };

  struct sp_local_bqueue  : bqueue_header {
    broadcast_message* head;
  };

  struct mp_shared_bqueue : bqueue_header {
    agt_u32_t               consumerCount;
    agt_ctx_t               ctx;
    mp_shared_bqueue_state* state;
    agt_u32_t               producerCount;
  };

  struct sp_shared_bqueue : bqueue_header {
    agt_u32_t               consumerCount;
    agt_ctx_t               ctx;
    sp_shared_bqueue_state* state;

  };



  struct bqueue_broadcaster_header {
    bqueue_kind_t kind;
  };

  struct bqueue_listener_header {
    bqueue_kind_t kind;
  };

  using bqueue_broadcaster_t = void*;
  using bqueue_listener_t    = void*;


  struct  sp_local_bqueue_broadcaster : bqueue_broadcaster_header {
    agt_u32_t          consumerCount;
    broadcast_message* head;
  };
  struct  mp_local_bqueue_broadcaster : bqueue_broadcaster_header {
    agt_u32_t          consumerCount;
    broadcast_message* head;
    agt_u32_t          producerCount;
  };
  struct sp_shared_bqueue_broadcaster : bqueue_broadcaster_header { };
  struct mp_shared_bqueue_broadcaster : bqueue_broadcaster_header { };

  struct  sp_local_bqueue_listener : bqueue_listener_header { };
  struct  mp_local_bqueue_listener : bqueue_listener_header { };
  struct sp_shared_bqueue_listener : bqueue_listener_header { };
  struct mp_shared_bqueue_listener : bqueue_listener_header { };


#define AGT_bqueue_kind(bq) (*static_cast<const bqueue_kind_t*>(bq))


  agt_status_t broadcastSpLocal(bqueue_broadcaster_t broadcaster,   broadcast_message* message) noexcept;
  agt_status_t broadcastMpLocal(bqueue_broadcaster_t broadcaster,   broadcast_message* message) noexcept;
  agt_status_t broadcastSpShared(bqueue_broadcaster_t broadcaster,  broadcast_message* message) noexcept;
  agt_status_t broadcastMpShared(bqueue_broadcaster_t broadcaster,  broadcast_message* message) noexcept;

  broadcast_message* listenForSpLocal(bqueue_listener_t listener, agt_timeout_t timeout) noexcept;
  broadcast_message* listenForMpLocal(bqueue_listener_t listener, agt_timeout_t timeout) noexcept;
  broadcast_message* listenForSpShared(bqueue_listener_t listener, agt_timeout_t timeout) noexcept;
  broadcast_message* listenForMpShared(bqueue_listener_t listener, agt_timeout_t timeout) noexcept;

  inline agt_status_t broadcast(bqueue_broadcaster_t          broadcaster, broadcast_message* message) noexcept {
    using broadcast_pfn = agt_status_t(*)(bqueue_broadcaster_t, broadcast_message*);
    constexpr static broadcast_pfn jmp_table[] = {
        &broadcastSpLocal,
        &broadcastMpLocal,
        &broadcastSpShared,
        &broadcastMpShared
    };
    AGT_assert( AGT_bqueue_kind(broadcaster) < bqueue_kind_max_enum );
    return (jmp_table[AGT_bqueue_kind(broadcaster)])(broadcaster, message);
  }
  inline agt_status_t broadcast(sp_local_bqueue_broadcaster*  broadcaster, broadcast_message* message) noexcept {
    AGT_assert( broadcaster );
    AGT_assert( broadcaster->kind == sp_local_bqueue_kind );
    return broadcastSpLocal(broadcaster, message);
  }
  inline agt_status_t broadcast(mp_local_bqueue_broadcaster*  broadcaster, broadcast_message* message) noexcept {
    AGT_assert( broadcaster );
    AGT_assert( broadcaster->kind == mp_local_bqueue_kind );
    return broadcastMpLocal(broadcaster, message);
  }
  inline agt_status_t broadcast(sp_shared_bqueue_broadcaster* broadcaster, broadcast_message* message) noexcept {
    AGT_assert( broadcaster );
    AGT_assert( broadcaster->kind == sp_shared_bqueue_kind );
    return broadcastSpShared(broadcaster, message);
  }
  inline agt_status_t broadcast(mp_shared_bqueue_broadcaster* broadcaster, broadcast_message* message) noexcept {
    AGT_assert( broadcaster );
    AGT_assert( broadcaster->kind == mp_shared_bqueue_kind );
    return broadcastMpShared(broadcaster, message);
  }

  inline broadcast_message* listenFor(bqueue_listener_t listener, agt_timeout_t timeout) noexcept {

  }
}

#endif//AGATE_BROADCAST_QUEUE_HPP
