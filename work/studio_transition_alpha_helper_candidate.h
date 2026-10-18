#pragma once
#include "studio_transition_backend_handlers_candidate.h"
struct StudioTransitionAlphaCacheCandidate{
 std::uint8_t enabled128{};
 std::uint32_t function12c{},reference130{};
};
template<class Cache,class Services>void studioTransitionAlphaHelperCandidate(StudioTransitionState72Candidate& state,
 Cache& cache,Services& services){
 const auto enabled=(state.flags44>>6)&1u;
 if(cache.enabled128!=enabled){
  services.render(15,enabled);
  cache.enabled128=static_cast<std::uint8_t>((state.flags44>>6)&1u);
 }
 if(!(state.flags44&64))return;
 const auto function=state.values[10];
 if(cache.function12c!=function){services.render(25,function);cache.function12c=state.values[10];}
 const auto reference=state.values[11];
 if(cache.reference130!=reference){services.render(24,reference);cache.reference130=state.values[11];}
}

