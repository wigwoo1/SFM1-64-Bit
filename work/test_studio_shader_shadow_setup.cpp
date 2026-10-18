#include <iostream>
#include <vector>
#include <array>
#include "studio_shader_shadow_setup_candidate.h"
struct Ops {
 StudioBaseShaderDrawStateCandidate& state;uintptr_t base;uint32_t flags;bool useShadow;unsigned resets=0,draws=0;
 std::vector<std::array<uint32_t,3>> calls;
 void reset(uintptr_t receiver){if(receiver!=base||state.parameters!=base+64)throw 1;++resets;state.shadow+=16;state.parameters+=16;}
 uint32_t primaryFlags(uintptr_t p){if(p!=base+80)throw 2;return flags;}
 void record(uintptr_t receiver,uint32_t slot,uint32_t a,uint32_t b){if(receiver!=base+16+calls.size()*16)throw 3;calls.push_back({slot,a,b});state.shadow+=16;flags=~flags;}
 void one(uintptr_t receiver,uint32_t slot,uint32_t a){record(receiver,slot,a,0xffffffff);}
 void two(uintptr_t receiver,uint32_t slot,uint32_t a,uint32_t b){record(receiver,slot,a,b);}
 void shadowSetup(uintptr_t shader,StudioBaseShaderDrawStateCandidate& s){if(shader!=base+32||&s!=&state)throw 4;studioShaderShadowSetupCandidate(s,*this);}
 void draw(uintptr_t shader,uintptr_t params,uintptr_t shadow,uintptr_t api,uint32_t compression,uintptr_t context,StudioBaseShaderDrawStateCandidate& s){
  if(shader!=base+32||params!=base+64||shadow!=(useShadow?base:0)||api!=base+96||compression!=0x400||context!=base+128||s.instanceData!=base+160)throw 5;++draws;
 }
};
int main(){unsigned cases=0;
 for(unsigned combo=0;combo<64;++combo)for(unsigned seed=0;seed<128;++seed)for(bool shadow:{false,true}){
  uint32_t flags=seed;unsigned masks[]={0x8000,0x10000,0x2000,0x400,0x10000000,0x20000000};for(unsigned i=0;i<6;++i)if(combo&(1u<<i))flags|=masks[i];
  uintptr_t base=uintptr_t(0x123456789000ull)+seed*512;StudioBaseShaderDrawStateCandidate state;Ops ops{state,base,flags,shadow,0,0,{}};
  studioBaseShaderDrawCandidate(state,base+32,base+64,0xffffffff,shadow?base:0,base+96,0x400,base+128,base+160,ops);
  std::vector<std::array<uint32_t,3>> expected;
  if(shadow){
   if(combo&1){expected.push_back({3,0,0xffffffff});expected.push_back({2,0,0xffffffff});}
   if(combo&2){expected.push_back({4,1,0xffffffff});expected.push_back({2,0,0xffffffff});}
   if(combo&4)expected.push_back({15,0,0xffffffff});if(combo&8)expected.push_back({1,1,0xffffffff});
   if(combo&16)expected.push_back({14,2,1});if(combo&32)expected.push_back({24,1,0xffffffff});
  }
  if(ops.calls!=expected||ops.resets!=unsigned(shadow)||ops.draws!=1||state.shadow||state.parameters||state.api||state.snapshot||state.pass||state.instanceData)return 1;++cases;
 }
 std::cout<<"shadow_setup_cases="<<cases<<" live_receiver_reload=1 cached_flags=1 wrapper_integration=1 ordered_arguments=1 full_width_pointers=1\n";
}
