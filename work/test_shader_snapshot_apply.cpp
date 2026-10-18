#include "studio_shader_snapshot_apply_candidate.h"
#include <cstdio>
#include <initializer_list>
struct Ops {
 StudioShaderPassDrawStateCandidate& state;uint32_t gate;uintptr_t selected;unsigned calls=0;
 uint32_t suppressed(){state.mesh=selected;return gate;}
 void meshSnapshot(uintptr_t mesh){if(mesh!=selected||state.snapshot!=uint16_t(expected))throw 1;++calls;state.mesh=0;state.snapshot=123;}
 uint32_t expected;
};
struct CombinedOps {
 StudioShaderPassDrawStateCandidate& state;uint16_t expected;uintptr_t destination;unsigned gates=0,notifies=0,draws=0;
 uint32_t suppressed(){++gates;return 0x100;}
 void meshSnapshot(uintptr_t mesh){if(mesh!=state.mesh)throw 11;++notifies;}
 void applyTransitions(uint16_t value){if(value!=expected)throw 12;state.snapshot=uint16_t(value+1);}
 void prepare(uint16_t value){if(value!=uint16_t(expected+1))throw 13;state.mesh=destination;}
 void meshDraw(uintptr_t mesh,uintptr_t instance){if(mesh!=destination||instance!=0x1234)throw 14;++draws;state.snapshot=42;}
 void managerDraw(uintptr_t instance){if(destination||instance!=0x1234)throw 15;++draws;state.snapshot=43;}
};
int main(){
 unsigned cases=0;
 const uintptr_t selected=sizeof(uintptr_t)==8?static_cast<uintptr_t>(0x1234567887654321ull):0x87654321u;
 for(uint32_t value=0;value<65536;++value)for(uint32_t gate:{0u,0x100u,1u,0x101u})for(bool mesh:{false,true}){
  StudioShaderPassDrawStateCandidate state{77,1};
  const uint32_t handle=value|0xabcd0000u;
  Ops ops{state,gate,mesh?selected:0,0,handle};
  studioShaderSnapshotApplyCandidate(state,handle,ops);
  if(uint8_t(gate)){if(state.snapshot!=77||ops.calls)return 2;}
  else if(mesh){if(state.snapshot!=123||state.mesh!=0||ops.calls!=1)return 3;}
  else if(state.snapshot!=uint16_t(value)||ops.calls)return 4;
  ++cases;
 }
 std::printf("snapshot_apply_cases=%u lowbyte_suppression=1 handle_truncation=1 callback_mutation=1 full_mesh_pointer=1\n",cases);
 unsigned combined=0;
 for(uint32_t value=0;value<65536;++value)for(bool mesh:{false,true}){
  StudioShaderPassDrawStateCandidate state{77,mesh?selected:0};
  CombinedOps ops{state,uint16_t(value),mesh?0:selected};
  studioShaderSnapshotApplyCandidate(state,value|0xabcd0000u,ops);
  studioShaderPassDrawCandidate(state,0x1234,ops);
  if(state.snapshot!=0xffff||ops.gates!=2||ops.notifies!=(mesh?1u:0u)||ops.draws!=1)return 16;
  ++combined;
 }
 std::printf("combined_snapshot_draw_cases=%u live_transition_prepare_mesh=1 final_handle_reset=1\n",combined);
 return 0;
}
