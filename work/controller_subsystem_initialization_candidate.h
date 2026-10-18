#pragma once
#include <cstdint>
template<class Adapter>
std::uint32_t initializeControllerSubsystemCandidate(std::uint32_t flags,Adapter& a){
 const auto previous=a.referenceCount();a.setReferenceCount(previous+1);
 if(previous)return 0;
 a.installAllocatorHooks();
 if((flags&1) && !a.initialize27d0())return 2;
 if((flags&2) && a.initialize0d60()!=0)return 2;
 a.setSubsystemFlags(flags);a.initialize25e0();return 0;
}
