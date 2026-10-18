#pragma once
#include <cstdint>
struct StudioCpuCountPublicationCandidate{uint8_t logical,physical;bool lowerPhysical;};
template<class Services>StudioCpuCountPublicationCandidate studioCpuCountPublicationCandidate(Services& services){
 uint8_t logical=static_cast<uint8_t>(services.logicalCount());
 uint8_t physical=services.isAMD()?logical:static_cast<uint8_t>(services.backendCount());
 if(!logical&&!physical)logical=physical=1;
 return {logical,physical,physical<logical};
}
