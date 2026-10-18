#pragma once
#include <cstdint>
#include <stdexcept>

inline uint32_t sourceDisplacementFaceLumpCandidate(int32_t hdrType,int32_t hdrBytes){return hdrType!=0&&hdrBytes>0?58u:7u;}
struct SourceDisplacementReadPlanCandidate {
 uint32_t vertexOffset=0,vertexBytes=0,triangleOffset=0,triangleBytes=0,nextVertex=0,nextTriangle=0;
};
// Original loader advances running cursors; it does not use info.vertexStart
// or info.triangleStart for these reads. Caller handles missing associations.
inline SourceDisplacementReadPlanCandidate sourceDisplacementReadPlanCandidate(uint32_t power,uint32_t vertex,uint32_t triangle){
 if(power<1||power>4)throw std::runtime_error("invalid displacement read power");const uint64_t cells=uint64_t(1)<<power,vc=(cells+1)*(cells+1),tc=2*cells*cells;
 if((uint64_t(vertex)+vc)*20>INT32_MAX||(uint64_t(triangle)+tc)*2>INT32_MAX)throw std::runtime_error("displacement read cursor overflow");
 return {vertex*20,uint32_t(vc*20),triangle*2,uint32_t(tc*2),uint32_t(vertex+vc),uint32_t(triangle+tc)};
}
