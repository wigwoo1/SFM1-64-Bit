#pragma once
#include "studio_transition_backend_handlers_candidate.h"
#include "studio_transition_command_handlers_candidate.h"
#include "studio_transition_depth_helper_candidate.h"
#include "studio_transition_alpha_helper_candidate.h"
#include "studio_transition_blend_helper_candidate.h"
#include "studio_transition_separate_blend_helper_candidate.h"
#include "studio_transition_state194_helper_candidate.h"
#include <stdexcept>
template<class Services>struct StudioTransitionDepthDispatchServicesCandidate{
 Services& services;
 void submit(std::uint32_t key,std::uint32_t value){services.render(key,value);}
 void applyDepthBackend(StudioTransitionState72Candidate& state){services.applyDepthBackend(state);}
};
template<class Services>void studioTransitionBackendDispatchCandidate(StudioTransitionState72Candidate* state,
 const std::uint8_t* operations,std::int32_t count,Services& services){
 if(count<=0)return;
 if(!state||!operations)throw std::invalid_argument("transition dispatch storage");
 for(std::int32_t i=0;i<count;++i){const auto opcode=operations[i];
  if(opcode&0x80){const unsigned handler=(opcode&0x7f)>>4;if(handler>=2)throw std::out_of_range("packed transition handler");if(handler==0)studioTransitionSampler11Candidate(*state,opcode&15,services);else studioTransitionSampler8Candidate(*state,opcode&15,services);continue;}
  const unsigned handler=opcode&127;
  switch(handler){
   case 0:{auto& cache=services.depthCache();StudioTransitionDepthDispatchServicesCandidate<Services> adapter{services};studioTransitionDepthHelperCandidate(*state,cache,adapter);break;}
   case 1:studioTransitionRender14Candidate(*state,services);break;
   case 2:studioTransitionRender168Candidate(*state,services);break;
   case 3:{auto& cache=services.alphaCache();studioTransitionAlphaHelperCandidate(*state,cache,services);break;}
   case 4:studioTransitionRender8Candidate(*state,services);break;
   case 5:{const auto force=services.force144();studioTransitionState194HelperCandidate(*state,force,services);break;}
   case 6:{auto& cache=services.blendCache();studioTransitionBlendHelperCandidate(*state,cache,services);break;}
   case 7:{auto& cache=services.separateBlendCache();studioTransitionSeparateBlendHelperCandidate(*state,cache,services);break;}
   case 8:studioTransitionFlag44Candidate(*state,services);break;
   case 9:case 11:studioTransitionCombinedFlagsCandidate(*state,services);break;
   case 10:studioTransitionStagesCandidate(state,services);break;
   case 12:studioTransitionFlag45Candidate(*state,services);break;
   default:throw std::out_of_range("ordinary transition handler");
  }
 }
}
