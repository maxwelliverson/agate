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

#include <iostream>

#include <random>
#include <thread>
#include <format>
#include <chrono>

#include <Windows.h>

#pragma comment(lib, "Mincore.lib")


class Sleeper {
  HANDLE   timer;

  using native_duration_int = int64_t;
  using native_timer_resolution = std::ratio<1, 10000000>;
  using native_duration = std::chrono::duration<native_duration_int, native_timer_resolution>;

  inline static LARGE_INTEGER get_system_time() noexcept {
    FILETIME systime;
    LARGE_INTEGER lrgInt;
    GetSystemTimePreciseAsFileTime(&systime);
    lrgInt.LowPart = systime.dwLowDateTime;
    lrgInt.HighPart = systime.dwHighDateTime;
    return lrgInt;
  }

  void do_sleep(native_duration_int duration) noexcept {
    if (duration <= 0) [[unlikely]]
      return;
    LARGE_INTEGER startTime = get_system_time();
    LARGE_INTEGER dueTime;
    dueTime.QuadPart = startTime.QuadPart + duration;

    if (duration > 5000) {
      LARGE_INTEGER sleepDueTime;
      const auto roundedSleepDuration = duration - (duration % 5000);
      sleepDueTime.QuadPart = startTime.QuadPart + roundedSleepDuration;
      SetWaitableTimerEx(timer, &sleepDueTime, 0, nullptr, nullptr, nullptr, 0);
      do {
        WaitForSingleObjectEx(timer, INFINITE, TRUE);
      } while(get_system_time().QuadPart < sleepDueTime.QuadPart); // This is needed because it is *possible*, though unlikely, that this is woken prematurely.
    }

    uint32_t backoff = 0;

    while(get_system_time().QuadPart < dueTime.QuadPart) {
      _mm_pause();
      for (uint32_t i = 0; i < backoff; ++i) {
        for (uint32_t j = 0; j < i; ++j)
          _mm_pause();
        _mm_pause();
      }
      if (backoff < 8)
        ++backoff;
    }

  }
public:

  Sleeper()
      : timer(CreateWaitableTimerEx(nullptr,
                                    nullptr,
                                    CREATE_WAITABLE_TIMER_MANUAL_RESET | CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                    TIMER_ALL_ACCESS))
  { }

  ~Sleeper() {
    if (timer)
      CloseHandle(timer);
  }

  template <typename Rep, typename Period>
  void sleep(const std::chrono::duration<Rep, Period>& duration) noexcept {
    do_sleep(std::chrono::duration_cast<native_duration>(duration).count());
  }
};


void printTimespec(const timespec& ts) noexcept {
  constexpr static size_t BufSize = 248;
  char buf[BufSize];
  char usBuf[10];

  tm tm;
  auto err = localtime_s(&tm, &ts.tv_sec);
  size_t sz = strftime(buf, BufSize, "%c ( ######### ns )", &tm);
  auto bufPos = (char*)memchr(buf, '#', sz);
  size_t remainingSize = (buf + sz) - bufPos;
  sprintf_s(usBuf, "%.9ld", ts.tv_nsec);
  memcpy(bufPos, usBuf, sizeof(usBuf) - 1);
  printf("%s\n", &buf[0]);
}


void printTime(const FILETIME& time) noexcept {
  FILETIME localTime;
  SYSTEMTIME sysTime;
  FileTimeToLocalFileTime(&time, &localTime);
  FileTimeToSystemTime(&localTime, &sysTime);

  LARGE_INTEGER lrgInt;
  lrgInt.LowPart = time.dwLowDateTime;
  lrgInt.HighPart = time.dwHighDateTime;

  lrgInt.QuadPart -= (11644473600LL * 10000000);

  timespec ts;
  ts.tv_sec = lrgInt.QuadPart / 10000000;
  ts.tv_nsec = (lrgInt.QuadPart % 10000000) * 100;
  printTimespec(ts);
}


void printSystemTimes() noexcept {
  FILETIME a, b, c, d, e;
  SwitchToThread();
  GetSystemTimePreciseAsFileTime(&a);
  GetSystemTimePreciseAsFileTime(&b);
  GetSystemTimePreciseAsFileTime(&c);
  GetSystemTimePreciseAsFileTime(&d);
  SleepEx(1, TRUE);
  GetSystemTimePreciseAsFileTime(&e);
  printTime(a);
  printTime(b);
  printTime(c);
  printTime(d);
  printTime(e);

  DWORD64 adjustment, increment;
  BOOL isDisabled;
  GetSystemTimeAdjustmentPrecise(&adjustment, &increment, &isDisabled);
  printf("\nAdjustment: %llu ns\n"
           "Increment:  %llu ns\n"
           "IsEnabled:  %s\n", adjustment * 100, increment * 100, isDisabled ? "false" : "true");
}


template <typename Rep, typename Period>
void testSleep(agt_ctx_t ctx, Sleeper& self, const std::chrono::duration<Rep, Period>& duration) noexcept {
  using duration_t = std::chrono::duration<Rep, Period>;
  using rep_t = Rep;

  const auto start = agt_now(ctx);

  self.sleep(duration);

  const auto end = agt_now(ctx);


  const std::chrono::nanoseconds actualDuration{static_cast<int64_t>(agt_duration_between(ctx, start, end))};


  std::cout << std::format("Expected Sleep Duration: {}\n"
                           "Actual Sleep Duration:   {}\n",
                           duration,
                           std::chrono::duration_cast<duration_t>(actualDuration)) << std::endl;
}

void printAgateTimes() noexcept {

  using namespace std::chrono_literals;

  agt_ctx_t ctx;
  agt_default_init(&ctx);

  Sleeper self;

  /*constexpr static auto print_time = [](agt_ctx_t ctx, agt_timestamp_t timestamp) {
    timespec ts;
    agt_get_timespec(ctx, timestamp, &ts);
    printTimespec(ts);
  };

  const auto start = agt_now(ctx);

  self.sleep(777us);

  const auto end = agt_now(ctx);

  auto duration = agt_duration_between(ctx, start, end);

  printf("Duration between: ~%llu ms\n\n", duration / 1000000);

  print_time(ctx, start);

  print_time(ctx, end);*/

  testSleep(ctx, self, 777us);
  testSleep(ctx, self, 1153ns);
  testSleep(ctx, self, 5.3ms);
  testSleep(ctx, self, 99.384ms);
  testSleep(ctx, self, 2.83748s);


  agt_finalize(ctx);
}


int main(int argc, char** argv) {

  // printSystemTimes();

  printAgateTimes();


  return 0;
}