#pragma once
// Private verification services only. Complete original condition function,
// name getter, ASCII comparison and prefix copy execute unchanged in RX code.
#include "material_construction_native_fixture.h"
struct MaterialParameterConditionNativeFixture:MaterialConstructionNativeFixture {
    std::string inputName;
    std::array<uintptr_t,43>hardwareMethods{};
    std::array<uintptr_t,1>ownerMethods{};
    uintptr_t hardware=0,owner=0;
    std::array<uint8_t,0x34>lowFill{},gpu{};
    uint32_t hdrReturn=0,srgbReturn=0;
    unsigned hdrCalls=0,srgbCalls=0,ownerNameCalls=0;
    static MaterialParameterConditionNativeFixture&self(){return *static_cast<MaterialParameterConditionNativeFixture*>(active);}
    static const char* __fastcall keyName(void*,void*,uint32_t symbol){
        auto&f=self();if(symbol)ExitProcess(113);++f.nameLookups;return f.inputName.c_str();
    }
    static uint32_t __fastcall hdr(void*,void*){auto&f=self();++f.hdrCalls;return f.hdrReturn;}
    static uint32_t __fastcall srgb(void*,void*){auto&f=self();++f.srgbCalls;return f.srgbReturn;}
    static const char* __fastcall ownerName(void*,void*){++self().ownerNameCalls;return "$condition_fixture";}
    explicit MaterialParameterConditionNativeFixture(const char*path):MaterialConstructionNativeFixture(path,true){
        keyValuesServices[4]=reinterpret_cast<uintptr_t>(&keyName);
        hardwareMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));
        hardwareMethods[34]=reinterpret_cast<uintptr_t>(&hdr);hardwareMethods[42]=reinterpret_cast<uintptr_t>(&srgb);
        hardware=reinterpret_cast<uintptr_t>(hardwareMethods.data());
        ownerMethods[0]=reinterpret_cast<uintptr_t>(&ownerName);owner=reinterpret_cast<uintptr_t>(ownerMethods.data());
        word(0x105914,reinterpret_cast<uintptr_t>(lowFill.data()));
        word(0x105a74,reinterpret_cast<uintptr_t>(gpu.data()));
        word(0x1059f4,reinterpret_cast<uintptr_t>(&hardware));
    }
    void prepare(uint32_t low,uint32_t hdrValue,uint32_t srgbValue,uint32_t gpuValue){
        std::memcpy(lowFill.data()+0x30,&low,4);std::memcpy(gpu.data()+0x30,&gpuValue,4);
        hdrReturn=hdrValue;srgbReturn=srgbValue;warnings=nameLookups=hdrCalls=srgbCalls=ownerNameCalls=0;
    }
};
