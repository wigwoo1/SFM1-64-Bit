#include "native_cpu_topology_candidate.h"
#include <cstdio>
struct Query{
 unsigned calls=0;int scenario;DWORD last=ERROR_INSUFFICIENT_BUFFER;
 bool query(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION data,DWORD* bytes){
  ++calls;const DWORD unit=sizeof(*data);
  if(scenario==1){last=ERROR_ACCESS_DENIED;*bytes=0xffffffffu;return false;}
  if(scenario==2){*bytes=unit+1;return false;}
  if(scenario==3){return true;}
  if(scenario==4){*bytes=unit*calls;return false;}
  if(!data){*bytes=unit*2;return false;}
  if(calls==2){*bytes=unit*4;return false;}
  data[0].Relationship=RelationProcessorCore;data[1].Relationship=RelationCache;
  data[2].Relationship=RelationProcessorCore;data[3].Relationship=static_cast<LOGICAL_PROCESSOR_RELATIONSHIP>(99);*bytes=unit*4;return true;
 }
 DWORD error(){return last;}
};
int main(){static_assert(sizeof(void*)==8,"x64 required");
 for(int scenario=0;scenario<5;++scenario){Query query{0,scenario};auto result=nativeCpuTopologyCandidate(query);
  if(scenario==0){if(!result.available||result.cores!=2||result.calls!=3)return 1;}
  else if(result.available||result.cores)return 2;
  else if(result.error!=static_cast<DWORD>(scenario==1?ERROR_ACCESS_DENIED:scenario==4?ERROR_RETRY:ERROR_INVALID_DATA))return 3;
 }
 NativeCpuTopologyQueryCandidate query;auto live=nativeCpuTopologyCandidate(query);
 std::printf("x64_topology_adapter_controlled_cases=5 host_record_bytes=%zu live_available=%u live_cores=%u live_error=%lu live_calls=%u\n",sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION),live.available?1u:0u,live.cores,live.error,live.calls);
 return live.available?0:4;
}
