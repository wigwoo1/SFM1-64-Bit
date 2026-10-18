#pragma once
#include <array>
#include <cstdint>
// Pool storage and concurrent completion are supplied by the owner.
template<class Services> std::uint32_t* studioDeviceQueueAcquireCandidate(std::array<std::uint32_t*,500>& pool,std::uint32_t header,Services& services){
 for(;;){
  for(unsigned index=0;index<pool.size();++index){if(pool[index][0]!=0)continue;pool[index][0]=header;return pool[index];}
  services.refill();services.waitZero();
 }
}
