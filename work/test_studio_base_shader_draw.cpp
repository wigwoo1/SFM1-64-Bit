#include <iostream>
#include "studio_base_shader_draw_candidate.h"
struct Ops {
 uintptr_t shader,parameters,shadow,api,context,data;uint32_t snapshot,compression;unsigned setupCalls=0,drawCalls=0;
 void check(const StudioBaseShaderDrawStateCandidate& s){if(s.parameters!=parameters||s.shadow!=shadow||s.api!=api||s.instanceData!=data||s.snapshot!=snapshot||s.pass)throw 1;}
 void shadowSetup(uintptr_t receiver,StudioBaseShaderDrawStateCandidate& state){if(receiver!=shader||!shadow)throw 2;check(state);++setupCalls;state.pass=17;}
 void draw(uintptr_t receiver,uintptr_t p,uintptr_t sh,uintptr_t a,uint32_t c,uintptr_t ctx,StudioBaseShaderDrawStateCandidate& state){
  if(receiver!=shader||p!=parameters||sh!=shadow||a!=api||c!=compression||ctx!=context||state.pass!=(shadow?17u:0u))throw 3;
  auto pass=state.pass;state.pass=0;check(state);state.pass=pass;
  ++drawCalls;state.parameters=1;state.shadow=2;state.api=3;state.instanceData=4;state.snapshot=5;state.pass=6;
 }
};
int main(){unsigned cases=0;
 for(unsigned i=0;i<65536;++i)for(bool shadow:{false,true}){
  uintptr_t base=uintptr_t(0x123456789000ull)+i*128;
  Ops ops{base,base+8,shadow?base+16:0,base+24,base+32,base+40,i==65535?0xffffffffu:i*977,i*17};
  StudioBaseShaderDrawStateCandidate state{9,8,7,6,5,4};
  studioBaseShaderDrawCandidate(state,ops.shader,ops.parameters,ops.snapshot,ops.shadow,ops.api,ops.compression,ops.context,ops.data,ops);
  if(ops.setupCalls!=unsigned(shadow)||ops.drawCalls!=1||state.parameters||state.shadow||state.api||state.instanceData||state.snapshot||state.pass)return 1;
  ++cases;
 }
 std::cout<<"base_shader_draw_cases="<<cases<<" full_width_pointers=1 published_before_setup=1 supplied_args_retained=1 cleanup_after_draw=1\n";
}
