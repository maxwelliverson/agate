//
// Created by maxwe on 2024-03-17.
//

#include "test_prolog.hpp"

#include "agate/async.h"
#include "agate/init.h"
#include "agate/channels.h"
#include "agate/naming.h"


#include <string_view>
#include <queue>
#include <stack>
#include <span>
#include <random>



template <typename T>
using result = std::pair<agt_status_t, T>;

using queue_t = std::pair<agt_sender_t, agt_receiver_t>;





result<agt_name_t>     reserve_name(agt_ctx_t ctx, std::string_view name) noexcept {
  if (name.empty())
    return { AGT_SUCCESS, AGT_ANONYMOUS };
  agt_string_t nameStr{
    .data = name.data(),
    .length = name.length()
  };
  agt_name_desc_t desc{
    .async = AGT_SYNCHRONIZE,
    .name = {
      .data = name.data(),
      .length = name.length()
    },
    .flags = 0,
    .retainMask = 0,
    .paramBuffer = nullptr
  };
  agt_name_result_t result;
  auto status = agt_reserve_name(ctx, &desc, &result);
  return { status, result.name };
}

agt_status_t           open_channel(agt_ctx_t ctx, std::string_view name, agt_sender_t* pSender, agt_receiver_t* pReceiver, const agt_channel_desc_t& chDesc) noexcept {
  auto [ status, nameTok ] = reserve_name(ctx, name);
  if (status != AGT_SUCCESS)
    return status;
  agt_channel_desc_t desc = chDesc;
  desc.async = AGT_SYNCHRONIZE;
  desc.name = nameTok;
  desc.flags = 0;
  status = agt_open_channel(ctx, pSender, pReceiver, &desc);
  if (status != AGT_SUCCESS)
    agt_release_name(ctx, nameTok);
  return status;
}

result<queue_t>        open_private_channel(agt_ctx_t ctx, std::string_view name = {}) noexcept {
  agt_channel_desc_t desc {
    .scope        = AGT_CTX_SCOPE,
    .maxSenders   = 1,
    .maxReceivers = 1
  };
  queue_t queue;
  auto status = open_channel(ctx, name, &queue.first, &queue.second, desc);
  return { status, queue };
}

result<queue_t>        open_local_spsc_channel(agt_ctx_t ctx, std::string_view name = {}) noexcept {
  agt_channel_desc_t desc {
    .scope        = AGT_INSTANCE_SCOPE,
    .maxSenders   = 1,
    .maxReceivers = 1
  };
  queue_t queue;
  auto status = open_channel(ctx, name, &queue.first, &queue.second, desc);
  return { status, queue };
}

result<agt_sender_t>   open_local_spmc_channel(agt_ctx_t ctx, std::string_view name = {}) noexcept {
  agt_channel_desc_t desc {
    .scope        = AGT_INSTANCE_SCOPE,
    .maxSenders   = 1,
    .maxReceivers = AGT_NO_LIMIT
  };
  agt_sender_t sender;
  auto status = open_channel(ctx, name, &sender, nullptr, desc);
  return { status, sender };
}

result<agt_receiver_t> open_local_mpsc_channel(agt_ctx_t ctx, std::string_view name = {}) noexcept {
  agt_channel_desc_t desc {
    .scope        = AGT_INSTANCE_SCOPE,
    .maxSenders   = AGT_NO_LIMIT,
    .maxReceivers = 1,
  };
  agt_receiver_t receiver;
  auto status = open_channel(ctx, name, nullptr, &receiver, desc);
  return { status, receiver };
}

result<queue_t>        open_local_mpmc_channel(agt_ctx_t ctx, std::string_view name = {}) noexcept {
  agt_channel_desc_t desc {
    .scope        = AGT_CTX_SCOPE,
    .maxSenders   = AGT_NO_LIMIT,
    .maxReceivers = AGT_NO_LIMIT,
  };
  queue_t queue;
  auto status = open_channel(ctx, name, &queue.first, &queue.second, desc);
  return { status, queue };
}


static agt_ctx_t get_ctx() noexcept {
  agt_ctx_t ctx;
  agt_default_init(&ctx);
  return ctx;
  /*thread_local agt_ctx_t ctx = [] {
    agt_ctx_t c;
    auto result = agt_default_init(&c);
    return c;
  }();
  return ctx;*/
}


TEST_CASE("Channels can be created and destroyed.", "[channels]") {

  context ctx{};

  SECTION("Private") {
    auto [ status, queue ] = open_private_channel(ctx);
    auto [ sender, receiver ] = queue;
    REQUIRE( status == AGT_SUCCESS );
    REQUIRE( sender != nullptr );
    REQUIRE( receiver != nullptr );
    agt_close(sender);
    agt_close(receiver);
  }

  SECTION("Local SPSC") {
    auto [ status, queue ] = open_local_spsc_channel(ctx);
    auto [ sender, receiver ] = queue;
    REQUIRE( status == AGT_SUCCESS );
    REQUIRE( sender != nullptr );
    REQUIRE( receiver != nullptr );
    agt_close(sender);
    agt_close(receiver);
  }

  SECTION("Local MPSC") {
    auto [ status, receiver ] = open_local_mpsc_channel(ctx);
    REQUIRE( status == AGT_SUCCESS );
    REQUIRE( receiver != nullptr );
    agt_close(receiver);
  }

  SECTION("Local SPMC") {
    auto [ status, sender ] = open_local_spmc_channel(ctx);
    REQUIRE( status == AGT_SUCCESS );
    REQUIRE( sender != nullptr );
    agt_close(sender);
  }

  SECTION("Local MPMC") {
    auto [ status, queue ] = open_local_mpmc_channel(ctx);
    auto [ sender, receiver ] = queue;
    REQUIRE( status == AGT_SUCCESS );
    REQUIRE( sender != nullptr );
    REQUIRE( receiver != nullptr );
    agt_close(sender);
    agt_close(receiver);
  }
}


TEST_CASE("Private channel can send and receive messages", "[channels]") {
  context ctx{};

  auto [ status, queue ] = open_private_channel(ctx);
  auto [ sender, receiver ] = queue;

  REQUIRE( status == AGT_SUCCESS );



  constexpr static char   SrcMessage[] = "This is the source message";
  constexpr static size_t SrcMessageLength = sizeof(SrcMessage) - 1;
  constexpr static agt_u64_t ResponseValue = 0xDEADBEEF;

  agt_async_t async = GENERATE(AGT_FORGET, AGT_SYNCHRONIZE);

  if (async == AGT_SYNCHRONIZE)
    async = agt_new_async(ctx, 0);

  /*SECTION("Fire and forget") {
    async = AGT_FORGET;
  }

  SECTION("Wait on response") {
    async = agt_new_async(ctx, 0);
  }*/

  void* srcMsgBuffer = nullptr;
  void* msg = nullptr;
  size_t msgSize = 0;
  agt_u64_t resultValue = 0;

  status = agt_receive_msg(receiver, &msg, &msgSize, 0);

  REQUIRE( status == AGT_ERROR_NO_MESSAGES );


  status = agt_acquire_msg(sender, SrcMessageLength, &srcMsgBuffer, 0);

  REQUIRE( status == AGT_SUCCESS );
  REQUIRE( srcMsgBuffer != nullptr );

  status = agt_receive_msg(receiver, &msg, &msgSize, 0);

  REQUIRE( status == AGT_ERROR_NO_MESSAGES );

  std::memcpy(srcMsgBuffer, SrcMessage, SrcMessageLength);

  status = agt_send_msg(sender, srcMsgBuffer, SrcMessageLength, async);

  REQUIRE( status == AGT_SUCCESS );

  status = agt_receive_msg(receiver, &msg, &msgSize, 0);

  REQUIRE( status == AGT_SUCCESS );
  REQUIRE( msg != nullptr );
  REQUIRE( msgSize == SrcMessageLength );

  std::string_view dstMsg{ static_cast<char*>(msg), msgSize };

  REQUIRE( dstMsg == SrcMessage ); // This is the real key, testing if the message contents have successfully been sent over.

  if (async != AGT_FORGET) {

    status = agt_async_status(async, &resultValue);

    REQUIRE( status == AGT_NOT_READY );
  }

  agt_retire_msg(receiver, msg, ResponseValue);

  if (async != AGT_FORGET) {

    resultValue = 0;
    status = agt_async_status(async, &resultValue);

    REQUIRE( status == AGT_SUCCESS );
    REQUIRE( resultValue == ResponseValue );


    agt_destroy_async(async);
  }

  agt_close(sender);
  agt_close(receiver);
}


TEST_CASE("Private channel messages are FIFO ordered.", "[channels]") {
  context ctx{};

  auto [ status, queue ] = open_private_channel(ctx);
  auto [ sender, receiver ] = queue;

  constexpr static uint32_t MaxMsgSize = 256;

  uint32_t msgCount = GENERATE(take(20, random(3, 100)));

  std::mt19937 rng{ msgCount };

  std::queue<std::vector<std::byte>> msgContents;


  for (uint32_t i = 0; i < msgCount; ++i) {
    std::vector<std::byte> proxyMsg;
    const uint32_t msgLength = rng() % MaxMsgSize;
    proxyMsg.reserve(msgLength);
    for (uint32_t j = 0; j < msgLength; ++j) {
      proxyMsg.push_back(static_cast<std::byte>(rng()));
    }

    void* msg = nullptr;
    status = agt_acquire_msg(sender, msgLength, &msg, 0);

    REQUIRE( status == AGT_SUCCESS );
    REQUIRE( msg != nullptr );

    std::memcpy(msg, proxyMsg.data(), msgLength);

    status = agt_send_msg(sender, msg, msgLength, AGT_FORGET);

    REQUIRE( status == AGT_SUCCESS );

    msgContents.push(std::move(proxyMsg));
  }


  uint32_t receivedMsgCount = 0;

  do {
    void*  msg = nullptr;
    size_t msgSize = 0;
    status = agt_receive_msg(receiver, &msg, &msgSize, 0);

    if (status == AGT_ERROR_NO_MESSAGES)
      break;

    ++receivedMsgCount;

    if (receivedMsgCount > msgCount)
      break;

    REQUIRE( status == AGT_SUCCESS );
    REQUIRE( msg != nullptr );
    REQUIRE( msgSize <= MaxMsgSize );

    auto proxyMsg = std::move(msgContents.front());
    msgContents.pop();

    REQUIRE( msgSize == proxyMsg.size() );

    std::span receivedMsg{ static_cast<std::byte*>(msg), msgSize };

    REQUIRE_THAT( receivedMsg, Catch::Matchers::RangeEquals(proxyMsg) );

    agt_retire_msg(receiver, msg, 0);
  } while(true);

  REQUIRE( receivedMsgCount == msgCount );

  agt_close(sender);
  agt_close(receiver);
}