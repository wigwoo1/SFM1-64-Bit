#pragma once
#include <cstdint>
template<class Owner,class Services>void studioQueryCleanupCandidate(Owner& owner,Services& services){
 if(services.diagnosticEnabled())services.diagnostic();
 if(owner.resource){services.release(owner.resource);owner.resource=nullptr;}
 for(unsigned index=0;index<2;++index){
  if(!owner.queries[index])continue;
  if(owner.active[index]){
   const double start=services.time();uint32_t output=0;
   for(;;){const auto result=services.getData(owner.queries[index],output,4u,1u);const double elapsed=services.time()-start;
    if(elapsed>services.limit()||static_cast<int32_t>(result)!=1)break;
   }
  }
  services.release(owner.queries[index]);owner.queries[index]=nullptr;owner.active[index]=0;
 }
}
