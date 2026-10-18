#pragma once
#include <Windows.h>
#include "native_cpu_mask_services_candidate.h"
// Legacy primary-group API binding. Use in isolated initialization only;
// process-wide affinity coordination and all-group scheduling are external.
struct NativeCpuConstructorServicesCandidate:NativeCpuMaskServicesCandidate{
 HANDLE currentProcess(){return GetCurrentProcess();}HANDLE currentThread(){return GetCurrentThread();}
 bool affinityMasks(HANDLE process,uintptr_t& processMask,uintptr_t& systemMask){DWORD_PTR p=0,s=0;if(!GetProcessAffinityMask(process,&p,&s))return false;processMask=p;systemMask=s;return true;}
 bool setProcessAffinity(HANDLE process,uintptr_t mask){return SetProcessAffinityMask(process,mask)!=FALSE;}
 uintptr_t setThreadAffinity(HANDLE thread,uintptr_t mask){return SetThreadAffinityMask(thread,mask);}
 void yield(){Sleep(0);}
};
