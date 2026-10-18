#pragma once
#include <cstdint>
struct StudioBaseShaderDrawStateCandidate {
 uintptr_t parameters=0,shadow=0,api=0,instanceData=0;
 uint32_t snapshot=0,pass=0;
};
// Logical100652c0. Callers serialize shared state and supply full shadow setup.
// Cleanup follows successful callback return; exception behavior is unverified.
template<class Ops>void studioBaseShaderDrawCandidate(StudioBaseShaderDrawStateCandidate& state,uintptr_t shader,uintptr_t parameters,uint32_t snapshot,uintptr_t shadow,uintptr_t api,uint32_t compression,uintptr_t context,uintptr_t instanceData,Ops& ops){
 state.parameters=parameters;state.api=api;state.shadow=shadow;
 state.snapshot=snapshot;state.instanceData=instanceData;state.pass=0;
 if(shadow)ops.shadowSetup(shader,state);
 ops.draw(shader,parameters,shadow,api,compression,context,state);
 state.instanceData=0;state.pass=0;state.snapshot=0;
 state.parameters=0;state.api=0;state.shadow=0;
}
