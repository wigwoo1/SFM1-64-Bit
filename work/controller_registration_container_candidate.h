#pragma once
#include <cstdint>
template<class Adapter>auto getControllerRegistrationContainerCandidate(std::uint32_t create,Adapter& a){
 a.guardService(9,1,263);
 auto* selected=a.cachedContainer();
 if(!selected && create){
  a.beginContainerAllocation(266);
  a.setCachedContainer(a.allocateContainer(0x10431320,0x1043cfa0));
  a.endContainerAllocation();
  selected=a.cachedContainer();
 }
 a.guardService(10,1,272);
 return selected;
}
