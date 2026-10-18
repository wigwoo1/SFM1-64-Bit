#include <array>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include "studio_interface_connector_candidate.h"
#include "studio_interface_factory_fallback_candidate.h"
#include "studio_vertex_allocation_tracker_noop_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Ops{bool provided;StudioInterfaceFactoryNodeCandidate* head;uintptr_t localObject,duplicateObject,externalObject;unsigned calls=0;
 uintptr_t external(const char* name,int* status){check(status==nullptr);++calls;return provided&&!std::strcmp(name,"VBAllocTracker001")?externalObject:0;}
 uintptr_t create(const StudioInterfaceFactoryNodeCandidate& node){return node.token?duplicateObject:localObject;}
 uintptr_t factory(int f,const char* name){check(f==0);return studioInterfaceFactoryFallbackCandidate(head,name,nullptr,*this);}
 void diagnostic(unsigned){std::abort();}
};
int main(){static_assert(sizeof(void*)==8,"x64");std::ifstream input("outputs/native-tracker-composed.bin",std::ios::binary);unsigned cases=0;StudioVertexAllocationTrackerNoOpCandidate localProvider;std::array<unsigned,2> otherProviders{};uintptr_t localObject=reinterpret_cast<uintptr_t>(&localProvider),duplicateObject=reinterpret_cast<uintptr_t>(&otherProviders[0]),externalObject=reinterpret_cast<uintptr_t>(&otherProviders[1]);check(localObject>UINT32_MAX&&duplicateObject>UINT32_MAX&&externalObject>UINT32_MAX);
 for(bool provided:{false,true})for(bool local:{false,true})for(bool duplicate:{false,true})for(unsigned initial:{0u,1u})for(bool seeded:{false,true}){std::array<uintptr_t,56> cells{};cells[19]=seeded?0xabcd:0;std::array<StudioInterfaceBindingCandidate,56> bindings{};for(unsigned i=0;i<56;++i)bindings[i]={i==19?"VBAllocTracker001":"unavailable",&cells[i]};StudioInterfaceFactoryNodeCandidate node{"VBAllocTracker001",0,nullptr},dup{"VBAllocTracker001",1,&node};Ops ops{provided,local?(duplicate?&dup:&node):nullptr,localObject,duplicateObject,externalObject};StudioInterfaceConnectorCandidate state;state.tracked=initial;state.connection=9;for(auto& record:state.records)record={reinterpret_cast<uintptr_t*>(uintptr_t{0xabababab}),0xabababab};if(initial)state.records[0]={&cells[19],8};studioConnectInterfacesCandidate(state,bindings,1,ops);
 uintptr_t selected=!initial&&seeded?0xabcdu:provided?externalObject:local?(duplicate?duplicateObject:localObject):0u;check(cells[19]==selected&&node.next==nullptr&&dup.next==&node);std::array<unsigned,188> actual{},original{};for(unsigned i=0;i<56;++i)if(i!=19){check(cells[i]==0);actual[i]=0;}actual[19]=!initial&&seeded?4u:provided?1u:local?(duplicate?3u:2u):0u;for(unsigned i=0;i<64;++i){actual[57+2*i]=state.records[i].connection;if(i<state.tracked){check(state.records[i].output==&cells[19]&&reinterpret_cast<uintptr_t>(state.records[i].output)>UINT32_MAX);actual[56+2*i]=20;}else{check(reinterpret_cast<uintptr_t>(state.records[i].output)==0xabababab);actual[56+2*i]=0xabababab;}}actual[184]=state.tracked;actual[185]=state.connection;actual[186]=ops.calls;input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&actual==original);++cases;}
 check(input.peek()==std::char_traits<char>::eof());std::printf("x64_tracker_connector_factory_composed_records_matched=%u passed\n",cases);
}
