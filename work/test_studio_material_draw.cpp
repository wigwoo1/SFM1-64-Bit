#include <iostream>
#include "studio_material_draw_candidate.h"
struct Ops {
 StudioMaterialDrawStateCandidate& state;unsigned missing=0,queried=0,draws=0;uint32_t compression,alpha,mode;
 uintptr_t replacement=uintptr_t(0x123456789abcull),key=uintptr_t(0xa5a5abcdef000000ull);
 void missingShader(){++missing;}
 void shaderCallback(uintptr_t shader){if(shader!=state.shader)throw 1;++queried;state.shader=replacement;state.encodedContext=key^replacement;state.shaderState=replacement+64;}
 uintptr_t cookie(){return key;}
 void draw(uintptr_t shader,uintptr_t params,uintptr_t shaderState,uint32_t c,uintptr_t context,uint32_t a,uint32_t m){
  if(shader!=replacement||params!=reinterpret_cast<uintptr_t>(state.parameters)||shaderState!=replacement+64||c!=compression||context!=replacement||a!=uint32_t(uint8_t(alpha)!=0)||m!=mode)throw 2;
  ++draws;
 }
};
int main(){unsigned cases=0;
 for(unsigned flags=0;flags<256;++flags)for(bool shader:{false,true})for(bool params:{false,true})for(bool entry:{false,true})for(uint32_t alpha:{0u,1u,2u,0x100u}){
  StudioMaterialDrawParameterCandidate parameter{flags};auto pointer=entry?&parameter:nullptr;
  StudioMaterialDrawStateCandidate state;state.shader=shader?uintptr_t(0xabcdef123400ull):0;state.parameters=params?&pointer:nullptr;
  Ops ops{state,0,0,0,flags*19,alpha,0xffffffffu-flags};
  studioMaterialDrawCandidate(state,ops.compression,alpha,ops.mode,ops);
  bool allowed=shader&&!(params&&entry&&(flags&4));
  if(ops.missing!=unsigned(!shader)||ops.queried!=unsigned(allowed)||ops.draws!=unsigned(allowed))return 1;
  ++cases;
 }
 std::cout<<"material_draw_cases="<<cases<<" missing_shader_gate=1 flags_gate=1 live_state_reload=1 full_width_context_decode=1 low_byte_alpha=1\n";
}
