#pragma once
#include "studio_transition_backend_handlers_candidate.h"
#include <stdexcept>
template<class Services>void studioTransitionRender8Candidate(const StudioTransitionState72Candidate& state,Services& services){
 const auto value=state.values[3];services.render(8,value);
}
template<class Services>void studioTransitionRender14Candidate(const StudioTransitionState72Candidate& state,Services& services){
 const auto value=(state.flags44>>1)&1u;services.render(14,value);
}
template<class Services>void studioTransitionRender168Candidate(const StudioTransitionState72Candidate& state,Services& services){
 const auto value=state.values[2];services.render(168,value);
 // Original rereads the state and global cache receiver after submission.
 services.publishCache140(state.values[2]);
}
inline std::uint8_t studioTransitionStageByteCandidate(const StudioTransitionState72Candidate& state,unsigned stage){
 if(stage>=16)throw std::out_of_range("transition stage byte");
 return static_cast<std::uint8_t>(state.values[12+stage/4]>>((stage%4)*8));
}
template<class Services>void studioTransitionSampler11Candidate(const StudioTransitionState72Candidate& state,unsigned stage,Services& services){
 const auto value=(studioTransitionStageByteCandidate(state,stage)>>1)&1u;
 services.sampler(stage,11,value);
}
template<class Services>void studioTransitionSampler8Candidate(const StudioTransitionState72Candidate& state,unsigned stage,Services& services){
 // Capability is queried before the original touches state storage.
 if(!static_cast<std::uint8_t>(services.specialCapability()))return;
 const auto value=(studioTransitionStageByteCandidate(state,stage)&4u)?0x34544547u:0x31544547u;
 services.sampler(stage,8,value);
}
