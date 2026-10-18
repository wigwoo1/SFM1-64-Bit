#pragma once
#include "studio_transition_backend_handlers_candidate.h"
struct StudioTransitionBlendCacheCandidate{
 std::uint8_t enabledFc{};
 std::uint32_t operation100{},source104{},destination108{};
};
template<class Cache,class Services>void studioTransitionBlendHelperCandidate(StudioTransitionState72Candidate& state,
 Cache& cache,Services& services){
 const auto enabled=(state.flags44>>5)&1u;
 if(cache.enabledFc!=enabled){services.render(27,enabled);cache.enabledFc=static_cast<std::uint8_t>((state.flags44>>5)&1u);}
 if(!(state.flags44&32))return;
 const auto operation=state.values[4];
 if(cache.operation100!=operation){services.render(19,operation);cache.operation100=state.values[4];}
 const auto source=state.values[5];
 if(cache.source104!=source){services.render(20,source);cache.source104=state.values[5];}
 const auto destination=state.values[6];
 if(cache.destination108!=destination){services.render(171,destination);cache.destination108=state.values[6];}
}

