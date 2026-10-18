#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

struct SourceDisplacementCoreInputCandidate {
 uint32_t power=0,surfaceFlags=0,flags=0;
 std::vector<std::array<float,56>>vertices; // Value fields only; 224 bytes.
 std::vector<uint16_t>zeroIndices,triangleTags;
};

// Collision-relevant value initialization from 101428E0 -> 10142210.
// Native's elevation argument is unused by these routines. Geometry creation
// and node internals are separate; this does not implement the full loader.
inline SourceDisplacementCoreInputCandidate sourceDisplacementCoreInputCandidate(
 uint32_t power,uint32_t encodedFlags,uint32_t flags,uint32_t priorSurfaceFlags,
 const std::vector<std::array<float,5>>&raw,const std::vector<uint16_t>&tags,
 const std::vector<std::array<float,20>>&extra={}){
 if(power<1||power>4)throw std::runtime_error("invalid CoreDisp input power");const size_t cells=size_t(1)<<power,vc=(cells+1)*(cells+1),tc=2*cells*cells;
 if(raw.size()!=vc||tags.size()!=tc||(!extra.empty()&&extra.size()!=vc))throw std::runtime_error("CoreDisp input coverage mismatch");
 for(const auto&v:raw)for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite CoreDisp input");for(const auto&v:extra)for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite CoreDisp extra input");
 SourceDisplacementCoreInputCandidate result;result.power=power;result.surfaceFlags=(encodedFlags&0x80000000u)?encodedFlags&0x7fffffffu:priorSurfaceFlags;result.flags=flags;
 const bool useExtra=(flags&0x40000000u)&&!extra.empty();if(!useExtra)result.flags&=~0x40000000u;
 result.vertices.resize(vc);result.zeroIndices.resize(vc*6);result.triangleTags=tags;
 for(size_t i=0;i<vc;++i){auto&v=result.vertices[i];for(size_t j=44;j<56;++j)v[j]=1.f;
  for(size_t j=0;j<4;++j)v[j]=raw[i][j];v[35]=raw[i][4];
  if(useExtra)for(size_t j=0;j<20;++j)v[36+j]=extra[i][j];
 }return result;
}
