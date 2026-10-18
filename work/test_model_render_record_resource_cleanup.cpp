#include "model_render_record_resource_cleanup_candidate.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#define check(x) do{if(!(x)){std::fprintf(stderr,"failed line %d\n",__LINE__);std::abort();}}while(false)
int main(){unsigned cases=0;
 std::vector<ModelRenderRecordResourceCandidate> original(65536),replacement(65536);
 for(unsigned index=0;index<65536;++index)for(unsigned mode=0;mode<4;++mode){
  uintptr_t value=(mode&1)?uintptr_t(0x123456789abcdef0ull):0;
  original[index].resource=value;replacement[index].resource=0xfedcba9876543210ull;
  auto* live=original.data();unsigned reads=0,calls=0;
  modelRenderRecordResourceCleanupCandidate(0xabcd0000u|index,[&](uint16_t selected)->ModelRenderRecordResourceCandidate&{check(selected==index);++reads;return live[selected];},[&](uintptr_t resource,int a,int b,int c){check(resource==value&&a==0&&b==0&&c==1);++calls;if(mode&2)live=replacement.data();});
  const bool invoked=index!=65535&&value!=0;check(calls==unsigned(invoked));check(reads==(index==65535?0u:invoked?2u:1u));
  check(original[index].resource==((invoked&&!(mode&2))?0:value));check(replacement[index].resource==((invoked&&(mode&2))?0:uintptr_t(0xfedcba9876543210ull)));++cases;
 }
 std::printf("PASS: %u cleanup states, all low16 indices/high aliases, null resource, full64-bit identity, callback arguments and callback storage replacement. Native resource service parity pending.\n",cases);
}
