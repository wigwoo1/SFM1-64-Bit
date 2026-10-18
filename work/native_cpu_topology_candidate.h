#pragma once
#include <Windows.h>
#include <vector>
#include "studio_cpu_topology_count_candidate.h"
struct NativeCpuTopologyResultCandidate{bool available=false;DWORD error=ERROR_GEN_FAILURE;uint32_t cores=0;unsigned calls=0;};
struct NativeCpuTopologyQueryCandidate{
 bool query(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION data,DWORD* bytes){return GetLogicalProcessorInformation(data,bytes)!=FALSE;}
 DWORD error(){return GetLastError();}
};
// Matches the recovered legacy API's current-processor-group scope. Full
// GetCPUInformation vendor selection and byte6 publication remain external.
template<class Query>NativeCpuTopologyResultCandidate nativeCpuTopologyCandidate(Query& query){
 NativeCpuTopologyResultCandidate result;std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> records;
 DWORD requested=0;
 for(unsigned attempt=0;attempt<8;++attempt){
  DWORD returned=requested;++result.calls;
  if(query.query(records.empty()?nullptr:records.data(),&returned)){
   if(returned==0||returned>requested||returned%sizeof(records[0])){result.error=ERROR_INVALID_DATA;return result;}
   const auto count=returned/sizeof(records[0]);std::vector<unsigned char> normalized(count*24,0);
   for(size_t i=0;i<count;++i){uint32_t relationship=static_cast<uint32_t>(records[i].Relationship);std::memcpy(normalized.data()+i*24+4,&relationship,4);}
   result.cores=studioCpuTopologyCountCandidate(normalized.data(),static_cast<uint32_t>(count));result.available=true;result.error=ERROR_SUCCESS;return result;
  }
  result.error=query.error();if(result.error!=ERROR_INSUFFICIENT_BUFFER)return result;
  if(returned<=requested||returned%sizeof(records[0])){result.error=ERROR_INVALID_DATA;return result;}
  records.resize(returned/sizeof(records[0]));requested=returned;
 }
 result.error=ERROR_RETRY;return result;
}
