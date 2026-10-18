#pragma once
#include "studio_transition_backend_handlers_candidate.h"
struct StudioTransitionSeparateBlendCacheCandidate{
 std::uint8_t enabled10c{};
 std::uint32_t operation110{},source114{},destination118{};
};
template<class Cache,class Services>void studioTransitionSeparateBlendHelperCandidate(StudioTransitionState72Candidate& state,
 Cache& cache,Services& services){
 const auto enabled=(state.flags45>>0)&1u;
 if(cache.enabled10c!=enabled){services.render(206,enabled);cache.enabled10c=static_cast<std::uint8_t>((state.flags45>>0)&1u);}
 if(!(state.flags45&1))return;
 const auto operation=state.values[7];
 if(cache.operation110!=operation){services.render(207,operation);cache.operation110=state.values[7];}
 const auto source=state.values[8];
 if(cache.source114!=source){services.render(208,source);cache.source114=state.values[8];}
 const auto destination=state.values[9];
 if(cache.destination118!=destination){services.render(209,destination);cache.destination118=state.values[9];}
}


