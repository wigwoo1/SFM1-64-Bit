#pragma once
#include <cstdint>
struct StudioShaderPassDrawStateCandidate {uint16_t snapshot=0xffff;uintptr_t mesh=0;};
// ShaderAPI94/10025ea0. The two extra native arguments are unused here.
template<class Ops>void studioShaderPassDrawCandidate(StudioShaderPassDrawStateCandidate& state,uintptr_t instanceWord,Ops& ops){
 if(uint8_t(ops.suppressed()))return;
 ops.applyTransitions(state.snapshot);ops.prepare(state.snapshot);
 if(state.mesh)ops.meshDraw(state.mesh,instanceWord);else ops.managerDraw(instanceWord);
 state.snapshot=0xffff;
}
