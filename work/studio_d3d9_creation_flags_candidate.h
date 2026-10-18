#pragma once
#include <cstdint>
// Native1002a692..1002a6ce: mode can change across CPU callback, and the
// second mode read and CPU read are lazy. CPUInformation byte6 semantics
// are deliberately left to the service binding; flag4 is MULTITHREADED.
template<class Services>uint32_t studioD3D9CreationFlagsCandidate(uint32_t flags,Services& services){
 const auto mode=services.queueMode();
 if(mode==2)return flags|4u;
 if(mode==-2&&services.cpuByte6()>=2)return flags|4u;
 if(services.queueMode()==-1&&services.cpuByte6()>=2)return flags|4u;
 return flags;
}
