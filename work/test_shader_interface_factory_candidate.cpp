#include "studio_shader_interface_factory_candidate.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
struct Services{void* device;void* api;void* shadow;unsigned reads=0;void* shaderDevice(){++reads;return device;}void* shaderApi(){++reads;return api;}void* shaderShadow(){++reads;return shadow;}};
int main(){unsigned cases=0;for(unsigned mask=0;mask<8;++mask)for(const char* name:{"ShaderDevice002","shaderdevice002","SHADERDEVICE002","ShaderApi030","sHaDeRaPi030","ShaderShadow010","SHADERSHADOW010","ShaderDevice001","ShaderApi030x","","unknown"})for(bool output:{false,true}){
 Services s{mask&1?reinterpret_cast<void*>(uintptr_t{0x100001234ull}):nullptr,mask&2?reinterpret_cast<void*>(uintptr_t{0x200005678ull}):nullptr,mask&4?reinterpret_cast<void*>(uintptr_t{0x300009abcull}):nullptr};int status=99;
 const std::string n=name;void* expected=nullptr;bool recognized=false;
 if(n=="ShaderDevice002"||n=="shaderdevice002"||n=="SHADERDEVICE002"){expected=s.device;recognized=true;}
 else if(n=="ShaderApi030"||n=="sHaDeRaPi030"){expected=s.api;recognized=true;}
 else if(n=="ShaderShadow010"||n=="SHADERSHADOW010"){expected=s.shadow;recognized=true;}
 assert(studioShaderInterfaceFactoryCandidate(name,output?&status:nullptr,s)==expected);assert(s.reads==(recognized?1u:0u));assert(status==(output?(recognized?0:1):99));++cases;
 }
 Services s{};s.api=reinterpret_cast<void*>(uintptr_t{0x400001234ull});assert(studioShaderInterfaceFactoryCandidate("ShaderApi030",nullptr,s)==s.api);s.api=reinterpret_cast<void*>(uintptr_t{0x500005678ull});assert(studioShaderInterfaceFactoryCandidate("ShaderApi030",nullptr,s)==s.api);
 std::printf("shader_interface_factory_candidate_cases=%u passed\n",cases);
}
