#include <iostream>
#include <vector>
#include <limits>
#include "studio_vertex_lit_passes_candidate.h"
#include "studio_shader_cloak_gate_candidate.h"
struct Ops {
 unsigned flags,changedFlags;float factor,strength;bool mutate;std::vector<unsigned> passes;unsigned opaqueCalls=0;
 bool cloakEnabled(){return (flags&1)!=0;}bool emissiveEnabled(){return (flags&2)!=0;}bool fleshEnabled(){return (flags&4)!=0;}
 bool cloakOpaque(){++opaqueCalls;return studioShaderCloakOpaqueCandidate(factor);}
 float cloakFactor(){return factor;}float emissiveStrength(){return strength;}
 void afterFirst(){if(passes.size()==1&&mutate)flags=changedFlags;}
 void skip(uint32_t value){if(value)throw 1;passes.push_back(0);afterFirst();}
 void mainLighting(){passes.push_back(1);afterFirst();}void cloak(){passes.push_back(2);}void emissive(){passes.push_back(3);}void flesh(){passes.push_back(4);}
};
int main(){unsigned cases=0;float nan=std::numeric_limits<float>::quiet_NaN();
 for(unsigned flags=0;flags<8;++flags)for(unsigned changed=0;changed<8;++changed)for(bool mutate:{false,true})for(bool shadow:{false,true})for(float factor:{-1.f,0.f,0.25f,0.5f,1.f,2.f,nan})for(float strength:{-1.f,0.f,1.f,nan}){
  Ops ops{flags,changed,factor,strength,mutate,{},0};studioVertexLitPassesCandidate(shadow?uintptr_t(0x123456789abcull):0,ops);
  bool skipMain=(flags&1)&&!shadow&&(factor==0.5f||factor==1.f||factor==2.f);
  std::vector<unsigned> expected{skipMain?0u:1u};auto live=mutate?changed:flags;
  if(live&1)expected.push_back(shadow||(factor>0&&factor<1)?2u:0u);
  if(live&2)expected.push_back(shadow||strength>0?3u:0u);
  if(live&4)expected.push_back(4);
  if(ops.passes!=expected||ops.opaqueCalls!=unsigned((flags&1)&&!shadow))return 1;++cases;
 }
 std::cout<<"vertex_lit_pass_cases="<<cases<<" live_enable_reload=1 cloak_gate_integration=1 ordered_effects=1 nan_gates=1\n";
}
