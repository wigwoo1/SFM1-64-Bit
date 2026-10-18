#include <iostream>
#include "studio_dynamic_mesh_unlock_candidate.h"
#include "studio_dynamic_mesh_draw_range_candidate.h"
#include "studio_dynamic_mesh_buffer_unlock_candidate.h"
int main(){unsigned cases=0;
for(unsigned i=0;i<65536;++i)for(unsigned mode=0;mode<4;++mode){
 uint32_t vertices=i*65537u,indices=i*977u;uintptr_t descriptor=(mode&1)?uintptr_t(0x123456789000ull):0;
 StudioDynamicMeshUnlockStateCandidate state{0xfffffff0,0xffffffe0,99,77,true,uint32_t(mode&2)};unsigned calls=0;
 studioDynamicMeshUnlockCandidate(state,vertices,indices,descriptor,descriptor+0xa4,
 [&](uint32_t count,uintptr_t ptr){if(count!=vertices||ptr!=descriptor||state.totalVertices!=uint32_t(0xfffffff0+vertices)||state.totalIndices!=uint32_t(0xffffffe0+indices)||!state.locked||state.lastIndices!=99)throw 1;++calls;state.primitive=state.primitive?0:uint32_t(3);},
 [&](uint32_t count,uintptr_t ptr){if(calls!=1||count!=indices||ptr!=(descriptor?descriptor+0xa4:0)||!state.locked)throw 1;++calls;});
 if(calls!=((mode&2)?1u:2u)||state.locked||state.lastIndices!=indices||state.lastVertices!=uint16_t(vertices))return 1;++cases;
}
unsigned bufferCases=0;
for(unsigned flag=0;flag<256;++flag)for(unsigned value=0;value<256;++value){
 uint8_t locked=uint8_t(flag);uintptr_t buffer=uintptr_t(0x123456789000ull)+value;uint32_t count=value*2654435761u;unsigned callbacks=0;
 studioDynamicMeshBufferUnlockCandidate(locked,buffer,count,[&](uintptr_t selected,uint32_t requested){if(!locked||selected!=buffer||requested!=count)throw 2;++callbacks;});
 if(locked||callbacks!=unsigned(flag!=0))return 2;
 studioDynamicMeshBufferUnlockCandidate(locked,buffer,count,[&](uintptr_t,uint32_t){throw 3;});++bufferCases;
}
unsigned rangeCases=0;
for(uint32_t primitive=0;primitive<12;++primitive)for(bool overridden:{false,true})for(int32_t first:{int32_t(-1),int32_t(0),int32_t(7),INT32_MIN,INT32_MAX})for(int32_t count:{int32_t(0),int32_t(3),int32_t(-1)})for(uint32_t base:{0u,5u,0xfffffff0u}){
 unsigned queries=0;auto range=studioDynamicMeshDrawRangeCandidate(first,count,base,overridden,primitive,17,31,[&]{++queries;return 43u;});
 bool full=first==-1||!count;
 if(range.first!=(full?base:uint32_t(first)+base)||range.count!=(full?(overridden?43u:((primitive==0||primitive==10)?17u:31u)):uint32_t(count))||queries!=unsigned(full&&overridden))return 4;++rangeCases;
}
std::cout<<"resolved_draw_range_cases="<<rangeCases<<" ";
std::cout<<"buffer_unlock_cases="<<bufferCases<<" repeated_unlock_noop=1 ";
std::cout<<"dynamic_unlock_cases="<<cases<<" live_primitive=1 count_wrap_and_truncation=1 callback_order=1\n";
}
