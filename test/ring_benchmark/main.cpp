//
// Created by maxwe on 2022-06-01.
//

#include "agate_test.hpp"

using factors = std::vector<uint64_t>;

inline static constexpr uint32_t CheckMsg = 1;
inline static constexpr uint32_t DoneMsg  = 2;
inline static constexpr uint64_t s_factor1 = 86028157;
inline static constexpr uint64_t s_factor2 = 329545133;
inline static constexpr uint64_t s_task_n = s_factor1 * s_factor2;


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


struct worker {
  inline constexpr static size_t static_message_size = sizeof(uint64_t);

  static void proc(agt_self_t self, const void* message, agt_size_t msgSize) noexcept {
    assert(msgSize == static_message_size);

    constexpr static size_t MaxFactorCount = 8;

    std::byte buffer[sizeof(uint64_t) * (MaxFactorCount + 1)];

    uint64_t value;
    memcpy(&value, message, sizeof value);
    auto facts = factorize(value);
    uint32_t msgId = CheckMsg;
    uint32_t factorCount = facts.size();
    assert( factorCount <= MaxFactorCount );

    auto bufferSize = sizeof(size_t) + (factorCount * sizeof(factors::value_type));
    memcpy(buffer, &msgId, sizeof msgId);
    memcpy(buffer + sizeof msgId, &factorCount, sizeof factorCount);
    memcpy(buffer + (2 * sizeof(uint32_t)), facts.data(), factorCount * sizeof(factors::value_type));

    agt_send_info_t msg;
    msg.size = bufferSize;
    msg.buffer = buffer;
    msg.async = nullptr;
    msg.flags = 0;

    agt_reply(self, &msg);
  }
};

struct supervisor {
  agt_signal_t signal;
  std::chrono::high_resolution_clock::time_point startTime;
  int          left;

  supervisor(agt_ctx_t ctx, agt_async_t& async, int numRings, int repetitions) {
    agt_new_async(ctx, &async, 0);
    agt_new_signal(ctx, &signal);
    agt_attach_signal(signal, &async);
    left = numRings + (numRings * repetitions);
    startTime = std::chrono::high_resolution_clock::now();
  }

  ~supervisor() {
    agt_destroy_signal(signal);
  }

  void proc(agt_self_t self, const void* message, agt_size_t messageSize) noexcept {

    auto bytes = static_cast<const std::byte*>(message);

    uint32_t msgId;

    memcpy(&msgId, bytes, sizeof(uint32_t));

    if (msgId == CheckMsg) {
      uint32_t factorCount;
      memcpy(&factorCount, bytes + sizeof(uint32_t), sizeof(uint32_t));
      auto factors = std::span<uint64_t>{ (uint64_t*)(bytes + (2*sizeof(uint32_t))), factorCount };
      assert(factors.size() == 2);
      assert(factors[0] == s_factor1);
      assert(factors[1] == s_factor2);
#ifdef NDEBUG
      static_cast<void>(factors);
#endif
    } else {
      assert(msgId == DoneMsg);
    }

    if (--left == 0) {
      auto endTime = std::chrono::high_resolution_clock::now();
      std::cout << std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(endTime - startTime) << std::endl;
      agt_raise_signal(signal);

      agt_exit(self, 0);
    }
  }
};

struct chain_master {
  int         iteration;
  int         ring_size;
  uint64_t    initial_token_value;
  int         num_iterations;
  agt_agent_t supervisor;
  agt_agent_t next;
  agt_agent_t factorizer;

  inline constexpr static size_t static_message_size = sizeof(uint64_t);


  chain_master(int ringSize, uint64_t itv, int repetitions, agt_agent_t sv, agt_agent_t worker)
      : iteration(0),
        ring_size(ringSize),
        initial_token_value(itv),
        num_iterations(repetitions),
        supervisor(sv),
        next(nullptr),
        factorizer(worker)
  {}

  void new_ring(agt_self_t self) noexcept {
    uint64_t msgValue = s_task_n;
    agt_send_info_t          msg;
    agt_agent_create_info_t  createInfo;

    auto context = agt_current_context();

    msg.flags  = 0;
    msg.size   = sizeof msgValue;
    msg.buffer = &msgValue;
    msg.async  = nullptr;

    agt_send_as(self, supervisor, factorizer, &msg);

    createInfo.flags            = 0;
    createInfo.name             = AGT_INVALID_NAME_TOKEN;
    createInfo.fixedMessageSize = sizeof(uint64_t);
    createInfo.initFn           = nullptr;
    createInfo.procFn           = [](agt_self_t self, void* state, const void* msg, size_t msgSize){
      assert( msgSize == sizeof(uint64_t) );
      (void)msg;
      agt_delegate(self, (agt_agent_t)state);
    };
    createInfo.dtorFn           = nullptr;
    createInfo.owner            = nullptr;

    agt_agent_t prev = nullptr;
    agt_agent_t nextAgt;

    agt_retain_self(self, &nextAgt);

    for (int i = 1; i < ring_size; ++i) {
      createInfo.state = nextAgt;
      agt_transfer_owner(context, prev, nextAgt); // While this will attempt to transfer ownership of self to the agent at the end of the chain, this will fail, given that self is detatched.
      prev = nextAgt;
      auto result = agt_create_agent(context, &createInfo, &nextAgt);
      assert(result == AGT_SUCCESS);
    }

    this->next = nextAgt;

    msgValue = initial_token_value;

    agt_send(self, nextAgt, &msg);
  }

  void init(agt_self_t self) noexcept {
    auto result = agt_retain(&supervisor, supervisor);
    assert(result == AGT_SUCCESS);
    result = agt_take_ownership(self, factorizer);
    assert(result == AGT_SUCCESS);
    new_ring(self);
  }

  void proc(agt_self_t self, const void* message, agt_size_t messageSize) noexcept {
    // assert(msgId == TokMsg);
    assert(messageSize == static_message_size);

    uint64_t value;
    memcpy(&value, message, sizeof value);

    if (value == 0) {
      if (++iteration < num_iterations) {
        agt_release(self, next); // By transitive ownership, this will release the entire ring
        new_ring(self);
      }
      else {
        /*agt_release(cm->factorizer);
            agt_release(cm->supervisor);*/
        agt_exit(self, 0);
      }
    }
    else {
      agt_send_info_t sendInfo;
      sendInfo.flags  = 0;
      sendInfo.async  = nullptr;
      sendInfo.buffer = &value;
      sendInfo.size   = sizeof value;

      --value;

      agt_send(self, next, &sendInfo);
    }
  }
};


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


  agt_ctx_t ctx = initialize_library();

  agt_async_t async;
  agt_agent_t sv;

  auto result = make_agent<supervisor>(
      ctx,
      name("supervisor"),
      detached,
      args(ctx, async, numRings, repetitions),
      return_to(sv)
  );

  assert( result == AGT_SUCCESS );

  for (int i = 0; i < numRings; ++i) {
    std::string idString = std::to_string(i);
    agt_agent_t workerAgent;

    result = make_agent<worker>(
        ctx,
        name("worker#" + idString),
        busy,
        return_to(workerAgent)
    );

    assert( result == AGT_SUCCESS );

    result = make_agent<chain_master>(
        ctx,
        name("chain_master#" + idString),
        detached,
        args(ringSize, initialTokenValue, repetitions, sv, workerAgent)
    );

    assert( result == AGT_SUCCESS );
  }

  result = agt_wait(&async, AGT_WAIT);

  assert(result == AGT_SUCCESS);

  agt_destroy_async(&async);
  agt_finalize(ctx);
}