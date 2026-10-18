#include "model_render_record_storage_candidate.h"
#include "model_render_record_initialize_candidate.h"
#include "model_render_record_insert_candidate.h"
#include "model_render_record_release_candidate.h"
#include <cstdio>
#include <cstdlib>
#define check(x) do{if(!(x)){std::fprintf(stderr,"failed line %d\n",__LINE__);std::abort();}}while(false)
int main(){
 ModelRenderRecordAllocationCandidate state;state.pool.maximumIndex=0xffff;ModelRenderRecordStorageCandidate storage;unsigned relocations=0;
 auto grow=[&](auto&,int additional){auto* old=state.pool.links;storage.grow(state,0,additional);if(old!=state.pool.links)++relocations;};
 auto warn=[](bool){std::abort();};
 for(unsigned i=0;i<65535;++i){
  auto index=modelRenderRecordAllocateCandidate(state,0,grow,warn);check(index==i);modelRenderRecordInsertCandidate(state.pool,0xffffffffu,index|0xabcd0000u);check(state.pool.tail==i&&state.pool.head==0&&state.pool.count==i+1);auto* record=storage.record(index);
  for(unsigned j=0;j<264;++j)record[j]=static_cast<unsigned char>((i*17+j*29)>>3);
  initializeModelRenderRecordBytesCandidate(record);
  check(state.cachedStorage==state.pool.links&&storage.capacity()==size_t(state.pool.capacity));
 }
 for(unsigned i=0;i<65535;++i){auto* record=storage.record(uint16_t(i));
  for(unsigned j=0;j<264;++j){bool zero=(j>=0x0c&&j<0x58)||(j>=0x68&&j<0xb4);check(record[j]==(zero?0:static_cast<unsigned char>((i*17+j*29)>>3)));}
  check(state.pool.links[i].previous==(i?uint16_t(i-1):uint16_t(0xffff))&&state.pool.links[i].next==(i+1<65535?uint16_t(i+1):uint16_t(0xffff)));
 }
 check(relocations==17);
 for(unsigned phase=0;phase<2;++phase)for(unsigned index=phase;index<65535;index+=2){modelRenderRecordReleaseCandidate(state,index|0xabcd0000u,[](uint16_t i){return uintptr_t(i);},[](uintptr_t){},[](uint32_t){},[](uint32_t){});check(state.pool.links[index].previous==index);}
 check(state.pool.count==0&&state.pool.head==0xffff&&state.pool.tail==0xffff);std::vector<bool> seen(65535);uint16_t node=state.freeHead;for(unsigned i=0;i<65535;++i){check(node<65535&&!seen[node]);seen[node]=true;node=state.pool.links[node].next;}check(node==0xffff);
 for(unsigned i=0;i<65535;++i){auto index=modelRenderRecordAllocateCandidate(state,0,grow,[](bool){std::abort();});check(index!=0xffff);modelRenderRecordInsertCandidate(state.pool,0xffff,index);}check(state.pool.count==65535&&storage.capacity()==65535&&state.freeHead==0xffff&&relocations==17);
 EngineShadowPoolLinkCandidate small[4]={{0,0},{1,1},{2,2},{3,3}};EngineShadowPoolStateCandidate move{small,4,0xffff,0xffff,0,3};for(unsigned i=0;i<4;++i)modelRenderRecordInsertCandidate(move,0xffff,i);modelRenderRecordInsertCandidate(move,0xabcd0000u,2);modelRenderRecordInsertCandidate(move,0xffff,0);check(move.head==2&&move.tail==0&&move.count==4);check(small[2].previous==0xffff&&small[2].next==1&&small[1].previous==2&&small[1].next==3&&small[3].previous==1&&small[3].next==0&&small[0].previous==3&&small[0].next==0xffff);
 std::puts("PASS: 65535 full264-byte model schemas preserved across17 forced storage replacements; constructor ranges, full active-list insertion/release and all65535 recycled-entry reinsertions verified. Pointer-bearing runtime ABI unverified.");
}
