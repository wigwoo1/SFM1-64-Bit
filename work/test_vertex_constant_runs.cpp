#include "studio_vertex_constant_runs_candidate.h"
#include <array>
#include <vector>
#include <utility>
#include <cstdio>
#include <initializer_list>
#include <limits>
struct Ops {
 std::array<float,40> values{},cached{},alternateValues{},alternateCache{};
 float* source=values.data();float* target=cached.data();bool replace=false;
 std::vector<std::pair<uint32_t,uint32_t>> runs;
 float* current(){return source;}float* cache(){return target;}
 void submit(uint32_t first,uint32_t count,const float* data){
  if(data!=source+first*4||first<1||first+count>9)throw 1;
  runs.emplace_back(first,count);
  if(replace){std::memcpy(alternateValues.data(),source,160);std::memcpy(alternateCache.data(),target,160);source=alternateValues.data();target=alternateCache.data();}
  source[first*4]=42; // Cache must use live source after this callback.
 }
};
int main(){unsigned cases=0;
 for(unsigned mask=0;mask<256;++mask)for(uint32_t force:{0u,256u,1u,257u})for(bool replace:{false,true}){
  Ops ops;ops.replace=replace;
  for(unsigned i=0;i<8;++i)if(mask&(1u<<i))ops.values[(i+1)*4]=1;
  std::vector<std::pair<uint32_t,uint32_t>> expected;
  for(unsigned i=0;i<8;){if(!uint8_t(force)&&!(mask&(1u<<i))){++i;continue;}unsigned first=i++;while(i<8&&(uint8_t(force)||(mask&(1u<<i))))++i;expected.emplace_back(first+1,i-first);}
  studioVertexConstantRunsCandidate(1,8,force,ops);
  if(ops.runs!=expected)return 2;
  for(auto run:expected)if(std::memcmp(ops.source+run.first*4,ops.target+run.first*4,run.second*16))return 3;
  ++cases;
 }
 Ops zeros;zeros.values[4]=-0.0f;studioVertexConstantRunsCandidate(1,8,0,zeros);if(!zeros.runs.empty())return 4;
 Ops nan;nan.values[4]=nan.cached[4]=std::numeric_limits<float>::quiet_NaN();studioVertexConstantRunsCandidate(1,8,0,nan);if(nan.runs!=std::vector<std::pair<uint32_t,uint32_t>>{{1,1}})return 5;
 std::printf("vertex_constant_run_cases=%u signed_zero_equal=1 nan_dirty=1 live_buffer_reload=1\n",cases);return 0;
}
