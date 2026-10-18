#pragma once
#include "studio_transition_backend_handlers_candidate.h"
template<class Services>void studioTransitionState194HelperCandidate(StudioTransitionState72Candidate& state,
 std::uint8_t force144,Services& services){
 if(force144){services.render(194,0);services.applyBackend271(0);return;}
 const auto capability=services.capability41();
 const bool overrideValue=capability!=0&&services.capability17()>=92;
 const auto value=overrideValue?0u:static_cast<unsigned>(state.flags44>>7);
 services.render(194,value);
 services.applyBackend271(static_cast<unsigned>(state.flags44>>7));
 if(static_cast<std::uint8_t>(services.capability25())){
  const auto flags=state.flags44;
  services.applyCombined(state.value40,flags&1u,flags>>7,(state.flags45>>2)&1u);
 }
}
