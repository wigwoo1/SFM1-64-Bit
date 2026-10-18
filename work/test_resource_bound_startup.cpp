#include <cstdio>
#include <cstdlib>
#include "studio_live_base_shutdown_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Factory {
 uintptr_t provider,token;bool file,utility;
 uintptr_t factory(uintptr_t f,const char* n,int* status){check(f==token&&status==nullptr);return ((!std::strcmp(n,"VFileSystem017")&&file)||(!std::strcmp(n,"VShaderUtil001")&&utility)||!std::strcmp(n,"ResourceSystem004")||!std::strcmp(n,"VPrecacheSystem001"))?provider:0;}
 uintptr_t create(const StudioInterfaceFactoryNodeCandidate&){std::abort();}
};
struct Host {
 unsigned warnings=0,timing=0;
 void diagnostic(unsigned){std::abort();}
 void initializeCvarAccess(){}
 void registerCvars(unsigned){std::abort();}
 void warning(const char*){++warnings;}
 void initializeMathLibrary(float,float,float,int,int,int,int,int){++timing;}
};
struct Allocation {
 unsigned calls=0;void freePayload(uintptr_t){++calls;}
 void freeNode(StudioOwnedKeyValuesNodeCandidate*){++calls;}
};
struct ResourceOps {
 StudioBaseStartupBindingsCandidate& bindings;StudioShutdownServicesCandidate& services;
 uintptr_t first,second;unsigned groups=0,registrations=0,nodes=0,unregistrations=0,cvarCalls=0;
  void resourceSlot9(uintptr_t p,uintptr_t token){check(p==second&&token==second&&services.resourceRegistered&&bindings.cells[9]==second);++unregistrations;}
 void cvarSlot32(uintptr_t,unsigned){++cvarCalls;}
 void cvarSlot11(uintptr_t,unsigned){++cvarCalls;}
 void precacheSlot10(uintptr_t p,uintptr_t g,unsigned i){check(p==(i?second:first)&&g==second&&i==groups);++groups;bindings.cells[25]=second;}
 void resourceSlot8(uintptr_t p,uintptr_t token){check(p==first&&token==second&&!services.resourceRegistered);++registrations;bindings.cells[9]=second;}
 void resourceSlot12(uintptr_t p,StudioResourceRegistrationNodeCandidate*){check(p==second&&services.resourceRegistered);++nodes;}
};
int main(){static_assert(sizeof(void*)==8,"x64");unsigned objects[3]{};uintptr_t first=reinterpret_cast<uintptr_t>(&objects[0]),second=reinterpret_cast<uintptr_t>(&objects[1]),token=reinterpret_cast<uintptr_t>(&objects[2]);check(first>UINT32_MAX&&second>UINT32_MAX&&token>UINT32_MAX);unsigned cases=0;
 for(bool file:{false,true})for(bool utility:{false,true}){StudioDeviceManagerBaseStartupCandidate state;StudioInterfaceConnectorCandidate connector;StudioBaseStartupBindingsCandidate bindings;StudioShutdownServicesCandidate services;services.resourceToken=second;StudioResourceRegistrationNodeCandidate node;StudioResourceStartupCandidate resources;resources.groups.fill(second);resources.head=&node;Factory factory{first,token,file,utility};Host host;ResourceOps resourceOps{bindings,services,first,second};StudioResourceBoundStartupHostCandidate<Host,ResourceOps> bound{host,resourceOps,bindings,services,resources};bool result=studioComposedBaseStartupCandidate(state,connector,bindings,nullptr,first,token,factory,bound);check(result==(file&&utility)&&resourceOps.groups==5&&resourceOps.registrations==1&&resourceOps.nodes==1&&bindings.cells[9]==second&&bindings.cells[25]==second&&resources.precacheInitialized&&services.resourceRegistered&&host.timing==unsigned(result)&&host.warnings==unsigned(!result));uintptr_t tree=0;Allocation allocation;studioLiveBaseShutdownCandidate(state,bindings,services,connector,tree,resourceOps,allocation);check(allocation.calls==0&&resourceOps.cvarCalls==0&&resourceOps.unregistrations==1&&!services.resourceRegistered&&bindings.cells[9]==0&&bindings.cells[25]==0&&state.manager==0&&state.shaderUtil==0&&state.fileSystem==0&&connector.connection==0&&tree==0);++cases;}
 std::printf("x64_live_resource_bound_startup_cases=%u passed\n",cases);
}



