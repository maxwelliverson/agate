//
// Created by maxwe on 2022-06-01.
//

#include "agate.h"

#include <vector>
#include <iostream>
#include <span>
#include <chrono>

namespace {

  inline constexpr agt_message_id_t StartMsg  = 1;
  inline constexpr agt_message_id_t TokMsg    = 2;
  inline constexpr agt_message_id_t CalcMsg   = 3;
  inline constexpr agt_message_id_t CheckMsg  = 4;
  inline constexpr agt_message_id_t RetainMsg = 5;
  inline constexpr agt_message_id_t DoneMsg   = 6;


  using factors = std::vector<uint64_t>;

  constexpr uint64_t s_factor1 = 86028157;
  constexpr uint64_t s_factor2 = 329545133;
  constexpr uint64_t s_task_n = s_factor1 * s_factor2;

  factors factorize(uint64_t n) {
    factors result;
    if (n <= 3) {
      result.push_back(n);
      return result;
    }
    uint64_t d = 2;
    while (d < n) {
      if ((n % d) == 0) {
        result.push_back(d);
        n /= d;
      } else {
        d = (d == 2) ? 3 : (d + 2);
      }
    }
    result.push_back(d);
    return result;
  }

  struct supervisor {
    agt_signal_t signal;
    std::chrono::high_resolution_clock::time_point startTime;
    int          left;

    static agt_action_result_t proc(void* p, agt_message_id_t id, void* msg, const agt_agent_message_info_t* msgInfo) noexcept {

      auto sv = (supervisor*)p;

      if (id == CheckMsg) {
        size_t factorCount;
        memcpy(&factorCount, msg, sizeof factorCount);
        auto factors = std::span<uint64_t>{ (uint64_t*)(((char*)msg) + sizeof factorCount), factorCount };
        assert(factors.size() == 2);
        assert(factors[0] == s_factor1);
        assert(factors[1] == s_factor2);
#ifdef NDEBUG
        static_cast<void>(factors);
#endif
      } else {
        assert(id == DoneMsg);
      }

      if (--sv->left == 0) {
        auto endTime = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(endTime - sv->startTime) << std::endl;
        agt_raise_signal(&sv->signal);
      }
    }
  };

  struct chain_master {
    int         iteration;
    int         ring_size;
    uint64_t    initial_token_value;
    int         num_iterations;
    agt_ctx_t   context;
    agt_agent_t self;
    agt_agent_t supervisor;
    agt_agent_t next;
    agt_agent_t factorizer;


    static void new_ring(chain_master* cm) noexcept {
      uint64_t msgValue = s_task_n;
      agt_send_info_t          msg;
      agt_agent_literal_info_t typeInfo;
      agt_agent_create_info_t  createInfo;

      msg.flags          = 0;
      msg.messageSize    = sizeof msgValue;
      msg.messageId      = CalcMsg;
      msg.pMessageBuffer = &msgValue;
      msg.asyncHandle    = nullptr;

      agt_send_as(cm->supervisor, cm->factorizer, &msg);

      cm->next = cm->self;

      typeInfo.destructor     = [](void* p){ agt_release((agt_agent_t)p); };
      typeInfo.dispatchKind   = AGT_DISPATCH_BY_ID;
      typeInfo.idDispatchProc = chain_link_proc;

      createInfo.name             = nullptr;
      createInfo.nameLength       = 0;
      createInfo.flags            = AGT_AGENT_CREATE_DISCONNECTED;
      createInfo.group            = nullptr;
      createInfo.agency           = nullptr;
      createInfo.fixedMessageSize = sizeof(uint64_t);

      agt_agent_t next = cm->self;

      for (int i = 1; i < cm->ring_size; ++i) {
        createInfo.userData = next;
        agt_create_agent_literal(cm->context, &typeInfo, &createInfo, &next);
      }

      msg.messageId = TokMsg;
      msgValue = cm->initial_token_value;

      agt_send(next, &msg);
    }

    static agt_action_result_t worker_proc(void* p, agt_message_id_t id, void* msg_, const agt_agent_message_info_t* msgInfo) noexcept {
      assert(id == CalcMsg);

      uint64_t value;
      memcpy(&value, msg_, sizeof value);
      auto facts = factorize(value);
      size_t factorCount = facts.size();
      auto bufferSize = sizeof(size_t) + (factorCount * sizeof(factors::value_type));
      auto buffer = alloca(bufferSize);
      memcpy(buffer, &factorCount, sizeof factorCount);
      memcpy((char*)buffer + sizeof factorCount, facts.data(), factorCount * sizeof(factors::value_type));


      agt_send_info_t msg;
      msg.messageId = CheckMsg;
      msg.messageSize = bufferSize;
      msg.pMessageBuffer = buffer;
      msg.asyncHandle = nullptr;
      msg.flags = 0;

      agt_reply(&msg);
    }
    static agt_action_result_t chain_link_proc(void* p, agt_message_id_t id, void* msg, const agt_agent_message_info_t* msgInfo) noexcept {
      assert(id == TokMsg);
      auto next = static_cast<agt_agent_t>(p);
      agt_delegate(next);
    }
    static agt_action_result_t proc(void* p, agt_message_id_t id, void* msg, const agt_agent_message_info_t* msgInfo) noexcept {

      auto cm = (chain_master*)p;

      if (id == StartMsg) [[unlikely]] {
        new_ring(cm);
      }
      else {
        assert(id == TokMsg);

        uint64_t value;
        memcpy(&value, msg, sizeof value);

        if (value == 0) {
          if (++cm->iteration < cm->num_iterations) {
            agt_release(cm->next);
            new_ring(cm);
          }
          else {
            agt_release(cm->factorizer);
            agt_release(cm->supervisor);
            agt_release(cm->self);
          }
        }
        else {
          agt_send_info_t sendInfo;
          sendInfo.messageId = TokMsg;
          sendInfo.flags = 0;
          sendInfo.asyncHandle = nullptr;
          sendInfo.pMessageBuffer = &value;
          sendInfo.messageSize = sizeof value;

          --value;

          agt_send(cm->next, &sendInfo);
        }
      }
    }
  };


  void make_chain_master(agt_ctx_t ctx, agt_agent_t sv, int id, int ringSize, uint64_t itv, int repetitions) {
    auto state = new chain_master;

    agt_agent_literal_info_t typeInfo;
    agt_agent_create_info_t createInfo;

    std::string name = "worker#" + std::to_string(id);


    typeInfo.dispatchKind = AGT_DISPATCH_BY_ID;
    typeInfo.destructor = nullptr;
    typeInfo.idDispatchProc = &chain_master::worker_proc;

    createInfo.name = name.data();
    createInfo.nameLength = name.length();
    createInfo.flags = AGT_AGENT_CREATE_DETATCHED;
    createInfo.fixedMessageSize = sizeof(uint64_t);
    createInfo.agency = nullptr;
    createInfo.group = nullptr;
    createInfo.userData = nullptr;

    agt_agent_t worker;

    auto result = agt_create_agent_literal(ctx, &typeInfo, &createInfo, &worker);

    assert(result == AGT_SUCCESS);

    name = "chain-master#" + std::to_string(id);

    typeInfo.destructor = [](void* p){
      auto cm = (chain_master*)p;
      delete cm;
    };
    typeInfo.idDispatchProc = &chain_master::proc;

    createInfo.name = name.data();
    createInfo.nameLength = name.length();
    createInfo.flags = 0;
    createInfo.fixedMessageSize = sizeof(uint64_t);
    createInfo.agency = nullptr;
    createInfo.group = nullptr;

    state->iteration = 0;
    state->ring_size = ringSize;
    state->initial_token_value = itv;
    state->num_iterations = repetitions;
    state->context = ctx;
    state->supervisor = sv;
    state->next = nullptr;
    state->factorizer = worker;

    agt_agent_t cm;

    result = agt_create_agent_literal(ctx, &typeInfo, &createInfo, &cm);

    assert(result == AGT_SUCCESS);

    state->self = cm;

    agt_send_info_t msg;

    msg.flags = 0;
    msg.messageId = StartMsg;
    msg.messageSize = 0;
    msg.pMessageBuffer = nullptr;
    msg.asyncHandle = nullptr;

    agt_send(cm, &msg);
  }

  agt_agent_t make_supervisor(agt_ctx_t ctx, agt_async_t& async, int numRings, int repetitions) {
    agt_new_async(ctx, &async, 0);

    agt_agent_literal_info_t typeInfo;
    agt_agent_create_info_t createInfo;

    typeInfo.dispatchKind = AGT_DISPATCH_BY_ID;
    typeInfo.destructor = [](void* p){
      auto sv = (supervisor*)p;
      agt_destroy_signal(&sv->signal);
      delete sv;
    };
    typeInfo.idDispatchProc = &supervisor::proc;

    createInfo.name = "supervisor";
    createInfo.nameLength = 0;
    createInfo.flags = 0;
    createInfo.fixedMessageSize = 0;
    createInfo.agency = nullptr;
    createInfo.group = nullptr;

    auto state = new supervisor;
    state->left = numRings + (numRings * repetitions);
    state->startTime = std::chrono::high_resolution_clock::now();
    agt_new_signal(ctx, &state->signal);

    createInfo.userData = state;

    agt_agent_t handle;
    auto result = agt_create_agent_literal(ctx, &typeInfo, &createInfo, &handle);
    assert(result == AGT_SUCCESS);

    return handle;
  }
}


int main(int argc, char** argv) {
  if (argc != 5) {
    std::cout << "usage: ring-benchmark "
            "NUM_RINGS RING_SIZE INITIAL_TOKEN_VALUE REPETITIONS\n\n";
    return 1;
  }

  auto numRings = atoi(argv[1]);
  auto ringSize = atoi(argv[2]);
  auto initialTokenValue = static_cast<uint64_t>(atoi(argv[3]));
  auto repetitions = atoi(argv[4]);

  agt_ctx_t ctx;
  agt_init(&ctx);

  agt_async_t async;



  auto sv = make_supervisor(ctx, async, numRings, repetitions);

  for (int i = 0; i < numRings; ++i)
    make_chain_master(ctx, sv, i, ringSize, initialTokenValue, repetitions);

  auto result = agt_wait(&async, AGT_WAIT);

  assert(result == AGT_SUCCESS);

  agt_destroy_async(&async);
  agt_finalize(ctx);
}