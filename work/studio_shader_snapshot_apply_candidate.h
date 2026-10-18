#pragma once
#include "studio_shader_pass_draw_candidate.h"
// ShaderAPI93/10011000: this publishes a handle and notifies mesh27.
// Actual render-state transitions are deferred to the draw path.
template<class Ops>void studioShaderSnapshotApplyCandidate(StudioShaderPassDrawStateCandidate& state,uint32_t handle,Ops& ops){
 if(uint8_t(ops.suppressed()))return;
 const uintptr_t mesh=state.mesh;
 state.snapshot=static_cast<uint16_t>(handle);
 if(mesh)ops.meshSnapshot(mesh);
}
