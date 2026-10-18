#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "studio_profile_counter_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Ops{
 StudioProfileCounterCandidate& state;unsigned thread,threadCalls=0,allocations=0,releases=0;std::array<char,64> storage{};
 unsigned currentThread(){++threadCalls;return thread;}
 int compare(const char* a,const char* b){return std::strcmp(a,b);}
 char* allocate(std::size_t size){check(size<=storage.size()&&++allocations==1);return storage.data();}
 void release(char* pointer){check(releases<state.count&&pointer==state.names[releases]);if(releases)check(state.names[releases-1]==nullptr);++releases;}
};
int main(){static_assert(sizeof(void*)==8,"x64 comparison");std::ifstream records("outputs/native-profile-counter-lifecycle.bin",std::ios::binary);unsigned cases=0;char existing[]="existing";
 for(unsigned count:{0u,1u,254u,255u})for(bool sameThread:{false,true})for(bool sameName:{false,true})for(unsigned group:{2u,255u}){
 StudioProfileCounterCandidate state;state.count=count;state.threadId=42;for(unsigned i=0;i<256;++i){state.values[i]=7+i;state.groups[i]=2;state.names[i]=i<count?existing:nullptr;}Ops ops{state,sameThread?42u:43u};unsigned fallback=99;const char* name=sameName?existing:"new";bool rejected=count==255||!sameThread;bool insert=!rejected&&!(sameName&&count>0);unsigned index=insert?count:0,liveCount=count+unsigned(insert);
 auto* returned=studioFindProfileCounterCandidate(state,name,group,fallback,ops);check(returned==(rejected?&fallback:&state.values[index])&&state.count==liveCount&&ops.allocations==unsigned(insert)&&ops.threadCalls==(count==255?0u:1u));if(insert){check(state.names[count]==ops.storage.data()&&reinterpret_cast<uintptr_t>(state.names[count])>UINT32_MAX&&!std::strcmp(state.names[count],name));}
 auto beforeValues=state.values;auto beforeGroups=state.groups;int resetGroup=group==255?-1:2;studioResetProfileCounterGroupCandidate(state,resetGroup);for(unsigned i=0;i<256;++i){unsigned expected=i<liveCount&&static_cast<std::int8_t>(beforeGroups[i])==resetGroup?0u:beforeValues[i];check(state.values[i]==expected);}auto resetValues=state.values;studioCleanupProfileCounterNamesCandidate(state,ops);check(state.count==0&&ops.releases==liveCount&&state.values==resetValues&&state.groups==beforeGroups&&fallback==99);for(auto pointer:state.names)check(pointer==nullptr);
 std::array<unsigned,1670> actual{},original{};actual.fill(0);std::fill(actual.begin(),actual.begin()+1664,0xabababab);actual[0x10b4/4]=actual[0x10b8/4]=actual[0x10bc/4]=actual[0x1094/4]=0;actual[0x19c8/4]=state.count;actual[0x19cc/4]=state.threadId;for(unsigned i=0;i<256;++i){actual[0x10c8/4+i]=state.values[i];reinterpret_cast<unsigned char*>(actual.data())[0x14c8+i]=state.groups[i];actual[0x15c8/4+i]=0;}actual[1664]=rejected?0u:index+1;actual[1665]=ops.allocations;actual[1666]=ops.releases;actual[1667]=ops.threadCalls;actual[1668]=1;actual[1669]=fallback;records.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(records)&&actual==original);++cases;
 }
 check(records.peek()==std::char_traits<char>::eof());std::printf("x64_profile_counter_lifecycle_records_matched=%u passed\n",cases);
}
