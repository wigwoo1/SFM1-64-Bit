#include <iostream>
#include "studio_index_buffer_unlock_candidate.h"
int main(){unsigned cases=0;
for(unsigned flags=0;flags<256;++flags)for(unsigned mode=0;mode<8;++mode){
 uintptr_t resource=(mode&1)?uintptr_t(0x123456789000ull):0;bool queued=(mode&2)!=0,failed=(mode&4)!=0;uint32_t count=flags*2654435761u;
 StudioIndexBufferUnlockStateCandidate state{resource,0xffffffe0,uint8_t(flags),{3,7,11}};unsigned queries=0,emits=0,unlocks=0,diagnostics=0;
 studioIndexBufferUnlockCandidate(state,count,[&]{++queries;return queued;},
 [&](auto command){if(command.resource!=resource||command.opcode!=((flags&4)?31u:19u)||state.usedIndices!=0xffffffe0)throw 1;if(flags&4){if(command.metadata!=state.metadata||command.bytes!=count*2)throw 1;}++emits;},
 [&](uintptr_t selected){if(selected!=resource||state.usedIndices!=0xffffffe0)throw 1;++unlocks;return failed?int32_t(-1):int32_t(0);},
 [&](unsigned line){if(!unlocks||line!=((flags&4)?0x420u:0x406u))throw 1;++diagnostics;});
 bool active=(flags&1)&&resource;
 if(queries!=unsigned(active)||emits!=unsigned(active&&queued)||unlocks!=unsigned(active&&!queued)||diagnostics!=unsigned(active&&!queued&&failed))return 1;
 if(state.usedIndices!=(active?uint32_t(0xffffffe0+count):0xffffffe0)||state.flags!=uint8_t(active?(flags&0xfe):flags))return 2;++cases;
}
std::cout<<"index_buffer_unlock_cases="<<cases<<" typed_commands=1 negative_hresult_advances=1 count_wrap=1\n";
}
