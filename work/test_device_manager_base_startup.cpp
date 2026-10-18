#include <array>
#include <vector>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "studio_device_manager_base_startup_candidate.h"
static void check(bool b){if(!b)std::abort();}
struct Ops {
 StudioDeviceManagerBaseStartupCandidate& state;uintptr_t provider,factory;bool cvar,file,external,local;std::vector<unsigned> trace;uintptr_t accessObject=0x1111;
 void connectInterfaces(){check(state.incomingFactory==factory);state.cvar=cvar?provider:0;state.fileSystem=file?provider:0;trace.push_back(1);}
 void initializeCvarAccess(){check(state.accessInitializationFlags&1);accessObject=provider;trace.push_back(2);}
 void registerCvars(unsigned flags){check(flags==0x800000&&state.cvar==provider);trace.push_back(3);}
 void connectResources(){check(state.incomingFactory==factory);trace.push_back(4);}
 uintptr_t externalFactory(uintptr_t f,const char* name){check(f==factory&&!std::strcmp(name,"VShaderUtil001"));trace.push_back(5);return external?provider:0;}
 uintptr_t localFactory(const char* name){check(!std::strcmp(name,"VShaderUtil001"));trace.push_back(6);return local?provider:0;}
 void warning(const char* message){check(state.incomingFactory==0&&!std::strcmp(message,"ShaderAPIDx10 was unable to access the required interfaces!\n"));trace.push_back(7);}
 void initializeMathLibrary(float a,float b,float c,int d,int e,int f,int g,int h){check(state.incomingFactory==0&&a==2.2f&&b==2.2f&&c==0&&d==2&&e==1&&f==1&&g==1&&h==1);trace.push_back(8);}
};
int main(){static_assert(sizeof(void*)==8,"x64");std::array<unsigned,2> objects{};uintptr_t provider=reinterpret_cast<uintptr_t>(&objects[0]),factory=reinterpret_cast<uintptr_t>(&objects[1]);check(provider>UINT32_MAX&&factory>UINT32_MAX);unsigned cases=0;std::ifstream input("outputs/native-base-startup.bin",std::ios::binary);
 for(bool cvar:{false,true})for(bool file:{false,true})for(bool external:{false,true})for(bool local:{false,true})for(bool self:{false,true})for(bool initialized:{false,true}){StudioDeviceManagerBaseStartupCandidate state;state.accessInitializationFlags=initialized?3:2;Ops ops{state,provider,factory,cvar,file,external,local,{}};bool result=studioDeviceManagerBaseConnectCandidate(state,self?provider:0,factory,ops);bool success=file&&self&&(external||local);std::vector<unsigned> expected{1};if(!initialized)expected.push_back(2);if(cvar)expected.push_back(3);expected.push_back(4);expected.push_back(5);if(!external)expected.push_back(6);expected.push_back(success?8:7);check(result==success&&ops.trace==expected&&state.accessInitializationFlags==3&&state.incomingFactory==0&&state.shaderUtil==((external||local)?provider:0)&&state.manager==(self?provider:0)&&state.fileSystem==(file?provider:0)&&state.cvar==(cvar?provider:0));check(ops.accessObject==(initialized?0x1111:provider));std::vector<unsigned> normalized;for(auto event:ops.trace)if(event!=2)normalized.push_back(event);std::array<unsigned,16> wire{},original{};wire[0]=static_cast<unsigned>(normalized.size());std::copy(normalized.begin(),normalized.end(),wire.begin()+1);wire[10]=state.shaderUtil?1:0;wire[11]=state.manager?1:0;wire[12]=state.accessInitializationFlags;wire[13]=static_cast<unsigned>(state.incomingFactory);wire[14]=unsigned(result);wire[15]=initialized?1:2;input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&wire==original);++cases;}
 check(input.peek()==std::char_traits<char>::eof());std::printf("x64_base_startup_sequence_cases=%u passed\n",cases);
}


