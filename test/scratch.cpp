//
// Created by Maxwell on 2024-02-07.
//


#include "agate/core.h"
#include "agate/init.h"

#include <cstdio>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <random>
#include <thread>

#include <Windows.h>


int main(int argc, char** argv) {

  agt_ctx_t ctx;
  agt_default_init(&ctx);

  constexpr static auto print_time = [](agt_ctx_t ctx, agt_timestamp_t timestamp) {
    constexpr static size_t BufSize = 248;
    char buf[BufSize];
    char usBuf[10];

    timespec ts;
    agt_get_timespec(ctx, timestamp, &ts);
    tm tm;
    auto err = localtime_s(&tm, &ts.tv_sec);
    size_t sz = strftime(buf, BufSize, "%c ( ######### ns )", &tm);
    auto bufPos = (char*)memchr(buf, '#', sz);
    size_t remainingSize = (buf + sz) - bufPos;
    sprintf_s(usBuf, "%.9ld", ts.tv_nsec);
    memcpy(bufPos, usBuf, sizeof(usBuf) - 1);
    printf("%s\n", &buf[0]);
  };

  const auto start = agt_now(ctx);

  Sleep(500);

  const auto end = agt_now(ctx);

  print_time(ctx, start);

  Sleep(4000);

  print_time(ctx, end);


  agt_finalize(ctx);


  return 0;
}