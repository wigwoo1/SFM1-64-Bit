#pragma once
#include <cstdint>

// Full server10267610 predicate. Both base resolutions precede owner checks;
// equal base identities do not imply equal handle identities or an early exit.
template<class Services>
bool sourceServerPassEntityFilterCandidate(uintptr_t candidate,uintptr_t ignored,Services& services){
 if(!ignored)return true;
 if(candidate==ignored)return false;
 uintptr_t base=0,ignoredBase=0;
 if(!services.isStaticProp(candidate))base=services.baseEntity(candidate);
 if(!services.isStaticProp(ignored))ignoredBase=services.baseEntity(ignored);
 if(!base||!ignoredBase)return true;
 if(services.ownerEntity(base)==ignoredBase)return false;
 return services.ownerEntity(ignoredBase)!=base;
}
