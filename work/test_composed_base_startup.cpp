#include <cstdio>
#include <cstdlib>
#include "studio_composed_base_startup_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Factories {
 uintptr_t token,provider;bool cvar,file,utility;unsigned calls=0,creates=0;
 uintptr_t factory(uintptr_t f,const char* name,int* status){check(f==token&&status==nullptr);++calls;return ((!std::strcmp(name,"VEngineCvar007")&&cvar)||(!std::strcmp(name,"VFileSystem017")&&file)||(!std::strcmp(name,"VShaderUtil001")&&utility))?provider:0;}
 uintptr_t create(const StudioInterfaceFactoryNodeCandidate& n){check(!std::strcmp(n.name,"VShaderUtil001"));++creates;return provider;}
};
struct Host {
 StudioDeviceManagerBaseStartupCandidate& state;unsigned init=0,registrations=0,resources=0,warnings=0,timing=0;
 void diagnostic(unsigned){std::abort();}
 void initializeCvarAccess(){++init;check(state.accessInitializationFlags==1);}
 void registerCvars(unsigned flags){check(flags==0x800000&&state.cvar);++registrations;}
 void connectResources(){check(state.incomingFactory);++resources;}
 void warning(const char*){check(state.incomingFactory==0);++warnings;}
 void initializeMathLibrary(float a,float b,float c,int d,int e,int f,int g,int h){check(a==2.2f&&b==2.2f&&c==0&&d==2&&e==1&&f==1&&g==1&&h==1&&state.incomingFactory==0);++timing;}
};
int main(){static_assert(sizeof(void*)==8,"x64");unsigned objects[2]{};uintptr_t provider=reinterpret_cast<uintptr_t>(&objects[0]),token=reinterpret_cast<uintptr_t>(&objects[1]);check(provider>UINT32_MAX&&token>UINT32_MAX);unsigned cases=0;
 for(bool cvar:{false,true})for(bool file:{false,true})for(bool utility:{false,true})for(bool local:{false,true}){
 StudioDeviceManagerBaseStartupCandidate state;StudioInterfaceConnectorCandidate connector;StudioBaseStartupBindingsCandidate bindings;StudioInterfaceFactoryNodeCandidate node{"VShaderUtil001",0,nullptr};Factories factories{token,provider,cvar,file,utility};Host host{state};bool result=studioComposedBaseStartupCandidate(state,connector,bindings,local?&node:nullptr,provider,token,factories,host);check(result==(file&&(utility||local))&&state.incomingFactory==0&&state.manager==provider&&state.shaderUtil==((utility||local)?provider:0)&&state.cvar==(cvar?provider:0)&&state.fileSystem==(file?provider:0)&&host.init==1&&host.registrations==unsigned(cvar)&&host.resources==1&&host.timing==unsigned(result)&&host.warnings==unsigned(!result)&&factories.calls==57&&connector.connection==1);auto table=bindings.table(state);unsigned tracked=0;for(unsigned i=0;i<56;++i){auto name=table[i].name;bool populated=(!std::strcmp(name,"VEngineCvar007")&&cvar)||(!std::strcmp(name,"VFileSystem017")&&file)||(!std::strcmp(name,"VShaderUtil001")&&(utility||local));check(*table[i].output==(populated?provider:0));if(populated){check(connector.records[tracked].output==table[i].output&&connector.records[tracked].connection==0&&reinterpret_cast<uintptr_t>(table[i].output)>UINT32_MAX);++tracked;}}check(connector.tracked==tracked&&node.next==nullptr);++cases;
 }
 std::printf("x64_composed_base_startup_interface_cases=%u passed\n",cases);
}

