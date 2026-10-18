#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
struct SourceBoxRecordPlaneCandidate {std::array<float,3>normal{};float distance=0;uint8_t type=0;};
struct SourceBoxRecordSideCandidate {uint32_t plane=0;int32_t texture=-1;uint8_t flags=0;};
// Reconstructs the checked six-side constructor, not the loader classifier.
// Padding and unwritten fields retain the caller's bytes, as in the native code.
// Rejects unsafe inputs before committing any output; this is defensive policy.
inline void sourceOptimizedBoxRecordCandidate(const std::vector<SourceBoxRecordPlaneCandidate>&planes,
 const std::array<SourceBoxRecordSideCandidate,6>&sides,const std::vector<uint32_t>&surfaceIds,
 std::array<uint8_t,48>&output){
 auto result=output;uint16_t flags=0;
 for(const auto&side:sides){
  if(side.plane>=planes.size())throw std::runtime_error("box plane index out of range");
  const auto&p=planes[side.plane];if(p.type>2||!std::isfinite(p.distance))throw std::runtime_error("invalid box plane");
  for(float n:p.normal)if(!std::isfinite(n))throw std::runtime_error("nonfinite box normal");
  uint16_t surface=65535;
  if(side.texture>=0){if(size_t(side.texture)>=surfaceIds.size())throw std::runtime_error("box texture index out of range");surface=uint16_t(surfaceIds[size_t(side.texture)]);}
  unsigned face=p.type;uint32_t distance;std::memcpy(&distance,&p.distance,4);
  if(p.normal[p.type]==1){face+=3;std::memcpy(result.data()+16+4*p.type,&distance,4);std::memcpy(result.data()+32+2*face,&surface,2);}
  else if(p.normal[p.type]==-1){distance^=0x80000000;std::memcpy(result.data()+4*p.type,&distance,4);std::memcpy(result.data()+32+2*face,&surface,2);}
  flags=uint16_t(flags|uint16_t(uint32_t(side.flags)<<face));
 }
 std::memcpy(result.data()+44,&flags,2);uint16_t zero=0;std::memcpy(result.data()+46,&zero,2);output=result;
}
