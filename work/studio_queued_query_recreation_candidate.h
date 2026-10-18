#pragma once
#include <cstdint>
template<class Owner,class Services>
void studioQueuedQueryRecreationCandidate(Owner& owner,Services& services){
 if(owner.queryCapability!=1){owner.resourceQueueActive=0;for(auto& slot:owner.queuedResources)slot=nullptr;return;}
 owner.resourceQueueActive=1;
 for(auto& slot:owner.queuedResources){
  auto* state=services.commandState();services.flush(state);auto* device=services.device(state);
  if(static_cast<uint32_t>(services.createQuery(device,8u,slot))==0x8876086au){slot=nullptr;owner.resourceQueueActive=0;}
 }
}
