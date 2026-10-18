#pragma once
#include "studio_shader_constant_pair_candidate.h"
// Borrow the owner's pointer slots, so service callbacks replacing storage are
// observed by the postsubmission copies. Storage extents/lifetimes are external.
struct StudioVertexConstantOwnerCandidate {
 uint32_t*& primary;
 uint32_t*& secondary;
};
template<class Services>struct StudioVertexConstantOwnerAdapterCandidate {
 StudioVertexConstantOwnerCandidate& owner;Services& services;
 uint32_t* primaryCache(){return owner.primary;}
 uint32_t* secondaryCache(){return owner.secondary;}
 void submit(uint32_t index,uint32_t count,const uint32_t* words){services.submit(index,count,words);}
};
template<class Services>void studioVertexConstantOwnerPairCandidate(
 StudioVertexConstantOwnerCandidate& owner,const std::array<uint32_t,4>& parameters,
 const std::array<uint32_t,4>& color,Services& services){
 StudioVertexConstantOwnerAdapterCandidate<Services> adapter{owner,services};
 studioShaderConstantPairCandidate(parameters,color,adapter);
}
