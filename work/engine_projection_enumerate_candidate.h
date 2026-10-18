#pragma once
#include <cstdint>
#include <cstring>
// Known original payload byte fields only; no invented padding initialization.
// Filter is zero in this caller. Nonzero native pointer layout is not supported.
struct EngineProjectionPayloadCandidate {unsigned char bytes[48];};
template<class Setup,class ReadState,class ReloadArray,class Visit>
inline void engineProjectionEnumerateCandidate(int32_t count,uint16_t handle,
 uint32_t& generation,EngineProjectionPayloadCandidate& payload,
 Setup setup,ReadState readState,ReloadArray reloadArray,Visit visit){
 if(count==0)return;
 ++generation;setup();
 std::memcpy(payload.bytes,&handle,2);
 float projection[3],center[3],radius;readState(projection,center,radius);
 std::memcpy(payload.bytes+4,projection,12);std::memcpy(payload.bytes+28,center,12);
 std::memcpy(payload.bytes+40,&radius,4);uint32_t zero=0;std::memcpy(payload.bytes+44,&zero,4);
 for(int32_t i=0;i<count;++i){uint32_t item=reloadArray()[i];visit(item,payload);}
}
