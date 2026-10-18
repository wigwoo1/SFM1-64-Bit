#pragma once
#include <cstdint>
struct StudioShaderRuntimePassStateCandidate {uint8_t snapshot=0,pass=0;};
// Logical ShaderSystem002 slot4 (secondary-this offsets86/87).
// Ops owns snapshot storage and live ShaderAPI callbacks.
template<class Ops>void studioShaderRuntimePassCandidate(StudioShaderRuntimePassStateCandidate& state,uintptr_t instanceWord,uint32_t draw,Ops& ops){
 int32_t count=ops.passCount(state.snapshot);
 if(uint8_t(draw))ops.render(instanceWord,state.pass,count);
 ops.reset();state.pass=uint8_t(state.pass+1);
 if(int32_t(state.pass)<count)ops.applySnapshot(ops.snapshotHandle(state.snapshot,state.pass));
}
