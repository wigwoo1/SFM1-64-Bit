#include <array>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include "studio_interface_connector_candidate.h"
#include "studio_interface_factory_fallback_candidate.h"
#include "studio_vertex_allocation_tracker_noop_candidate.h"
#include "studio_material_interface_factory_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct MaterialOps {
 bool present,provided; StudioInterfaceFactoryNodeCandidate* head;
 uintptr_t upstreamObject,localObject; unsigned upstreamCalls=0,localCalls=0;
 uintptr_t direct(unsigned){return 0;}
 bool hasUpstream(){return present;}
 uintptr_t upstream(const char* name,int* status){check(status==nullptr);++upstreamCalls;return provided&&!std::strcmp(name,"VBAllocTracker001")?upstreamObject:0;}
 uintptr_t create(const StudioInterfaceFactoryNodeCandidate&){++localCalls;return localObject;}
};
struct Ops{StudioInterfaceFactoryNodeCandidate* head;uintptr_t localObject,duplicateObject;MaterialOps* material;
 uintptr_t external(const char* name,int* status){check(status==nullptr);return studioMaterialInterfaceFactoryCandidate(material->head,name,status,*material);}
 uintptr_t create(const StudioInterfaceFactoryNodeCandidate& node){return node.token?duplicateObject:localObject;}
 uintptr_t factory(int f,const char* name){check(f==0);return studioInterfaceFactoryFallbackCandidate(head,name,nullptr,*this);}
 void diagnostic(unsigned){std::abort();}
};
int main(){static_assert(sizeof(void*)==8,"x64");std::ifstream input("outputs/native-tracker-nested.bin",std::ios::binary);unsigned cases=0;StudioVertexAllocationTrackerNoOpCandidate localProvider;std::array<unsigned,3> otherProviders{};uintptr_t localObject=reinterpret_cast<uintptr_t>(&localProvider),duplicateObject=reinterpret_cast<uintptr_t>(&otherProviders[0]),externalObject=reinterpret_cast<uintptr_t>(&otherProviders[1]);uintptr_t materialObject=reinterpret_cast<uintptr_t>(&otherProviders[2]);check(materialObject>UINT32_MAX);check(localObject>UINT32_MAX&&duplicateObject>UINT32_MAX&&externalObject>UINT32_MAX);
 for(bool provided:{false,true})for(bool local:{false,true})for(bool duplicate:{false,true})for(unsigned initial:{0u,1u})for(bool seeded:{false,true})for(bool upstreamPresent:{false,true})for(bool materialLocal:{false,true}){std::array<uintptr_t,56> cells{};cells[19]=seeded?0xabcd:0;std::array<StudioInterfaceBindingCandidate,56> bindings{};for(unsigned i=0;i<56;++i)bindings[i]={i==19?"VBAllocTracker001":"unavailable",&cells[i]};StudioInterfaceFactoryNodeCandidate node{"VBAllocTracker001",0,nullptr},dup{"VBAllocTracker001",1,&node};StudioInterfaceFactoryNodeCandidate materialNode{"VBAllocTracker001",0,nullptr};MaterialOps materialOps{upstreamPresent,provided,materialLocal?&materialNode:nullptr,externalObject,materialObject};Ops ops{local?(duplicate?&dup:&node):nullptr,localObject,duplicateObject,&materialOps};StudioInterfaceConnectorCandidate state;state.tracked=initial;state.connection=9;for(auto& record:state.records)record={reinterpret_cast<uintptr_t*>(uintptr_t{0xabababab}),0xabababab};if(initial)state.records[0]={&cells[19],8};studioConnectInterfacesCandidate(state,bindings,1,ops);
 uintptr_t selected=!initial&&seeded?0xabcdu:(provided&&upstreamPresent)?externalObject:materialLocal?materialObject:local?(duplicate?duplicateObject:localObject):0u;check(cells[19]==selected&&node.next==nullptr&&dup.next==&node);std::array<unsigned,188> actual{},original{};for(unsigned i=0;i<56;++i)if(i!=19){check(cells[i]==0);actual[i]=0;}actual[19]=!initial&&seeded?4u:(provided&&upstreamPresent)?1u:materialLocal?5u:local?(duplicate?3u:2u):0u;for(unsigned i=0;i<64;++i){actual[57+2*i]=state.records[i].connection;if(i<state.tracked){check(state.records[i].output==&cells[19]&&reinterpret_cast<uintptr_t>(state.records[i].output)>UINT32_MAX);actual[56+2*i]=20;}else{check(reinterpret_cast<uintptr_t>(state.records[i].output)==0xabababab);actual[56+2*i]=0xabababab;}}actual[184]=state.tracked;actual[185]=state.connection;actual[186]=materialOps.upstreamCalls;actual[187]=materialOps.localCalls;check(materialNode.next==nullptr);input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&actual==original);++cases;}
 check(input.peek()==std::char_traits<char>::eof());std::printf("x64_tracker_connector_factory_nested_records_matched=%u passed\n",cases);
}


