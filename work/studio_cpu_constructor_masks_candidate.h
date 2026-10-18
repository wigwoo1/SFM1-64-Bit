#pragma once
#include "studio_cpu_id_masks_candidate.h"
// Services supplies cached maximum leaves, vendor predicate, and CPUID
// register snapshots. Exact native lazy service ordering remains external.
template<class Services>bool studioCpuConstructorMasksCandidate(Services& services,std::array<uint8_t,8>& output){
 uint32_t total=1,cores=1;
 if(services.maximumBasic()>=1){
  const auto leaf=services.cpuid(1u,0u);
  if(leaf[3]&0x10000000u){
   total=(leaf[1]>>16)&255u;
   if(services.isIntel()){
    if(services.maximumBasic()>=4)cores=(services.cpuid(4u,0u)[0]>>26)+1u;
   }else if(services.maximumExtended()>=8){
    // Preserve original comparison >=8, rather than inventing a modern
    // leaf-support rule. Caller service must distinguish this legacy policy.
    const uint32_t ecx=services.cpuid(0x80000008u,0u)[2];
    const unsigned bits=(ecx>>12)&15u;cores=bits?(1u<<bits):((ecx&255u)+1u);
   }
  }
 }
 return studioCpuIdMasksCandidate(static_cast<uint8_t>(total),static_cast<uint8_t>(cores),output);
}
