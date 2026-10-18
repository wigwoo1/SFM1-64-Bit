#pragma once
#include <cstdint>
// Logical slot262 policy. Ops owns unresolved1001d1f0 binding implementation.
//1001d1f0 dereferences this value as a texture record pointer.
struct StudioTextureEnableStageCandidate {uintptr_t handle=0;uint8_t enabled=0;};
template<class Ops>void studioTextureEnableTransitionCandidate(uint8_t stateByte,uint32_t stage,StudioTextureEnableStageCandidate& live,Ops& ops){
 const uint8_t enabled=stateByte&1;
 if(enabled==live.enabled)return;
 live.enabled=enabled;
 if(enabled){const uintptr_t handle=live.handle;ops.bind(stage,handle,1);return;}
 if(!live.handle)return;
 if(ops.queueEnabled())ops.queueNullTexture(stage);else ops.directNullTexture(stage);
}
