#pragma once
#include "lightmapped_parameters_native_fixture.h"
#include "world_transition_init_candidate.h"
struct WorldTransitionNativeFixture:LightmappedParametersNativeFixture {
    explicit WorldTransitionNativeFixture(const char*material,const char*directory,const char*mount):LightmappedParametersNativeFixture(material,directory,mount){
        unsigned found=0;using Count=int(__thiscall*)(void*);using Get=const uint32_t*(__thiscall*)(void*,int);using Fallback=const char*(__thiscall*)(void*,void**);
        const auto declaredTypes=worldTransitionParameterTypesCandidate();
        for(size_t i=0;i<shaders.size();++i)if(shaderNames[i]=="WorldVertexTransition"||shaderNames[i]=="WorldVertexTransition_DX9"){
            ++found;bool alias=shaderNames[i]=="WorldVertexTransition";auto*p=shaders[i];auto t=*static_cast<void***>(p);const int count=alias?13:50;
            if(relative(t[4])!=0x64a30||relative(t[12])!=(alias?0xf6e0u:0x61cb0u)||relative(t[1])!=(alias?0x61ac0u:0x41370u)||reinterpret_cast<Count>(t[2])(p)!=count)throw std::runtime_error("World transition method/schema identity");
            auto fallback=reinterpret_cast<Fallback>(t[1])(p,nullptr);if(alias?(!fallback||std::strcmp(fallback,"WorldVertexTransition_DX9")):fallback!=nullptr)throw std::runtime_error("World transition actual fallback identity");
            for(int j=0;j<count;++j){auto*d=reinterpret_cast<Get>(t[3])(p,j);relative(d);if(worldTransitionParameterNamesCandidate()[size_t(j)]!=reinterpret_cast<const char*>(d[0])||declaredTypes[size_t(j)]!=d[2])throw std::runtime_error("World transition actual descriptor");}
            if(!alias){
                // Functional semantic field order of the shared helper's info
                // carrier, not original instruction bytes or a padded var array.
                static constexpr std::array<size_t,60>fields{6,7,8,13,14,18,15,16,17,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,9,10,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67};
                std::array<int32_t,60>actual;actual.fill(-1);using Map=void(__thiscall*)(void*,void*);reinterpret_cast<Map>(stdBase+0x61b00)(p,actual.data());const auto map=worldTransitionSharedMappingCandidate();
                for(size_t j=0;j<fields.size();++j)if(actual[j]!=map[fields[j]])throw std::runtime_error("Actual50-variable mapping differs from owned semantic mapping");
            }
        }
        if(found!=2)throw std::runtime_error("World transition alias and concrete registry objects required");
    }
};
