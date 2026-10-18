#include "studio_vertex_texture_submit_candidate.h"
#include "native_d3d9_vertex_texture_candidate.h"
#include <cstdio>
#include <vector>
struct Event {uint32_t stage,type;uintptr_t value;bool queued;};
struct PlainOps {
 std::vector<Event> events;
 bool queueEnabled(){return false;}
 void directTexture(uint32_t s,uintptr_t r){events.push_back({s,0,r,false});}
 void directSampler(uint32_t s,uint32_t t,uint32_t v){events.push_back({s,t,v,false});}
 void queueTexture(uint32_t,uintptr_t){throw 8;}
 void queueSampler(uint32_t,uint32_t,uint32_t){throw 9;}
};
struct Ops {
 StudioVertexTextureRecordCandidate& record;
 StudioVertexTextureRecordCandidate*& current;
 StudioVertexTextureRecordCandidate& replacement;
 StudioVertexTextureCacheCandidate& cache;
 bool queued,mutate;
 std::vector<Event> events;
 bool queueEnabled(){return queued;}
 void texture(uint32_t stage,uintptr_t value,bool q){
  if(cache.record!=&record)throw 1;
  events.push_back({stage,0,value,q});
  if(mutate){record.sampler[0]=91;current=&replacement;queued=!queued;}
 }
 void sampler(uint32_t stage,uint32_t type,uint32_t value,bool q){
  unsigned field=type==1?0:type==2?1:type==6?4:type==5?3:5;
  if(cache.sampler[field]!=value)throw 2;
  events.push_back({stage,type,value,q});
  if(mutate&&type==1)record.sampler[1]=92;
 }
 void queueTexture(uint32_t s,uintptr_t r){texture(s,r,true);}
 void directTexture(uint32_t s,uintptr_t r){texture(s,r,false);}
 void queueSampler(uint32_t s,uint32_t t,uint32_t v){sampler(s,t,v,true);}
 void directSampler(uint32_t s,uint32_t t,uint32_t v){sampler(s,t,v,false);}
};
int main(){
 static_assert(sizeof(uintptr_t)==8,"x64 test required");
 unsigned cases=0;
 for(unsigned mask=0;mask<32;++mask)for(unsigned mode=0;mode<32;++mode){
  const bool same=mode&1,queued=mode&2,mutate=mode&4,array=mode&8;
  const uint32_t force=mode&16?257u:256u;
  uintptr_t resources[]={0x123456789abcdef0ull,0xabcdef0198765432ull};
  StudioVertexTextureRecordCandidate record{{11,12,13,14,15,16},uint8_t(array?2:1),1,array?reinterpret_cast<uintptr_t>(resources):resources[0]},replacement{};
  auto* current=&record;StudioVertexTextureCacheCandidate cache{};
  cache.record=same?&record:nullptr;cache.sampler=record.sampler;
  constexpr unsigned fields[]={0,1,4,3,5};constexpr uint32_t types[]={1,2,6,5,7};
  for(unsigned j=0;j<5;++j)if(mask&(1u<<j))cache.sampler[fields[j]]=0;
  const auto initial=cache;Ops ops{record,current,replacement,cache,queued,mutate,{}};
  studioVertexTextureSubmitCandidate(&current,&cache,1,force,ops);
  const bool texture=uint8_t(force)||!same;
  size_t event=0;
  if(texture){auto e=ops.events.at(event++);if(e.type||e.stage!=257||e.value!=resources[array?1:0]||e.queued!=queued)return 3;}
  for(unsigned j=0;j<5;++j){
   const unsigned f=fields[j];
   if(uint8_t(force)||initial.sampler[f]!=record.sampler[f]){
    auto e=ops.events.at(event++);if(e.type!=types[j]||e.stage!=257||e.value!=record.sampler[f]||e.queued!=(texture&&mutate?!queued:queued))return 4;
   }
  }
  if(event!=ops.events.size()||cache.record!=&record)return 5;
  ++cases;
 }
 unsigned boundaryCases=0;
 for(int32_t count:{-1,0,1,2,3})for(uint32_t force:{0u,256u,1u,257u}){
  StudioVertexTextureRecordCandidate record{{1,2,3,4,5,6},1,0,0x123456789abcdef0ull};
  StudioVertexTextureRecordCandidate* current[]{nullptr,&record,nullptr};
  StudioVertexTextureCacheCandidate cache[3]{};
  cache[0].record=&record;cache[2].record=&record;
  for(auto& c:cache)c.sampler={11,12,13,14,15,16};
  PlainOps ops;studioVertexTextureSubmitCandidate(current,cache,count,force,ops);
  const size_t expected=count<=0?0:count==1?1:count==2?7:8;
  if(ops.events.size()!=expected)return 6;
  if(count>0&&(cache[0].record||ops.events[0].stage!=257||ops.events[0].value||cache[0].sampler[0]!=11))return 7;
  if(count>=2&&(ops.events[1].stage!=258||ops.events[1].value!=record.resource))return 10;
  if(count==3&&(ops.events[7].stage!=259||ops.events[7].value||cache[2].record))return 11;
  ++boundaryCases;
 }
 std::printf("vertex_texture_cases=%u boundary_cases=%u full_width_resources=1 pre_submit_cache=1 retained_record_live_fields=1\n",cases,boundaryCases);
 return 0;
}
