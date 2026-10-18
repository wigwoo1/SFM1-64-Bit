#include "studio_transition_restore_cache_views_candidate.h"
#include "studio_transition_depth_helper_candidate.h"
#include "studio_transition_alpha_helper_candidate.h"
#include "studio_transition_blend_helper_candidate.h"
#include "studio_transition_separate_blend_helper_candidate.h"
#include <cassert>
#include <cstdio>
struct Services{
 StudioRestoreBlendStateCandidate& parent;unsigned calls=0;
 void render(unsigned key,unsigned){++calls;if(key==27)parent.f104=13;if(key==206)parent.f114=13;}
 void submit(unsigned key,unsigned value){render(key,value);}
 void applyDepthBackend(StudioTransitionState72Candidate&){++calls;}
};
int main(){StudioRestoreBlendStateCandidate parent;StudioTransitionState72Candidate state{};state.flags44=255;state.flags45=1;state.values[0]=3;state.values[1]=1;state.values[4]=state.values[7]=state.values[10]=7;state.values[5]=state.values[8]=state.values[11]=13;state.values[6]=state.values[9]=17;std::uint8_t dirty=1;Services services{parent};
 StudioTransitionRestoreDepthViewCandidate depth(parent,dirty);StudioTransitionRestoreAlphaViewCandidate alpha(parent);StudioTransitionRestoreBlendViewCandidate blend(parent);StudioTransitionRestoreSeparateBlendViewCandidate separate(parent);
 studioTransitionDepthHelperCandidate(state,depth,services);studioTransitionAlphaHelperCandidate(state,alpha,services);studioTransitionBlendHelperCandidate(state,blend,services);studioTransitionSeparateBlendHelperCandidate(state,separate,services);
 assert(parent.f11c==1&&parent.f120==3&&parent.f124==3&&dirty==1);assert(parent.f128==1&&parent.f12c==7&&parent.f130==13);
 assert(parent.ffc==1&&parent.f100==7&&parent.f104==13&&parent.f108==17);assert(parent.f10c==1&&parent.f110==7&&parent.f114==13&&parent.f118==17);
 assert(services.calls==12);const auto before=services.calls;
 studioTransitionDepthHelperCandidate(state,depth,services);studioTransitionAlphaHelperCandidate(state,alpha,services);studioTransitionBlendHelperCandidate(state,blend,services);studioTransitionSeparateBlendHelperCandidate(state,separate,services);assert(services.calls==before);
 std::puts("transition_restore_shared_cache_views=passed");
}
