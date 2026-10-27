#pragma once
#include "lightmapped_parameters_native_fixture.h"
#include "unlit_two_texture_init_candidate.h"
struct UnlitTwoTextureNativeFixture:LightmappedParametersNativeFixture {
    explicit UnlitTwoTextureNativeFixture(const char*material,const char*directory,const char*mount):LightmappedParametersNativeFixture(material,directory,mount){
        unsigned found=0;using Count=int(__thiscall*)(void*);using Get=const uint32_t*(__thiscall*)(void*,int);using Fallback=const char*(__thiscall*)(void*,void**);const auto declaredTypes=unlitTwoTextureParameterTypesCandidate();
        for(size_t i=0;i<shaders.size();++i)if(shaderNames[i]=="UnlitTwoTexture"||shaderNames[i]=="UnlitTwoTexture_DX9"){
            ++found;const bool alias=shaderNames[i]=="UnlitTwoTexture";auto*p=shaders[i];auto t=*static_cast<void***>(p);const int count=alias?13:20;
            if(relative(t[4])!=0x64a30||relative(t[12])!=(alias?0xf6e0u:0x4cdb0u)||relative(t[1])!=(alias?0x4ccc0u:0x41370u)||reinterpret_cast<Count>(t[2])(p)!=count)throw std::runtime_error("UnlitTwoTexture method/schema identity");
            auto fallback=reinterpret_cast<Fallback>(t[1])(p,nullptr);if(alias?(!fallback||std::strcmp(fallback,"UnlitTwoTexture_DX9")):fallback!=nullptr)throw std::runtime_error("UnlitTwoTexture fallback identity");
            for(int j=0;j<count;++j){auto*d=reinterpret_cast<Get>(t[3])(p,j);relative(d);if(unlitTwoTextureParameterNamesCandidate()[size_t(j)]!=reinterpret_cast<const char*>(d[0])||declaredTypes[size_t(j)]!=d[2])throw std::runtime_error("UnlitTwoTexture actual descriptor");}
        }
        if(found!=2)throw std::runtime_error("UnlitTwoTexture alias/concrete registry objects required");
        for(const auto&entry:std::array<std::pair<size_t,uint32_t>,4>{{{0xc9358,16},{0xc93d0,17},{0xc9340,18},{0xc93b8,19}}}){uint32_t actual;std::memcpy(&actual,stdBase+entry.first,4);if(actual!=entry.second)throw std::runtime_error("Original cloak field mapping");}
    }
};
