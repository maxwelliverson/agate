//
// Created by maxwe on 2023-02-28.
//

#include "thread_pool.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

VOID
    CALLBACK
    MyWorkCallback(
        PTP_CALLBACK_INSTANCE Instance,
        PVOID                 Parameter,
        PTP_WORK              Work
    )
{
  // Instance, Parameter, and Work not used in this example.
  UNREFERENCED_PARAMETER(Instance);
  UNREFERENCED_PARAMETER(Parameter);
  UNREFERENCED_PARAMETER(Work);

  DWORD threadId = GetCurrentThreadId();

  BOOL bRet = FALSE;

  //
  // Do something when the work callback is invoked.
  //
  {
    _tprintf(_T("MyWorkCallback: ThreadId = %d Task performed.\n"), threadId);
  }

  return;
}

int main()
{
  TP_CALLBACK_ENVIRON CallBackEnviron;
  PTP_POOL pool = NULL;
  PTP_CLEANUP_GROUP cleanupgroup = NULL;
  PTP_WORK_CALLBACK workcallback = MyWorkCallback;
  PTP_TIMER timer = NULL;
  PTP_WORK work = NULL;


  InitializeThreadpoolEnvironment(&CallBackEnviron);
  pool = CreateThreadpool(NULL);
  SetThreadpoolThreadMaximum(pool, 1);
  SetThreadpoolThreadMinimum(pool, 3);
  cleanupgroup = CreateThreadpoolCleanupGroup();
  SetThreadpoolCallbackPool(&CallBackEnviron, pool);
  SetThreadpoolCallbackCleanupGroup(&CallBackEnviron, cleanupgroup, NULL);
  work = CreateThreadpoolWork(workcallback, NULL, &CallBackEnviron);
  for (int i = 0; i < 10; ++i)
  {
    SubmitThreadpoolWork(work);
  }
}