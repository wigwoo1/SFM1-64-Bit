#pragma once
#include "studio_base_shader_draw_candidate.h"
// Logical10064ab0: flags captured after reset and retained across later calls.
// Receiver is reloaded from shared state for every native virtual call.
template<class Ops>void studioShaderShadowSetupCandidate(StudioBaseShaderDrawStateCandidate& state,Ops& ops){
 ops.reset(state.shadow);
 auto flags=ops.primaryFlags(state.parameters);
 if(flags&0x8000u){ops.one(state.shadow,3,0);ops.one(state.shadow,2,0);}
 if(flags&0x10000u){ops.one(state.shadow,4,1);ops.one(state.shadow,2,0);}
 if(flags&0x2000u)ops.one(state.shadow,15,0);
 if(flags&0x400u)ops.one(state.shadow,1,1);
 if(flags&0x10000000u)ops.two(state.shadow,14,2,1);
 if(flags&0x20000000u)ops.one(state.shadow,24,1);
}
