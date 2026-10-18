#include "model_render_record_storage_candidate.h"
#include "model_render_record_initialize_candidate.h"
#include <cstdio>
#include <cstdlib>
#define check(x) do{if(!(x)){std::fprintf(stderr,"failed line %d\n",__LINE__);std::abort();}}while(false)
int main(){
 ModelRenderRecordAllocationCandidate state;state.pool.maximumIndex=0xffff;ModelRenderRecordStorageCandidate storage;unsigned relocations=0;
 auto grow=[&](auto&,int additional){auto* old=state.pool.links;storage.grow(state,0,additional);if(old!=state.pool.links)++relocations;};
 auto warn=[](bool){std::abort();};
 for(unsigned i=0;i<65535;++i){
  auto index=modelRenderRecordAllocateCandidate(state,0,grow,warn);check(index==i);auto* record=storage.record(index);
  for(unsigned j=0;j<264;++j)record[j]=static_cast<unsigned char>((i*17+j*29)>>3);
  initializeModelRenderRecordBytesCandidate(record);
  check(state.cachedStorage==state.pool.links&&storage.capacity()==size_t(state.pool.capacity));
 }
 for(unsigned i=0;i<65535;++i){auto* record=storage.record(uint16_t(i));
  for(unsigned j=0;j<264;++j){bool zero=(j>=0x0c&&j<0x58)||(j>=0x68&&j<0xb4);check(record[j]==(zero?0:static_cast<unsigned char>((i*17+j*29)>>3)));}
  check(state.pool.links[i].previous==i&&state.pool.links[i].next==i);
 }
 check(relocations==17);state.freeHead=123;state.pool.links[123].next=456;state.pool.links[456].next=0xffff;
 auto beforeCapacity=storage.capacity();check(modelRenderRecordAllocateCandidate(state,1,grow,warn)==123);check(modelRenderRecordAllocateCandidate(state,1,grow,warn)==456);check(storage.capacity()==beforeCapacity&&state.freeHead==0xffff);
 check(state.pool.links[123].previous==0xffff&&state.pool.links[456].next==0xffff);
 std::puts("PASS: 65535 full264-byte model schemas preserved across17 forced storage replacements; constructor ranges, logical links, cached storage and free-entry reuse verified. Pointer-bearing runtime ABI unverified.");
}
