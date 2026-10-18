#include <array>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "studio_interface_connector_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Ops{unsigned offers;std::array<unsigned,2> calls{};uintptr_t baseA=0x1234,baseB=0x5678;
 uintptr_t factory(int f,const char* name){check(f>=0&&f<2);++calls[f];return !std::strcmp(name,"VBAllocTracker001")&&(offers&(1u<<f))?(f==0?baseA:baseB):0;}
 void diagnostic(unsigned){std::abort();}
};
int main(){static_assert(sizeof(void*)==8,"x64");std::ifstream input("outputs/native-tracker-connector.bin",std::ios::binary);unsigned cases=0;
 for(unsigned initial:{0u,1u})for(bool seeded:{false,true})for(unsigned mask=0;mask<4;++mask)for(unsigned count=0;count<3;++count){std::array<uintptr_t,56> cells{};cells[19]=seeded?0xabcd:0;std::array<StudioInterfaceBindingCandidate,56> table{};for(unsigned i=0;i<56;++i)table[i]={i==19?"VBAllocTracker001":"unavailable",&cells[i]};check(reinterpret_cast<uintptr_t>(&cells[19])>UINT32_MAX);StudioInterfaceConnectorCandidate state;for(auto& r:state.records)r={reinterpret_cast<uintptr_t*>(uintptr_t{0xabababab}),0xabababab};state.tracked=initial;state.connection=9;if(initial)state.records[0]={&cells[19],8};Ops ops{mask};studioConnectInterfacesCandidate(state,table,static_cast<int>(count),ops);
 std::array<unsigned,188> actual{},original{};for(unsigned i=0;i<56;++i){check(cells[i]<=UINT32_MAX);actual[i]=static_cast<unsigned>(cells[i]);}for(unsigned i=0;i<64;++i){actual[57+2*i]=state.records[i].connection;if(i<state.tracked){check(state.records[i].output==&cells[19]);actual[56+2*i]=20;}else{check(reinterpret_cast<uintptr_t>(state.records[i].output)==0xabababab);actual[56+2*i]=0xabababab;}}actual[184]=state.tracked;actual[185]=state.connection;actual[186]=ops.calls[0];actual[187]=ops.calls[1];input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&actual==original);++cases;}
 check(input.peek()==std::char_traits<char>::eof());std::array<uintptr_t,56> cells{};std::array<StudioInterfaceBindingCandidate,56> table{};for(unsigned i=0;i<56;++i)table[i]={i==19?"VBAllocTracker001":"unavailable",&cells[i]};StudioInterfaceConnectorCandidate state;Ops ops{3};ops.baseA=0x100001234ull;ops.baseB=0x100005678ull;studioConnectInterfacesCandidate(state,table,2,ops);check(cells[19]==ops.baseA&&state.records[0].output==&cells[19]);studioConnectInterfacesCandidate(state,table,2,ops);check(cells[19]==ops.baseB&&state.tracked==1);std::printf("x64_tracker_connector_records_matched=%u full_width_case=passed\n",cases);
}
