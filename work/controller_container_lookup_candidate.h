#pragma once
#include <cstdint>
template<class Adapter>auto lookupControllerContainerCandidate(const std::uint32_t* key,Adapter& a){
 a.setLookupError(0);std::uint32_t output;
 auto* node=*a.findNodeSlot(key,&output);
 if(!node){a.setMissCount(a.missCount()+1);return a.nullPayload();}
 auto payload=a.nodePayload(node);a.setHitCount(a.hitCount()+1);return payload;
}
