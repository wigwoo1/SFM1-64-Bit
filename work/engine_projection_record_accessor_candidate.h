#pragma once
#include <cstdint>
// Logical x64 records; original150h/360h strides are not x64 object layouts.
// Caller guarantees valid arrays/indices; no native invalid-handle guard.
template<class Shadow,class Projection>
inline Projection* engineProjectionRecordCandidate(uint32_t handle,const Shadow* shadows,Projection* projections){
 return projections+shadows[uint16_t(handle)].projectionIndex;
}
