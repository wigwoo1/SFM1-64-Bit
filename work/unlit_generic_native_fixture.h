#pragma once
#include "vertexlit_concrete_native_fixture.h"

// Normal private ShaderDLL004 registry. Reuse the accepted variable/allocator/
// InitParams surrounding-stage services, selecting the actual UnlitGeneric
// object and its own descriptors. No shader or variable methods are replaced.
struct UnlitGenericNativeFixture:VertexLitConcreteNativeFixture {
    std::array<int32_t,110>mapping{};
    explicit UnlitGenericNativeFixture(const char*material,const char*directory):VertexLitConcreteNativeFixture(material,directory){
        using Factory=void*(__cdecl*)(const char*,int*);using Count=int(__thiscall*)(void*);
        using Get=void*(__thiscall*)(void*,int);using Name=const char*(__thiscall*)(void*);
        auto factory=reinterpret_cast<Factory>(GetProcAddress(shaderModule,"CreateInterface"));int status=-1;
        auto registry=factory("ShaderDLL004",&status);if(!registry||status)throw std::runtime_error("Unlit registry");
        auto methods=*static_cast<void***>(registry);shaderObject=nullptr;
        for(int i=0;i<reinterpret_cast<Count>(methods[2])(registry);++i){
            auto registered=reinterpret_cast<Get>(methods[3])(registry,i);auto table=*static_cast<void***>(registered);
            if(!std::strcmp(reinterpret_cast<Name>(table[0])(registered),"UnlitGeneric")){if(shaderObject)throw std::runtime_error("Duplicate UnlitGeneric");shaderObject=registered;}
        }
        if(!shaderObject)throw std::runtime_error("UnlitGeneric object");auto table=*static_cast<void***>(shaderObject);
        if(reinterpret_cast<Count>(table[2])(shaderObject)!=68||relative(table[4])!=0x64a30||relative(table[12])!=0x4ca60)throw std::runtime_error("UnlitGeneric descriptor/entry identity");
        parameters.clear();types.clear();
        for(int i=0;i<68;++i){const auto*p=static_cast<const uint32_t*>(reinterpret_cast<Get>(table[3])(shaderObject,i));relative(p);relative(reinterpret_cast<void*>(p[0]));parameters.emplace_back(reinterpret_cast<const char*>(p[0]));types.push_back(p[2]);}
        std::array<int32_t,112>guarded;guarded.fill(-1);guarded.front()=0x12345678;guarded.back()=0x23456789;
        using Map=void(__thiscall*)(void*,void*);reinterpret_cast<Map>(stdBase+0x4c7c0)(shaderObject,guarded.data()+1);
        if(guarded.front()!=0x12345678||guarded.back()!=0x23456789)throw std::runtime_error("Unlit mapping extent");
        std::copy(guarded.begin()+1,guarded.end()-1,mapping.begin());
        for(auto i:mapping)if(i<-1||i>=68)throw std::runtime_error("Unlit mapping parameter bound");
        for(const auto&r:stdCode)if(std::memcmp(r.first,r.second.data(),r.second.size()))throw std::runtime_error("Unlit mapping executable integrity");
    }
};
