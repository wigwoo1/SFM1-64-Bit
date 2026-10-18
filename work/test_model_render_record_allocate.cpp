#include "model_render_record_allocate_candidate.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
void check(bool ok){if(!ok)std::abort();}
int main(){std::vector<EngineShadowPoolLinkCandidate> links(65536);ModelRenderRecordAllocationCandidate state;state.pool.links=links.data();state.pool.capacity=65535;state.pool.maximumIndex=0xffff;unsigned growth=0,memoryWarn=0,indexWarn=0;
 auto grow=[&](auto&,int n){check(n==1);++growth;};auto warn=[&](bool index){index?++indexWarn:++memoryWarn;};
 for(unsigned i=0;i<65535;++i){check(modelRenderRecordAllocateCandidate(state,0,grow,warn)==i);check(links[i].previous==i&&links[i].next==i);}
 check(state.freshCount==0xffff&&state.pool.maximumIndex==0xfffe&&growth==0);
 check(modelRenderRecordAllocateCandidate(state,0,grow,warn)==0xffff&&growth==1&&memoryWarn==1&&indexWarn==0);
 state.freeHead=7;links[7].next=3;links[3].next=0xffff;check(modelRenderRecordAllocateCandidate(state,0x101,grow,warn)==7&&links[7].previous==0xffff&&links[7].next==0xffff);check(modelRenderRecordAllocateCandidate(state,0x100,grow,warn)==3&&links[3].previous==3&&links[3].next==3);check(state.freeHead==0xffff&&state.freshCount==0xffff&&state.pool.maximumIndex==0xfffe&&growth==1);
 state.pool.capacity=65536;check(modelRenderRecordAllocateCandidate(state,0,grow,warn)==0xffff&&indexWarn==1&&growth==1);
 ModelRenderRecordAllocationCandidate empty;empty.pool.maximumIndex=0xffff;std::vector<EngineShadowPoolLinkCandidate> replacement(8);check(modelRenderRecordAllocateCandidate(empty,1,[&](auto& pool,int){pool.capacity=8;pool.links=replacement.data();},[](bool){check(false);})==0);check(empty.cachedStorage==replacement.data()&&empty.freshCount==1&&replacement[0].previous==0xffff);
 ModelRenderRecordAllocationCandidate failed;failed.pool.maximumIndex=0xffff;unsigned warnings=0;check(modelRenderRecordAllocateCandidate(failed,0,[](auto&,int){},[&](bool range){check(!range);++warnings;})==0xffff&&warnings==1&&failed.freshCount==0);
 std::puts("PASS: all65535 fresh indices, exhaustion/range failure, reuse7->3, lowbyte flags and replacement storage. Native allocation/growth integration pending.");
}
