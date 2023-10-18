//
// Created by maxwe on 2023-02-28.
//

#include "thread.hpp"
#include "thread_id.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


void agt::thread::do_start(proc_rettype(* pProc)(proc_argtype), thread_info* threadInfo, bool startSuspended) noexcept {
  DWORD tid;
  m_handle = CreateThread(nullptr, 0, pProc, threadInfo, startSuspended ? CREATE_SUSPENDED : 0, &tid);
  m_pInfo = threadInfo;
}

void agt::thread::set_thread_info(thread_info* pInfo) noexcept {
  pInfo->started = true;
  pInfo->tid = get_thread_id();
  pInfo->startedFlag.store(true);
}

void agt::thread::resume() noexcept {
  ResumeThread(m_handle);
}