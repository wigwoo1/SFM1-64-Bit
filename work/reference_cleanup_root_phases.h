#pragma once
#include <cstdint>
// 1035B920..1035BAED. enumerateRaw visits eligible raw slots in ascending order.
// Eligibility uses non-null pointer, index fitting22 bits and low31 state <=511;
// it does not reject bit31 (retired) separately. Snapshot raw count once per pass.
template<class EnumerateRaw,class Registration,class Mark,class Dependencies,class Resolve>
void recoveredReferenceCleanupRootPhasesCandidate(EnumerateRaw enumerateRaw,Registration registration,Mark mark,Dependencies dependencies,Resolve resolve){
 enumerateRaw([&](auto& e){if(e.cleanupContext!=0xffffffffu){registration(e).setEligible(false,0);registration(e).setEligible(false,1);}});
 enumerateRaw([&](auto& e){if(e.cleanupContext!=0xffffffffu&&(e.record.counts&0x3ff00000u))mark(e,1,0,1);});
 dependencies([&](uint32_t handle){if(auto*e=resolve(handle))mark(*e,1,0,0);});
}
