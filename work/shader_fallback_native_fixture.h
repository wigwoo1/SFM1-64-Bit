#pragma once
#include "vertexlit_shared_init_candidate.h"
#include "vertexlit_concrete_native_fixture.h"
struct ShaderFallbackNativeFixture:VertexLitConcreteNativeFixture {
    std::vector<void*>registeredShaders;
    std::array<uintptr_t,35>fallbackHardwareMethods{};uintptr_t fallbackHardware=0;
    ShaderFallbackEnvironmentCandidate environment;
    std::vector<uint32_t>hardwareQueries;size_t levelCalls=0;
    static ShaderFallbackNativeFixture&self(){return *static_cast<ShaderFallbackNativeFixture*>(active);}
    static int32_t __fastcall hardware17(void*,void*){auto&f=self();if(f.levelCalls>=2)ExitProcess(140);f.hardwareQueries.push_back(17);return f.environment.level17[f.levelCalls++];}
    static int32_t __fastcall hardware24(void*,void*){auto&f=self();f.hardwareQueries.push_back(24);return f.environment.capability24;}
    static uint32_t __fastcall hardware34(void*,void*){auto&f=self();f.hardwareQueries.push_back(34);return f.environment.return34;}
    ShaderFallbackNativeFixture(const char*materialPath,const char*dir):VertexLitConcreteNativeFixture(materialPath,dir){
        using Factory=void*(__cdecl*)(const char*,int*);auto factory=reinterpret_cast<Factory>(GetProcAddress(shaderModule,"CreateInterface"));
        int status=-1;void*registry=factory("ShaderDLL004",&status);if(!registry||status)throw std::runtime_error("Fallback original registry");auto methods=*static_cast<void***>(registry);
        using Count=int(__thiscall*)(void*);using Get=void*(__thiscall*)(void*,int);using Name=const char*(__thiscall*)(void*);
        if(reinterpret_cast<Count>(methods[2])(registry)!=157)throw std::runtime_error("Fallback registry count");
        for(size_t i=0;i<157;++i){void*p=reinterpret_cast<Get>(methods[3])(registry,int(i));auto table=*static_cast<void***>(p);relative(table[1]);if(shaderFallbackEntriesCandidate()[i].name!=reinterpret_cast<Name>(table[0])(p))throw std::runtime_error("Fallback registered name order");registeredShaders.push_back(p);}
        fallbackHardwareMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));fallbackHardwareMethods[17]=reinterpret_cast<uintptr_t>(&hardware17);fallbackHardwareMethods[24]=reinterpret_cast<uintptr_t>(&hardware24);fallbackHardwareMethods[34]=reinterpret_cast<uintptr_t>(&hardware34);
        fallbackHardware=reinterpret_cast<uintptr_t>(fallbackHardwareMethods.data());stdWord(0xdb9d8,reinterpret_cast<uintptr_t>(&fallbackHardware));
    }
    ShaderFallbackDecisionCandidate decide(size_t index,void**params,const ShaderFallbackEnvironmentCandidate&env){
        environment=env;hardwareQueries.clear();levelCalls=0;config.fill(0);reinterpret_cast<uint8_t*>(config.data())[0x38]=env.configByte38;
        stdWord(0xdb9dc,env.configPresent?reinterpret_cast<uintptr_t>(config.data()):0);const auto beforeConfig=config;
        using Fallback=const char*(__thiscall*)(void*,void**);void*p=registeredShaders.at(index);auto table=*static_cast<void***>(p);
        const char*name=reinterpret_cast<Fallback>(table[1])(p,params);ShaderFallbackDecisionCandidate out;out.hardwareQueries=hardwareQueries;
        if(name){relative(name);size_t n=0;for(;n<1024&&name[n];++n){relative(name+n);if(static_cast<unsigned char>(name[n])>127)ExitProcess(141);}if(n==1024)ExitProcess(142);out.fallback=std::string(name,n);}
        if(config!=beforeConfig)ExitProcess(143);for(const auto&r:stdCode)if(std::memcmp(r.first,r.second.data(),r.second.size()))ExitProcess(144);
        return out;
    }
};
