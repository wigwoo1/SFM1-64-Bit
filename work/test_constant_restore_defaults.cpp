#include "studio_constant_restore_defaults_candidate.h"
#include "studio_vertex_constant_submit_candidate.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <initializer_list>
struct Backend{
 std::array<uint32_t,904> first,second,replacement;std::array<uint32_t,64> block{};
 uint32_t* current;bool queued,mutate=false;std::vector<unsigned> trace;
 StudioDeviceQueueStateCandidate state{block.data(),block.data()+1,63};
 explicit Backend(bool q):current(first.data()),queued(q){first.fill(0x11111111);second.fill(0x22222222);replacement.fill(0x33333333);}
 uint32_t* primaryCache(){return current;}uint32_t* secondaryCache(){return second.data();}
 bool queueEnabled(){return queued;}auto& queueState(){return state;}
 void publish(uint32_t*){assert(false);}uint32_t* acquire(unsigned){assert(false);return nullptr;}
 void directVertex(uint32_t index,uint32_t count,const uint32_t*){assert(count==1);trace.push_back(index);if(mutate){current=replacement.data();mutate=false;}}
 void submitVertex(unsigned index,const std::array<uint32_t,4>& words){studioVertexConstantSubmitCandidate(index,words,*this);}
};
int main(){unsigned cases=0;const unsigned indices[]={0,1,3,225};
 for(bool queued:{false,true})for(unsigned mask=0;mask<16;++mask)for(unsigned inputCase=0;inputCase<4;++inputCase){
  const uint32_t inputs[]={0x40000000,0xc0000000,0,0x7f800000};const uint32_t reciprocals[]={0x3f000000,0xbf000000,0x7f800000,0};
  const std::array<std::array<uint32_t,4>,4> expected={{{0,0x3f800000,0x40000000,0x3f000000},{0x3ee8ba2e,inputs[inputCase],0x3eaaaaab,reciprocals[inputCase]},{0x41d80000,0x42000000,0x443f40a4,0x42680000},{0,0,0,0}}};
  Backend backend(queued);for(unsigned phase=0;phase<4;++phase)if(mask&(1u<<phase))std::memcpy(backend.first.data()+indices[phase]*4,expected[phase].data(),16);
  studioConstantRestoreDefaultsCandidate(inputs[inputCase],backend);unsigned submitted=0;
  for(unsigned phase=0;phase<4;++phase){assert(std::memcmp(backend.first.data()+indices[phase]*4,expected[phase].data(),16)==0);if(mask&(1u<<phase)){assert(backend.second[indices[phase]*4]==0x22222222);continue;}
   assert(std::memcmp(backend.second.data()+indices[phase]*4,expected[phase].data(),16)==0);
   if(queued){auto* command=backend.block.data()+1+submitted*7;assert(command[0]==10&&command[1]==indices[phase]&&command[2]==1);assert(std::memcmp(command+3,expected[phase].data(),16)==0);}else assert(backend.trace[submitted]==indices[phase]);++submitted;
  }if(queued)assert(backend.state.remaining==63-submitted*7&&backend.state.cursor==backend.block.data()+1+submitted*7);else assert(backend.trace.size()==submitted);++cases;
 }
 Backend changing(false);changing.mutate=true;studioConstantRestoreDefaultsCandidate(0x40000000,changing);assert(changing.current==changing.replacement.data()&&changing.trace.size()==4);assert(changing.first[0]==0x11111111&&changing.replacement[0]==0&&changing.replacement[7]==0x3f000000);std::printf("constant_restore_default_cases=%u mutation=1 passed\n",cases);
}
