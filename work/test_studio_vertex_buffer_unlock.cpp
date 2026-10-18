#include <iostream>
#include "studio_vertex_buffer_unlock_candidate.h"
int main(){unsigned cases=0;
for(unsigned flags=0;flags<256;++flags)for(unsigned mode=0;mode<8;++mode){
 uintptr_t resource=(mode&1)?uintptr_t(0x123456789000ull):0;bool queued=(mode&2)!=0,failed=(mode&4)!=0;uint32_t count=flags*2654435761u;
 StudioVertexBufferUnlockStateCandidate state{resource,17,12,uint8_t(flags),{3,7,11}};unsigned queries=0,emits=0,unlocks=0,diagnostics=0;
 studioVertexBufferUnlockCandidate(state,count,[&]{++queries;return queued;},
 [&](auto command){if(command.resource!=resource||command.opcode!=((flags&1)?29u:18u)||state.usedBytes!=17)throw 1;if(flags&1){if(command.metadata!=state.metadata||command.bytes!=count*12)throw 1;}++emits;},
 [&](uintptr_t selected){if(selected!=resource||state.usedBytes!=17)throw 1;++unlocks;return failed?int32_t(-1):int32_t(0);},
 [&](unsigned line){if(!unlocks||line!=((flags&1)?0x3f4u:0x3dau))throw 1;++diagnostics;});
 bool active=(flags&2)&&resource;
 if(queries!=unsigned(active)||emits!=unsigned(active&&queued)||unlocks!=unsigned(active&&!queued)||diagnostics!=unsigned(active&&!queued&&failed))return 1;
 if(state.usedBytes!=(active?uint32_t(24+count*12):17)||state.flags!=uint8_t(active?(flags&0xfd):flags))return 2;++cases;
}
unsigned alignmentCases=0;
for(uint32_t stride=1;stride<=128;++stride)for(uint32_t cursor=0;cursor<=256;++cursor){
 StudioVertexBufferUnlockStateCandidate state{uintptr_t(0x123456789000ull),cursor,stride,2,{}};
 studioVertexBufferUnlockCandidate(state,3,[]{return false;},[](auto){throw 2;},[](uintptr_t){return int32_t(0);},[](unsigned){throw 2;});
 uint32_t aligned=cursor;while(aligned%stride)++aligned;
 if(state.usedBytes!=aligned+stride*3||state.flags)return 3;++alignmentCases;
}
std::cout<<"alignment_cases="<<alignmentCases<<" ";
std::cout<<"vertex_buffer_unlock_cases="<<cases<<" typed_commands=1 negative_hresult_advances=1 count_wrap=1\n";
}
