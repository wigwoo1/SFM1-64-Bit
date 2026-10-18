#include <iostream>
#include <vector>
#include "studio_shader_runtime_pass_candidate.h"
struct Ops {
 StudioShaderRuntimePassStateCandidate& state;int32_t count;uint8_t before,changed;bool mutate;unsigned renders=0,applied=0;
 int32_t passCount(uint8_t snapshot){if(snapshot!=17)throw 1;return count;}
 void render(uintptr_t word,uint8_t pass,int32_t c){if(word!=uintptr_t(0x123456789abcull)||pass!=before||c!=count)throw 2;++renders;}
 void reset(){if(mutate){state.pass=changed;state.snapshot=29;}}
 uint16_t snapshotHandle(uint8_t snapshot,uint8_t pass){if(snapshot!=(mutate?29:17)||pass!=uint8_t((mutate?changed:before)+1))throw 3;return uint16_t(snapshot*256+pass);}
 void applySnapshot(uint16_t){++applied;}
};
int main(){unsigned cases=0;for(unsigned pass=0;pass<256;++pass)for(int32_t count:{-1,0,1,17,255,256})for(uint32_t draw:{0u,1u,256u,257u})for(bool mutate:{false,true}){
 StudioShaderRuntimePassStateCandidate state{17,uint8_t(pass)};Ops ops{state,count,uint8_t(pass),uint8_t(255-pass),mutate};
 studioShaderRuntimePassCandidate(state,uintptr_t(0x123456789abcull),draw,ops);
 auto next=uint8_t((mutate?255-pass:pass)+1);if(state.pass!=next||ops.renders!=unsigned(uint8_t(draw)!=0)||ops.applied!=unsigned(int32_t(next)<count))return 1;++cases;
}std::cout<<"runtime_pass_cases="<<cases<<" pass_byte_wrap=1 live_state_reload=1 skipped_pass_advances=1 full_width_instance=1\n";}
