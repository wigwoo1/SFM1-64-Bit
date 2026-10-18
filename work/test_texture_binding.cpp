#include "studio_texture_binding_candidate.h"
#include <cstdio>
#include <vector>
struct BoundaryOps {
 uint32_t limit=0xffffffffu;unsigned textures=0;uintptr_t resource=0;
 uint32_t budget(){return limit;}
 void texture(uint32_t,uintptr_t r){++textures;resource=r;}
 void sampler(uint32_t,uint32_t,uint32_t){}
 bool filteringEnabled(){return true;}bool mipEnabled(){return true;}
};
struct Ops {
 uint32_t limit;bool filter,mip;StudioTextureBindingStageCandidate& cache;
 unsigned textures=0;uintptr_t resource=0;std::vector<uint32_t> types;
 uint32_t budget(){return limit;}
 void texture(uint32_t s,uintptr_t r){if(s!=3)throw 1;++textures;resource=r;}
 void sampler(uint32_t s,uint32_t t,uint32_t v){const unsigned f=t==1?0:t==2?1:t==3?2:t==6?4:t==5?3:5;if(s!=3||cache.sampler[f]!=v)throw 2;types.push_back(t);}
 bool filteringEnabled(){return filter;}bool mipEnabled(){return mip;}
};
int main(){unsigned cases=0;
 for(unsigned mode=0;mode<256;++mode){
  uint32_t counter=0;StudioTextureBindingRecordCandidate record;
  record.sampler={2,3,4,5,6,7};record.resource=0x123456789abcdef0ull;record.bytes=2048;record.usageCounter=&counter;
  StudioTextureBindingStageCandidate cache;cache.enabled=mode&1?1:0;cache.record=mode&2?&record:nullptr;
  StudioTextureBindingOwnerCandidate owner{0,mode&4?0u:1u,uint8_t(mode&8?1:0)};
  Ops ops{mode&16?1u:0xffffffffu,bool(mode&32),bool(mode&64),cache,0,0,{}};
  const uint32_t force=mode&128?257u:256u;
  const bool skip=(mode&2)&&!uint8_t(force);
  const bool reject=!skip&&(mode&16)&&!(mode&4);
  const bool execute=!skip&&!reject&&(mode&1);
  studioTextureBindingCandidate(3,&record,force,cache,owner,ops);
  if(ops.textures!=(reject||execute?1u:0u)||ops.resource!=(execute?record.resource:0))return 3;
  if(execute){
   if(record.epoch!=owner.epoch||counter!=(!(mode&4)?2048u:0u)||record.uses!=(mode&8?0u:1u))return 4;
   if(cache.sampler[4]!=(mode&32?6u:0u)||cache.sampler[3]!=(mode&32?5u:0u)||cache.sampler[5]!=(mode&64?(mode&32?7u:1u):0u))return 5;
   for(size_t i=1;i<ops.types.size();++i)if(ops.types[i]==1)return 6;
  }
  ++cases;
 }
 unsigned boundaries=0;
 {StudioTextureBindingStageCandidate cache;StudioTextureBindingRecordCandidate sentinel;cache.record=&sentinel;
  StudioTextureBindingOwnerCandidate owner;BoundaryOps ops;
  studioTextureBindingCandidate(3,nullptr,0,cache,owner,ops);
  if(ops.textures!=1||ops.resource||cache.record!=&sentinel)return 7;++boundaries;
 }
 for(unsigned index=0;index<2;++index){
  uintptr_t resources[]{0x123456789abcdef0ull,0xabcdef0123456789ull};
  StudioTextureBindingRecordCandidate record;record.resourceCount=2;record.activeResource=uint8_t(index);record.resource=reinterpret_cast<uintptr_t>(resources);
  StudioTextureBindingStageCandidate cache;cache.enabled=1;StudioTextureBindingOwnerCandidate owner;BoundaryOps ops;
  studioTextureBindingCandidate(3,&record,0,cache,owner,ops);
  if(ops.resource!=resources[index])return 8;++boundaries;
 }
 // Explicit expected native signed comparisons after32-bit addition and shift.
 struct BudgetCase {uint32_t used,bytes,budget;bool reject;};
 const BudgetCase budgets[]{
  {0,1024,1,false},{0,1025,1,true},
  {0x7fffffffu,1,1,false},{0xffffffffu,2,0,true},
  {0,1,0x00200000u,true},{0,1,0x00400000u,true},
  {0,0xffffffffu,0,false},{0xffffffffu,1,0,false}
 };
 for(auto b:budgets){
  StudioTextureBindingRecordCandidate record,sentinel;record.bytes=b.bytes;record.resource=0x123456789abcdef0ull;
  StudioTextureBindingStageCandidate cache;cache.enabled=1;cache.record=&sentinel;
  StudioTextureBindingOwnerCandidate owner{b.used,1,0};BoundaryOps ops;ops.limit=b.budget;
  studioTextureBindingCandidate(3,&record,0,cache,owner,ops);
  if(ops.textures!=1||ops.resource!=(b.reject?0:record.resource)||cache.record!=(b.reject?&sentinel:&record))return 9;
  ++boundaries;
 }
 struct UseCase {uint32_t uses,maximum,expectedUses,expectedMaximum;};
 const UseCase uses[]{
  {0xffffffffu,0,0,0},{0x7fffffffu,0x7fffffffu,0x80000000u,0x7fffffffu},
  {0xfffffffeu,0xffffffffu,0xffffffffu,0xffffffffu},{0,0xffffffffu,1,1}
 };
 for(auto u:uses){
  StudioTextureBindingRecordCandidate record;record.uses=u.uses;record.maximumUses=u.maximum;
  StudioTextureBindingStageCandidate cache;cache.enabled=1;StudioTextureBindingOwnerCandidate owner;BoundaryOps ops;
  studioTextureBindingCandidate(3,&record,0,cache,owner,ops);
  if(record.uses!=u.expectedUses||record.maximumUses!=u.expectedMaximum)return 10;++boundaries;
 }
 std::printf("texture_binding_cases=%u boundary_cases=%u pointer_width=1 budget_gate=1 epoch_accounting=1 filter_configuration=1\n",cases,boundaries);return 0;
}
