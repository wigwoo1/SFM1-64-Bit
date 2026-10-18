#pragma once
#include <cstdint>
// Logical x64 resource field corresponding to original record+100h.
// Resource ownership/service type remains unresolved; retain full pointer width.
struct ModelRenderRecordResourceCandidate {uintptr_t resource=0;};
template<class Record,class Release>
inline void modelRenderRecordResourceCleanupCandidate(uint32_t argument,Record record,Release release){
 const uint16_t index=uint16_t(argument);if(index==0xffff)return;
 const uintptr_t resource=record(index).resource;if(!resource)return;
 release(resource,0,0,1);
 record(index).resource=0;
}
