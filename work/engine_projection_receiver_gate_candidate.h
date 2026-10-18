#pragma once
#include "engine_projection_box_sphere_candidate.h"
#include "engine_projection_box_planes_candidate.h"
// Logical object record fields only, not original binary ABI/layout.
struct EngineProjectionObjectRecordCandidate {uint32_t flags;uint8_t byte16;};
template<class Record,class Bounds,class Planes,class ShadowFlags,class Associate>
inline bool engineProjectionReceiverGateCandidate(uint32_t branch,uint16_t handle,
 const float* center,float radius,Record record,Bounds bounds,Planes planes,
 ShadowFlags shadowFlags,Associate associate){
 uint8_t special=uint8_t(branch);
 if(!special&&(record()->flags&0x2000))return false;
 float minimum[3],maximum[3];bounds(minimum,maximum);
 if(special){if(engineProjectionBoxOutsidePlanesCandidate(planes(handle),minimum,maximum))return false;}
 else if(!engineProjectionBoxSphereCandidate(minimum,maximum,center,radius))return false;
 auto* selected=record();
 if(!(selected->byte16&1)&&!special)return false;
 if(!(shadowFlags(handle)&6)&&(selected->flags&0x3020))return false;
 associate(selected,handle);return true;
}
