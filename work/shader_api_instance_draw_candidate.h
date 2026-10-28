#pragma once
#include <array>
#include <cstdint>
struct ShaderApiInstanceDrawStateCandidate {
 uintptr_t mesh=0,instances=0,metadata1=0,metadata2=0;
 uint32_t count=0,lightingState=0,configurationMode=0;
 std::array<uint32_t,7>configuration{},savedConfiguration{};
 uint8_t restore=0,lightingValid=0,savedMode=0;
};
// Orchestration of ShaderApi030 slot253,10025B50. Records/resources remain
// caller-owned and valid through synchronous consumption, including count0
// (native still reads the first record). Opaque IDs are pointer-width; this is
// not a native object layout. Preparation services/ownership/threading are
// external. Successful-path cleanup only; no invented exception rollback.
template<class Ops>inline void shaderApiInstanceDrawCandidate(ShaderApiInstanceDrawStateCandidate&state,uintptr_t mesh,uint32_t count,uintptr_t instances,uint32_t compression,uintptr_t metadata1,uintptr_t metadata2,Ops&ops){
 if(ops.disabled())return;
 state.mesh=mesh;state.count=count;state.instances=instances;state.metadata1=metadata1;state.metadata2=metadata2;
 state.restore=0;state.lightingValid=0;state.lightingState=0;
 state.savedMode=uint8_t(state.configurationMode==1);state.savedConfiguration=state.configuration;
 bool mode=false;
 if(state.mesh){
  const auto format=ops.format(state.mesh);
  if(static_cast<int32_t>(ops.meshCount(state.mesh))>0){if(ops.queryMode()==1)mode=true;else if(ops.queryMode()==2)mode=true;}
  const bool flag=(ops.materialFlags(ops.currentMaterial())&uint64_t(0x100))!=0;
  const auto material=ops.currentMaterial();const auto selectedMesh=state.mesh;
  const auto query=ops.meshQuery(selectedMesh);
  const auto alpha=uint8_t(ops.materialAlpha(material,mode,query));
  const bool meshFlag=ops.meshFlag(selectedMesh);
  ops.setup(format,flag,meshFlag,alpha);
 }
 // Uses the original argument pointer, not a potentially callback-replaced
 // published state.instances. The pointed alpha itself is read at this phase.
 const bool changedAlpha=ops.firstAlpha(instances)!=1.f;
 ops.prepareA();ops.prepareB();if(ops.needsPrepareC())ops.prepareC();ops.drainCallbacks();
 ops.drawMaterial(ops.currentMaterial(),compression,changedAlpha,mode);
 if(state.restore)ops.restore(state.savedMode,state.savedConfiguration);
 state.mesh=0;state.count=0;state.instances=0;state.metadata1=0;state.metadata2=0;state.restore=0;state.lightingValid=0;
}
