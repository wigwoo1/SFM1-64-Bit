#include "studio_texture_binding_candidate.h"
#include "studio_sampler_submit_candidate.h"
#include <cstdio>
#include <vector>
struct Ops {
 StudioTextureBindingRecordCandidate& record;StudioTextureBindingOwnerCandidate& owner;StudioTextureBindingStageCandidate& cache;
 bool suppress,queued;unsigned textures=0,gates=0;std::vector<std::array<uint32_t,4>> submitted;
 uint32_t budget(){return 0xffffffffu;}
 void texture(uint32_t stage,uintptr_t resource){
  if(stage!=4||resource!=0x123456789abcdef0ull||cache.record!=&record)throw 1;
  ++textures;owner.epoch=7;record.bytes=31;record.sampler[0]=11;
 }
 void sampler(uint32_t stage,uint32_t type,uint32_t value){studioSamplerSubmitCandidate(stage,type,value,*this);}
 uint32_t suppressed(){++gates;if(cache.sampler[0]!=11)throw 2;record.sampler[1]=12;queued=!queued;return suppress?257u:256u;}
 bool queueEnabled(){return queued;}
 void queueSampler(const std::array<uint32_t,4>& p){submitted.push_back(p);}
 void directSampler(uint32_t s,uint32_t t,uint32_t v){submitted.push_back({17,s,t,v});}
 bool filteringEnabled(){record.sampler[4]=16;return true;}
 bool mipEnabled(){record.sampler[5]=17;return true;}
};
int main(){unsigned cases=0;
 for(unsigned mode=0;mode<4;++mode){
  uint32_t counter=5;StudioTextureBindingRecordCandidate record;record.resource=0x123456789abcdef0ull;record.sampler={1,2,3,4,5,6};record.bytes=9;record.usageCounter=&counter;
  StudioTextureBindingOwnerCandidate owner{13,1,0};StudioTextureBindingStageCandidate cache;cache.enabled=1;
  Ops ops{record,owner,cache,bool(mode&1),bool(mode&2),0,0,{}};
  studioTextureBindingCandidate(4,&record,0,cache,owner,ops);
  if(ops.textures!=1||ops.gates!=6||record.epoch!=7||owner.bytes!=44||counter!=36||record.uses!=1)return 1;
  if(cache.sampler!=std::array<uint32_t,6>{11,12,3,4,16,17})return 2;
  if(ops.submitted.size()!=(mode&1?0u:6u))return 3;
  if(!(mode&1)){
   const uint32_t types[]{1,2,3,6,5,7},values[]{11,12,3,16,4,17};
   for(unsigned i=0;i<6;++i)if(ops.submitted[i]!=std::array<uint32_t,4>{17,4,types[i],values[i]})return 4;
  }
  ++cases;
 }
 std::printf("texture_binding_callback_cases=%u live_accounting=1 live_sampler_fields=1 cache_advances_on_suppression=1\n",cases);return 0;
}
