#include <iostream>
#include <vector>
#include "studio_shader_pass_draw_candidate.h"
struct Ops {
 StudioShaderPassDrawStateCandidate& state;uint32_t gate;uint16_t initial;bool mesh;std::vector<unsigned> calls;
 uint32_t suppressed(){calls.push_back(0);return gate;}
 void applyTransitions(uint16_t value){if(value!=initial)throw 1;calls.push_back(1);state.snapshot=uint16_t(initial+977);}
 void prepare(uint16_t value){if(value!=uint16_t(initial+977))throw 2;calls.push_back(2);state.mesh=mesh?uintptr_t(0x123456789abcull):0;}
 void meshDraw(uintptr_t receiver,uintptr_t word){if(receiver!=uintptr_t(0x123456789abcull)||word!=uintptr_t(0xabcdef123456ull))throw 3;calls.push_back(3);state.snapshot=17;}
 void managerDraw(uintptr_t word){if(word!=uintptr_t(0xabcdef123456ull))throw 4;calls.push_back(4);state.snapshot=19;}
};
int main(){unsigned cases=0;for(unsigned i=0;i<65536;++i)for(uint32_t gate:{0u,1u,256u,257u})for(bool mesh:{false,true}){
 StudioShaderPassDrawStateCandidate state{uint16_t(i),mesh?0:uintptr_t(1)};Ops ops{state,gate,uint16_t(i),mesh,{}};
 studioShaderPassDrawCandidate(state,uintptr_t(0xabcdef123456ull),ops);
 bool blocked=uint8_t(gate)!=0;auto expected=blocked?std::vector<unsigned>{0}:std::vector<unsigned>{0,1,2,mesh?3u:4u};
 if(ops.calls!=expected||state.snapshot!=(blocked?uint16_t(i):0xffff))return 1;++cases;
}std::cout<<"shader_pass_draw_cases="<<cases<<" live_snapshot_reload=1 live_mesh_reload=1 suppressed_preserves_state=1 full_width_pointers=1\n";}
