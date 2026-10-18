#pragma once
#include <cstdint>
struct StudioShaderCullTransitionStateCandidate {
 uint32_t configured=1,applied=1;uint8_t enabled=0;bool flip=false;
};
// ShaderAPI263/1000f580. applied retains unflipped requested mode.
template<class Setter>void studioShaderCullTransitionCandidate(StudioShaderCullTransitionStateCandidate& state,uint32_t input,Setter&& setter){
 uint32_t requested=state.configured;
 state.enabled=static_cast<uint8_t>(input);
 if(!state.enabled)requested=1;
 if(requested==state.applied)return;
 uint32_t submitted=requested;
 if(state.flip&&requested!=1)submitted=requested==2?3:2;
 setter(22u,submitted,0u);
 state.applied=requested;
}
