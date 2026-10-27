#pragma once
#include "material_object_parameters_native_fixture.h"
#include "lightmapped_generic_init_candidate.h"
// Test-only services at the newly reached LightmappedGeneric boundary. Every
// original executable section stays unchanged; InitParams is the real method.
struct LightmappedParametersNativeFixture:MaterialObjectParametersNativeFixture {
    explicit LightmappedParametersNativeFixture(const char*material,const char*directory,const char*mount):MaterialObjectParametersNativeFixture(material,directory,mount){
        bool found=false;using Count=int(__thiscall*)(void*);using Get=const uint32_t*(__thiscall*)(void*,int);
        const auto declaredTypes=lightmappedGenericParameterTypesCandidate();
        for(size_t i=0;i<shaders.size();++i)if(shaderNames[i]=="LightmappedGeneric"){
            found=true;auto*p=shaders[i];auto t=*static_cast<void***>(p);
            if(relative(t[4])!=0x64a30||relative(t[12])!=0x28290||reinterpret_cast<Count>(t[2])(p)!=68)throw std::runtime_error("Lightmapped registry method/schema identity");
            for(int j=0;j<68;++j){auto*d=reinterpret_cast<Get>(t[3])(p,j);relative(d);if(lightmappedGenericParameterNamesCandidate()[size_t(j)]!=reinterpret_cast<const char*>(d[0])||declaredTypes[size_t(j)]!=d[2])throw std::runtime_error("Lightmapped descriptor identity");}
        }
        if(!found)throw std::runtime_error("Registered LightmappedGeneric required");
    }
    std::array<uint32_t,32>lightFog{},lightwarp{},fancyBlend{};
    std::array<uintptr_t,7>lightInitMethods{};uintptr_t lightInitObject=0;
    uint32_t slot5=0,slot6=0;
    static LightmappedParametersNativeFixture&self(){return *static_cast<LightmappedParametersNativeFixture*>(active);}
    static uint32_t __fastcall init5(void*,void*){return self().slot5;}
    static uint32_t __fastcall init6(void*,void*){return self().slot6;}
    static void __cdecl lightWarning(const char*format,...){
        auto&f=self();if(format==reinterpret_cast<const char*>(f.stdBase+0x94000)){++f.shaderWarnings;return;}
        VertexLitConcreteNativeFixture::stdWarning(format);
    }
    void configureLightmapped(const LightmappedInitSettingsCandidate&s){
        lightFog.fill(0);lightwarp.fill(0);fancyBlend.fill(0);
        lightFog[12]=uint32_t(s.forceVertexFog);lightwarp[12]=uint32_t(s.disableLightwarp);fancyBlend[12]=uint32_t(s.disableFancyBlending);
        stdWord(0xc4394,reinterpret_cast<uintptr_t>(lightFog.data()));stdWord(0xc43ec,reinterpret_cast<uintptr_t>(lightwarp.data()));stdWord(0xc449c,reinterpret_cast<uintptr_t>(fancyBlend.data()));
        lightInitMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));lightInitMethods[5]=reinterpret_cast<uintptr_t>(&init5);lightInitMethods[6]=reinterpret_cast<uintptr_t>(&init6);
        lightInitObject=reinterpret_cast<uintptr_t>(lightInitMethods.data());stdWord(0xdb9e0,reinterpret_cast<uintptr_t>(&lightInitObject));
        slot5=s.shaderInitSlot5;slot6=s.shaderInitSlot6;reinterpret_cast<uint8_t*>(config.data())[0x5f]=s.paintEnabled?1u:0u;
        import(0x890f8,reinterpret_cast<uintptr_t>(&lightWarning));
    }
};
