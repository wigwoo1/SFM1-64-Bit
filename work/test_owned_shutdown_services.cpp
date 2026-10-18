#include <array>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include "studio_owned_shutdown_services_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Services {
 StudioDeviceManagerBaseShutdownCandidate& base;std::vector<unsigned>& trace;
 void resourceSlot9(uintptr_t p,uintptr_t t){check(p&&t&&base.service54==0&&base.service58==0);trace.push_back(1);}
 void cvarSlot32(uintptr_t p,unsigned h){check(p&&h==7);trace.push_back(2);}
 void cvarSlot11(uintptr_t p,unsigned h){check(p&&h==7);trace.push_back(3);}
};
struct Allocations {
 uintptr_t& cell;StudioDeviceManagerBaseShutdownCandidate& base;std::vector<unsigned>& trace;StudioOwnedKeyValuesNodeCandidate* root;unsigned payloadFrees=0,nodeFrees=0;
 void freePayload(uintptr_t p){check(cell==0&&base.ownedTree==reinterpret_cast<uintptr_t>(root));++payloadFrees;trace.push_back(p?4:5);}
 void freeNode(StudioOwnedKeyValuesNodeCandidate* p){check(cell==0&&p==root&&p->payloadA==0&&p->payloadB==0);++nodeFrees;trace.push_back(6);}
};
int main(){static_assert(sizeof(void*)==8,"x64");unsigned cases=0;
 for(bool resource:{false,true})for(bool cvar:{false,true})for(bool tree:{false,true}){
 std::array<unsigned,2> providers{};uintptr_t provider=reinterpret_cast<uintptr_t>(&providers[0]),payload=reinterpret_cast<uintptr_t>(&providers[1]);StudioOwnedKeyValuesNodeCandidate root{payload,0,nullptr,nullptr};uintptr_t cell=provider;StudioDeviceManagerBaseShutdownCandidate base{provider,provider,tree?reinterpret_cast<uintptr_t>(&root):0};StudioShutdownServicesCandidate serviceState{provider,provider,payload,7,resource,cvar};StudioInterfaceConnectorCandidate interfaces;interfaces.connection=1;interfaces.tracked=1;interfaces.records[0]={&cell,0};std::vector<unsigned> trace;Services services{base,trace};Allocations allocations{cell,base,trace,&root};StudioOwnedShutdownServicesCandidate<Services,Allocations> ops{services,allocations};check(provider>UINT32_MAX&&reinterpret_cast<uintptr_t>(&root)>UINT32_MAX);studioComposedBaseShutdownCandidate(base,serviceState,interfaces,ops);std::vector<unsigned> expected;if(resource)expected.push_back(1);if(cvar){expected.push_back(2);expected.push_back(3);}if(tree){expected.push_back(4);expected.push_back(5);expected.push_back(6);}check(trace==expected&&cell==0&&base.service54==0&&base.service58==0&&base.ownedTree==0&&interfaces.connection==0&&interfaces.tracked==1&&allocations.nodeFrees==unsigned(tree)&&allocations.payloadFrees==(tree?2u:0u));++cases;
 }
 std::printf("x64_composed_owned_shutdown_cases=%u passed\n",cases);
}
