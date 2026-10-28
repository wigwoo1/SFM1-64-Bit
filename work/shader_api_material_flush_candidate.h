#pragma once
#include <cstdint>
// Live single-owner fields at native interface+DE4/DE8. Meaning is not inferred.
struct ShaderApiMaterialFlushStateCandidate {uint32_t first=0,second=0;};
// ShaderApi030 slot84,10019A10. The optional service predicate is consulted
// BEFORE saving fields, so its mutations are included in the saved values.
// No exception rollback, synchronization or material snapshot is introduced.
template<class Ops>inline void shaderApiMaterialFlushCandidate(ShaderApiMaterialFlushStateCandidate&state,Ops&ops){
 const auto gate=ops.gate();if(gate&&!ops.allowFlush(gate))return;
 const auto first=state.first,second=state.second;
 ops.flushMeshManager();
 state.second=second;state.first=first;
}
