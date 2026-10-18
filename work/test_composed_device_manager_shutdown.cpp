#include <array>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include "studio_composed_device_manager_shutdown_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Backend {
 StudioDeviceManagerBackendCandidate& device;StudioDeviceManagerBaseShutdownCandidate& base;uintptr_t& cell;std::vector<unsigned>& trace;uintptr_t provider;
 void release(uintptr_t p){check(p==provider&&device.direct3D==provider&&cell==provider&&base.service54==provider);trace.push_back(0);device.direct3D=0xabcd;}
};
struct Services {
 StudioDeviceManagerBackendCandidate& device;
 StudioDeviceManagerBaseShutdownCandidate& base;std::vector<unsigned>& trace;
 void resourceSlot9(uintptr_t p,uintptr_t t){check(device.direct3D==0&&p&&t&&base.service54==0&&base.service58==0);trace.push_back(1);}
 void cvarSlot32(uintptr_t p,unsigned h){check(device.direct3D==0&&p&&h==7);trace.push_back(2);}
 void cvarSlot11(uintptr_t p,unsigned h){check(device.direct3D==0&&p&&h==7);trace.push_back(3);}
};
struct Allocations {
 uintptr_t& cell;StudioDeviceManagerBaseShutdownCandidate& base;std::vector<unsigned>& trace;StudioOwnedKeyValuesNodeCandidate* root;unsigned payloadFrees=0,nodeFrees=0;
 void freePayload(uintptr_t p){check(cell==0&&base.ownedTree==reinterpret_cast<uintptr_t>(root));++payloadFrees;trace.push_back(p?4:5);}
 void freeNode(StudioOwnedKeyValuesNodeCandidate* p){check(cell==0&&p==root&&p->payloadA==0&&p->payloadB==0);++nodeFrees;trace.push_back(6);}
};
int main(){static_assert(sizeof(void*)==8,"x64");unsigned cases=0;std::ifstream input("outputs/native-shutdown-chain.bin",std::ios::binary);
 for(bool resource:{false,true})for(bool cvar:{false,true})for(bool tree:{false,true})for(bool hasDevice:{false,true}){
 std::array<unsigned,2> providers{};uintptr_t provider=reinterpret_cast<uintptr_t>(&providers[0]),payload=reinterpret_cast<uintptr_t>(&providers[1]);StudioOwnedKeyValuesNodeCandidate root{payload,0,nullptr,nullptr};uintptr_t cell=provider;StudioDeviceManagerBaseShutdownCandidate base{provider,provider,tree?reinterpret_cast<uintptr_t>(&root):0};StudioShutdownServicesCandidate serviceState{provider,provider,payload,7,resource,cvar};StudioInterfaceConnectorCandidate interfaces;interfaces.connection=1;interfaces.tracked=1;interfaces.records[0]={&cell,0};std::vector<unsigned> trace;StudioDeviceManagerBackendCandidate device{hasDevice?provider:0};Backend backend{device,base,cell,trace,provider};Services services{device,base,trace};Allocations allocations{cell,base,trace,&root};check(provider>UINT32_MAX&&reinterpret_cast<uintptr_t>(&root)>UINT32_MAX);studioComposedDeviceManagerShutdownCandidate(device,base,serviceState,interfaces,backend,services,allocations);std::vector<unsigned> expected;if(hasDevice)expected.push_back(0);if(resource)expected.push_back(1);if(cvar){expected.push_back(2);expected.push_back(3);}if(tree){expected.push_back(4);expected.push_back(5);expected.push_back(6);}check(device.direct3D==0&&trace==expected&&cell==0&&base.service54==0&&base.service58==0&&base.ownedTree==0&&interfaces.connection==0&&interfaces.tracked==1&&allocations.nodeFrees==unsigned(tree)&&allocations.payloadFrees==(tree?2u:0u));check(!serviceState.resourceRegistered&&!serviceState.cvarRegistered&&serviceState.cvarHandle==(cvar?0xffffffffu:7u));std::array<unsigned,24> wire{},original{};wire[0]=static_cast<unsigned>(trace.size());std::copy(trace.begin(),trace.end(),wire.begin()+1);wire[8]=serviceState.cvarHandle;wire[9]=interfaces.tracked;wire[10]=interfaces.connection;wire[11]=allocations.payloadFrees;wire[12]=allocations.nodeFrees;input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&wire==original);++cases;
 }
 check(input.peek()==std::char_traits<char>::eof());std::printf("x64_composed_owned_shutdown_cases=%u passed\n",cases);
}


