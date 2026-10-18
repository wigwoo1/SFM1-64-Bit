#pragma once
#include "material_selfillum_candidate.h"
#include <cstddef>
struct MaterialSelfIllumDrawPushCandidate {
 std::array<float,24> existingVertexConstants{};
 float alpha=1,alphaCutoff=0,enabled=0,separateMask=0;
 std::array<float,3>linearTint{1,1,1};
};
static_assert(offsetof(MaterialSelfIllumDrawPushCandidate,alpha)==96);
static_assert(offsetof(MaterialSelfIllumDrawPushCandidate,alphaCutoff)==100);
static_assert(offsetof(MaterialSelfIllumDrawPushCandidate,enabled)==104);
static_assert(offsetof(MaterialSelfIllumDrawPushCandidate,separateMask)==108);
static_assert(offsetof(MaterialSelfIllumDrawPushCandidate,linearTint)==112);
static_assert(sizeof(MaterialSelfIllumDrawPushCandidate)==124);
// Callers supply separately verified linear tint; authored VMT tint is not
// silently interpreted as linear. This does not allocate/bind mask resources.
inline MaterialSelfIllumDrawPushCandidate materialSelfIllumDrawPushCandidate(
 const std::array<float,24>&vertexConstants,float alpha,float cutoff,
 const MaterialSelfIllumCandidate&settings,bool maskAvailable,
 const std::array<float,3>&linearTint){
 if(!std::isfinite(alpha)||alpha<0||alpha>1||!std::isfinite(cutoff)||cutoff<0||cutoff>1)throw std::runtime_error("invalid selfillum draw alpha");
 for(float value:vertexConstants)if(!std::isfinite(value))throw std::runtime_error("nonfinite selfillum vertex constants");
 for(float value:linearTint)if(!std::isfinite(value)||value<0)throw std::runtime_error("invalid selfillum linear tint");
 bool explicitMask=settings.enabled&&!settings.mask.empty();
 if(explicitMask&&!maskAvailable)throw std::runtime_error("selfillum draw mask unavailable");
 return {vertexConstants,alpha,cutoff,settings.enabled?1.f:0.f,explicitMask?1.f:0.f,linearTint};
}
