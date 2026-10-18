#pragma once
#include "studio_restore_blend_candidate.h"
#include "studio_restore_cached_state_candidate.h"
template<class Services>struct StudioRestoreBlendCommitServicesCandidate{
 StudioRestoreBlendStateCandidate& state;Services& services;
 void submit(std::uint32_t key,std::uint32_t value){services.render(key,value);}
 void apply134(std::uint32_t value){studioRestoreCachedStateCandidate(state.f120,23,value,*this);}
 void apply135(std::uint32_t value){studioRestoreCachedStateCandidate(state.f11c,7,value,*this);}
};
template<class Services>void studioRestoreBlendCommitCandidate(StudioRestoreBlendStateCandidate& state,Services& services){
 StudioRestoreBlendCommitServicesCandidate<Services> adapter{state,services};
 studioRestoreCommitCandidate(state.commit,adapter);
}
