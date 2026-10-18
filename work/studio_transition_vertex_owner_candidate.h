#pragma once
#include "studio_transition_vertex_stage_candidate.h"
#include "studio_vertex_constant_owner_candidate.h"
// View of live normalized owner fields. Callbacks may change these fields and
// cache pointer slots. Caller retains their storage throughout the transition.
struct StudioTransitionVertexOwnerCandidate {
 uint32_t& state;
 float& lower;
 float& upper;
 float& density;
 StudioVertexConstantOwnerCandidate constants;
};
template<class Services>struct StudioTransitionVertexOwnerAdapterCandidate {
 StudioTransitionVertexOwnerCandidate& owner;Services& services;
 void entryService(){services.entryService();}
 uint32_t currentState(){return owner.state;}
 int32_t capability(){return services.capability();}
 float lower(){return owner.lower;}
 float upper(){return owner.upper;}
 float density(){return owner.density;}
 std::array<uint32_t,4> vertexColor(){return services.vertexColor();}
 uint32_t* primaryCache(){return owner.constants.primary;}
 uint32_t* secondaryCache(){return owner.constants.secondary;}
 void submit(uint32_t index,uint32_t count,const uint32_t* words){services.submit(index,count,words);}
};
template<class Services>uint32_t studioTransitionVertexOwnerCandidate(
 StudioTransitionVertexOwnerCandidate& owner,uint32_t input,uint32_t flag,Services& services){
 StudioTransitionVertexOwnerAdapterCandidate<Services> adapter{owner,services};
 return studioTransitionVertexStageCandidate(input,flag,adapter);
}
