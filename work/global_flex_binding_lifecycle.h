#pragma once
#include <cstdint>
struct RecoveredFlexBindingCache {uint32_t destination=0xffffffffu,index=0xffffffffu;};
// Native105C3700 is invoked with the secondary interface pointer, base+4.
template<class ResolveModel,class LookupIndex,class BindDestination>
inline void recoveredFlexBindingNotificationCandidate(RecoveredFlexBindingCache&cache,bool registryAvailable,bool modelAttributeMatches,ResolveModel resolveModel,LookupIndex lookupIndex,BindDestination bindDestination){if(!registryAvailable||!modelAttributeMatches||!resolveModel())return;cache.index=lookupIndex();cache.destination=bindDestination();}

// Native105C36B0 retries negative controller indices and only unset destinations.
template<class LookupIndex,class BindDestination>
inline void recoveredLazyFlexBindingCandidate(RecoveredFlexBindingCache&cache,LookupIndex lookupIndex,BindDestination bindDestination){if(cache.index&0x80000000u)cache.index=lookupIndex();if(cache.destination==0xffffffffu){auto destination=bindDestination();if(destination)cache.destination=*destination;}}
