#pragma once
#include <cstdint>
struct StudioMaterialDrawParameterCandidate {uint32_t flags=0;};
struct StudioMaterialDrawStateCandidate {
 uintptr_t shader=0;StudioMaterialDrawParameterCandidate** parameters=nullptr;
 uintptr_t shaderState=0,encodedContext=0;
};
// Logical equivalent of10004ae0. Encoding cookie widened with encoded pointer.
// Native callbacks may change state: reread fields after shader slot0 callback.
template<class Ops>void studioMaterialDrawCandidate(StudioMaterialDrawStateCandidate& state,uint32_t compression,uint32_t alpha,uint32_t mode,Ops& ops){
 if(!state.shader){ops.missingShader();return;}
 auto parameter=state.parameters?*state.parameters:nullptr;
 if(parameter&&(parameter->flags&4u))return;
 ops.shaderCallback(state.shader);
 ops.draw(state.shader,reinterpret_cast<uintptr_t>(state.parameters),state.shaderState,compression,state.encodedContext^ops.cookie(),uint32_t(uint8_t(alpha)!=0),mode);
}
