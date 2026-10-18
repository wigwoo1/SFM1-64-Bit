#pragma once
#include "studio_transition_backend_handlers_candidate.h"
#include "studio_restore_cached_state_candidate.h"
struct StudioTransitionDepthCacheCandidate{
 std::uint32_t state7{},state23{},mode124{};
 std::uint8_t dirty178{};
};
template<class Cache,class Services>void studioTransitionDepthHelperCandidate(StudioTransitionState72Candidate& state,
 Cache& cache,Services& services){
 studioRestoreCachedStateCandidate(cache.state7,7,state.values[1],services);
 if(state.values[1])studioRestoreCachedStateCandidate(cache.state23,23,state.values[0],services);
 const auto flags=state.flags44;
 const auto mode=(flags>>2)&3u;
 if(cache.mode124!=mode||((flags&12u)==8u&&cache.dirty178)){
  services.applyDepthBackend(state);
  cache.mode124=(state.flags44>>2)&3u;
 }
}

