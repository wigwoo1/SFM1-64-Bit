#pragma once
#include "material_typed_dispatch_native_fixture.h"
struct MaterialParameterCollectionNativeFixture:MaterialTypedDispatchNativeFixture {
    std::vector<std::string>nodeNames,parameters,flags;
    std::vector<std::array<uint8_t,32>>nodes;
    std::vector<std::array<uintptr_t,6>>descriptors;
    std::array<std::array<uint8_t,40>,1024>pool{};
    std::array<uintptr_t,89>ownerMethods{};uintptr_t owner=0;
    std::array<uintptr_t,43>hardwareMethods{};uintptr_t hardware=0;
    std::array<uintptr_t,11>flagMethods{};uintptr_t flagProvider=0;
    uintptr_t originalFlagProvider=0;
    std::array<uintptr_t,6>toolsMethods{};uintptr_t tools=0;
    std::array<uintptr_t,4>shaderMethods{};uintptr_t shader=0;
    std::array<uint8_t,0x34>lowFill{},gpu{},percentOverride{};
    uint32_t hdrState=0,srgbState=0,percentGateState=0,percentKeepState=0;
    unsigned notifications=0;
    static MaterialParameterCollectionNativeFixture&self(){return *static_cast<MaterialParameterCollectionNativeFixture*>(active);}
    static const char* __fastcall nodeName(void*,void*,uint32_t symbol){auto&f=self();if(symbol>=f.nodeNames.size())ExitProcess(114);++f.nameLookups;return f.nodeNames[symbol].c_str();}
    static const char* __fastcall materialName(void*,void*){return "$collection_fixture";}
    static void __fastcall notify(void*,void*,void*){++self().notifications;}
    static void* __fastcall noMaterial(void*,void*){return nullptr;}
    static uint32_t __fastcall hdr(void*,void*){return self().hdrState;}
    static uint32_t __fastcall srgb(void*,void*){return self().srgbState;}
    static uint32_t __fastcall percentGate(void*,void*){return self().percentGateState;}
    static uint32_t __fastcall percentKeep(void*,void*){return self().percentKeepState;}
    static const char* __fastcall flagName(void*,void*,int index){auto&f=self();if(index<0||size_t(index)>f.flags.size())ExitProcess(115);return size_t(index)==f.flags.size()?"":f.flags[size_t(index)].c_str();}
    static int __fastcall parameterCount(void*,void*){return int(self().parameters.size());}
    static void* __fastcall parameter(void*,void*,int index){auto&f=self();if(index<0||size_t(index)>=f.descriptors.size())ExitProcess(116);return f.descriptors[size_t(index)].data();}
    explicit MaterialParameterCollectionNativeFixture(const char*path):MaterialTypedDispatchNativeFixture(path){
        keyValuesServices[4]=reinterpret_cast<uintptr_t>(&nodeName);
        ownerMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));ownerMethods[0]=reinterpret_cast<uintptr_t>(&materialName);ownerMethods[88]=reinterpret_cast<uintptr_t>(&notify);owner=reinterpret_cast<uintptr_t>(ownerMethods.data());
        services[130]=reinterpret_cast<uintptr_t>(&noMaterial);services[126]=reinterpret_cast<uintptr_t>(&noMaterial);services[141]=reinterpret_cast<uintptr_t>(&percentKeep);
        hardwareMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));hardwareMethods[34]=reinterpret_cast<uintptr_t>(&hdr);hardwareMethods[42]=reinterpret_cast<uintptr_t>(&srgb);hardware=reinterpret_cast<uintptr_t>(hardwareMethods.data());word(0x1059f4,reinterpret_cast<uintptr_t>(&hardware));
        std::memcpy(&originalFlagProvider,image.base+0x1017f8,4);
        flagMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));flagMethods[10]=reinterpret_cast<uintptr_t>(&flagName);flagProvider=reinterpret_cast<uintptr_t>(flagMethods.data());word(0x1017f8,reinterpret_cast<uintptr_t>(&flagProvider));
        toolsMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));toolsMethods[5]=reinterpret_cast<uintptr_t>(&percentGate);tools=reinterpret_cast<uintptr_t>(toolsMethods.data());word(0x11b0a0,reinterpret_cast<uintptr_t>(&tools));
        shaderMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));shaderMethods[2]=reinterpret_cast<uintptr_t>(&parameterCount);shaderMethods[3]=reinterpret_cast<uintptr_t>(&parameter);shader=reinterpret_cast<uintptr_t>(shaderMethods.data());
        word(0x105914,reinterpret_cast<uintptr_t>(lowFill.data()));word(0x105a74,reinterpret_cast<uintptr_t>(gpu.data()));word(0x1057cc,reinterpret_cast<uintptr_t>(percentOverride.data()));
    }
    void prepare(const MaterialParameterCollectionEnvironmentCandidate&env,const std::vector<MaterialParameterInputCandidate>&base,const std::vector<MaterialParameterInputCandidate>&over,bool useOriginalFlags=false){
        word(0x1017f8,useOriginalFlags?originalFlagProvider:reinterpret_cast<uintptr_t>(&flagProvider));
        for(size_t i=0;i<pool.size();++i){pool[i].fill(0xa5);const uintptr_t next=i+1<pool.size()?reinterpret_cast<uintptr_t>(pool[i+1].data()):0;std::memcpy(pool[i].data(),&next,4);}
        std::memset(image.base+0x10db04,0,0x28);word(0x10db04,40);word(0x10db10,reinterpret_cast<uintptr_t>(pool[0].data()));
        word(0x13ff3c,env.numericSse2?1:0);word(0x13ff40,env.trigSse2?1:0);
        std::memcpy(lowFill.data()+0x30,&env.condition.lowFill,4);std::memcpy(gpu.data()+0x30,&env.condition.gpu,4);std::memcpy(percentOverride.data()+0x30,&env.percentOverride,4);
        hdrState=uint32_t(env.condition.hdr);srgbState=env.condition.srgbReturn;percentGateState=env.percentGateReturn;percentKeepState=env.percentKeepReturn;
        descriptors.resize(parameters.size());for(size_t i=0;i<parameters.size();++i){descriptors[i].fill(0);descriptors[i][0]=reinterpret_cast<uintptr_t>(parameters[i].c_str());}
        nodeNames.clear();nodes.resize(base.size()+over.size());
        for(size_t i=0;i<nodes.size();++i){
            const auto&n=i<base.size()?base[i]:over[i-base.size()];nodeNames.push_back(n.name);nodes[i].fill(0);
            // GetName uses the case-sensitive24-bit symbol split across byte3
            // and word18, not the separate case-insensitive symbol at bytes0..2.
            nodes[i][3]=uint8_t(i);const uint16_t high=uint16_t(i>>8);std::memcpy(nodes[i].data()+18,&high,2);
            const uintptr_t text=reinterpret_cast<uintptr_t>(n.text.c_str());std::memcpy(nodes[i].data()+4,&text,4);std::memcpy(nodes[i].data()+12,&n.bits,4);nodes[i][16]=uint8_t(n.type);
            const bool next=i+1<base.size()||(i>=base.size()&&i+1<nodes.size());const uintptr_t link=next?reinterpret_cast<uintptr_t>(nodes[i+1].data()):0;std::memcpy(nodes[i].data()+20,&link,4);
        }
        warnings=nameLookups=notifications=0;
    }
    void cleanup(void*p){using Destroy=void*(__thiscall*)(void*,unsigned);reinterpret_cast<Destroy>(0x10018e60)(p,1);}
};
