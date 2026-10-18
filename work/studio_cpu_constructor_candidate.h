#pragma once
#include <vector>
#include "studio_cpu_constructor_masks_candidate.h"
struct StudioCpuConstructorResultCandidate{bool valid=false,restorationFailed=false;std::array<uint8_t,8> masks{};std::vector<uint8_t> ids;};
// Successful service calls preserve recovered order. Failure checks and
// exception cleanup deliberately adapt the original unchecked behavior.
template<class Services>StudioCpuConstructorResultCandidate studioCpuConstructorCandidate(Services& services){
 StudioCpuConstructorResultCandidate result;if(!studioCpuConstructorMasksCandidate(services,result.masks))return result;
 auto process=services.currentProcess();auto thread=services.currentThread();uintptr_t processMask=0,systemMask=0;if(!services.affinityMasks(process,processMask,systemMask)||!processMask||!systemMask)return result;
 if(systemMask==1){result.ids.push_back(0);result.valid=true;return result;}
 if(processMask!=systemMask&&!services.setProcessAffinity(process,systemMask))return result;
 uintptr_t originalThread=0;
 auto restore=[&]() noexcept {bool complete=true;
  try{if(!services.setProcessAffinity(process,processMask))complete=false;}catch(...){complete=false;}
  if(originalThread)try{if(!services.setThreadAffinity(thread,originalThread))complete=false;}catch(...){complete=false;}
  result.restorationFailed=!complete;return complete;
 };
 try{
 for(uintptr_t bit=1;bit&&bit<=systemMask;bit<<=1){
  if(!(systemMask&bit))continue;
  const auto previous=services.setThreadAffinity(thread,bit);if(!previous){restore();return result;}if(!originalThread)originalThread=previous;
  services.yield();const auto leaf=services.maximumBasic()>=1?services.cpuid(1u,0u):std::array<uint32_t,4>{};result.ids.push_back(static_cast<uint8_t>(leaf[1]>>24));
 }
 }catch(...){restore();throw;}
 if(!restore())return result;services.yield();result.valid=true;return result;
}
