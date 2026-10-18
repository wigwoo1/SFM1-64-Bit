#pragma once
#include <cstdint>
template<class Owner,class Services>
void studioPairQueryRecreationCandidate(Owner& owner,Services& services){
 if(services.diagnosticEnabled())services.diagnostic();
 services.recreateTexture(owner);
 if(owner.queryCapability==0){for(unsigned i=0;i<2;++i){owner.queries[i]=nullptr;owner.active[i]=0;}return;}
 for(unsigned i=0;i<2;++i){
  auto* state=services.commandState();services.flush(state);auto* device=services.device(state);
  if(static_cast<uint32_t>(services.createQuery(device,8u,owner.queries[i]))==0x8876086au){services.queryUnavailable();}
  else{services.issue(owner.queries[i],1u);owner.active[i]=1;}
 }
}
