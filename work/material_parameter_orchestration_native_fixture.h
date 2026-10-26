#pragma once
#include "vertexlit_shared_init_candidate.h"
#include "vertexlit_concrete_native_fixture.h"
#include "source_material_patch_policy_candidate.h"
#include <map>
struct MaterialParameterOrchestrationNativeFixture:VertexLitConcreteNativeFixture {
    std::array<uint8_t,0x90>material{};
    std::array<uintptr_t,15>systemMethods{};
    std::array<uintptr_t,11>commandLineMethods{};uintptr_t commandLineObject=0;
    std::vector<void*>shaders;std::vector<std::string>shaderNames,lookups;
    std::vector<std::array<uint8_t,36>>tree;
    std::vector<const SourceMaterialPatchNodeCandidate*>sourceNodes;
    std::map<std::string,uint32_t>symbols;
    unsigned leaseDepth=0,iterations=0;
    static MaterialParameterOrchestrationNativeFixture&self(){return *static_cast<MaterialParameterOrchestrationNativeFixture*>(active);}
    static std::string lower(std::string s){for(auto&c:s)if(c>='A'&&c<='Z')c=char(c+32);return s;}
    static int __fastcall symbol(void*,void*,const char*name,bool create){auto&f=self();auto k=lower(name?name:"");auto i=f.symbols.find(k);if(i!=f.symbols.end())return int(i->second);if(!create)return -1;if(f.symbols.size()>=0xffff)ExitProcess(150);auto id=uint32_t(f.symbols.size()+1);f.symbols[k]=id;return int(id);}
    static const char* __fastcall materialName(void*,void*){return "$loop_fixture";}
    static uint32_t __fastcall acquire(void*,void*){auto&f=self();if(f.leaseDepth++)ExitProcess(151);return 0x12344321;}
    static void __fastcall release(void*,void*,uint32_t token){auto&f=self();if(f.leaseDepth!=1||token!=0x12344321)ExitProcess(152);--f.leaseDepth;}
    static void* __fastcall findShader(void*,void*,const char*name){auto&f=self();if(++f.iterations>16)ExitProcess(153);f.lookups.emplace_back(name);auto key=lower(name);for(size_t i=0;i<f.shaders.size();++i)if(lower(f.shaderNames[i])==key)return f.shaders[i];return nullptr;}
    static int32_t __fastcall materialLevel(void*,void*){return self().level;}
    static void* __cdecl command(){return &self().commandLineObject;}
    static int __fastcall hush(void*,void*,const char*name){if(!name||std::strcmp(name,"-hushasserts"))ExitProcess(154);return 0;}
    explicit MaterialParameterOrchestrationNativeFixture(const char*path,const char*dir):VertexLitConcreteNativeFixture(path,dir){
        using Factory=void*(__cdecl*)(const char*,int*);auto factory=reinterpret_cast<Factory>(GetProcAddress(shaderModule,"CreateInterface"));int status=-1;void*registry=factory("ShaderDLL004",&status);if(!registry||status)throw std::runtime_error("Loop registry");auto methods=*static_cast<void***>(registry);
        using Count=int(__thiscall*)(void*);using Get=void*(__thiscall*)(void*,int);using Name=const char*(__thiscall*)(void*);
        for(int i=0;i<reinterpret_cast<Count>(methods[2])(registry);++i){void*p=reinterpret_cast<Get>(methods[3])(registry,i);auto t=*static_cast<void***>(p);shaders.push_back(p);shaderNames.emplace_back(reinterpret_cast<Name>(t[0])(p));}
        ownerMethods[0]=reinterpret_cast<uintptr_t>(&materialName);keyValuesServices[3]=reinterpret_cast<uintptr_t>(&symbol);
        services[106]=reinterpret_cast<uintptr_t>(&acquire);services[107]=reinterpret_cast<uintptr_t>(&release);hardwareMethods[17]=reinterpret_cast<uintptr_t>(&materialLevel);
        systemMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));systemMethods[9]=reinterpret_cast<uintptr_t>(&findShader);systemMethods[10]=0x10036a40;systemMethods[14]=0x10036bb0;
        commandLineMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));commandLineMethods[10]=reinterpret_cast<uintptr_t>(&hush);commandLineObject=reinterpret_cast<uintptr_t>(commandLineMethods.data());word(0x951e0,reinterpret_cast<uintptr_t>(&command));
    }
    size_t buildNode(const SourceMaterialPatchNodeCandidate&source){
        if(tree.size()>=4096)ExitProcess(155);const size_t index=tree.size();tree.emplace_back();tree.back().fill(0);sourceNodes.push_back(&source);nodeNames.push_back(source.key);
        const auto key=lower(source.key);auto found=symbols.find(key);const uint32_t ci=found==symbols.end()?uint32_t(symbols.size()+1):found->second;symbols[key]=ci;
        auto*b=tree[index].data();std::memcpy(b,&ci,3);b[3]=uint8_t(index);const uint16_t hi=uint16_t(index>>8);std::memcpy(b+18,&hi,2);b[16]=uint8_t(source.kind);
        const uintptr_t text=reinterpret_cast<uintptr_t>(source.text.c_str());std::memcpy(b+4,&text,4);std::memcpy(b+12,&source.bits,4);
        size_t previous=size_t(-1);
        for(const auto&child:source.children){const auto id=buildNode(child);const auto ptr=reinterpret_cast<uintptr_t>(tree[id].data());if(previous==size_t(-1))std::memcpy(tree[index].data()+24,&ptr,4);else std::memcpy(tree[previous].data()+20,&ptr,4);previous=id;}
        return index;
    }
    void*run(const SourceMaterialPatchNodeCandidate&source,const MaterialParameterCollectionEnvironmentCandidate&env,
            const VertexLitInitSettingsCandidate&settings,int32_t hardwareLevel){
        // Reuse accepted pool/name/owner services; no fake shader initializer,
        // variable setter, condition-block selector or fallback decision.
        MaterialParameterCollectionNativeFixture::prepare(env,{}, {},true);tree.clear();tree.reserve(4096);sourceNodes.clear();sourceNodes.reserve(4096);nodeNames.clear();symbols.clear();buildNode(source);
        material.fill(0);const auto methods=reinterpret_cast<uintptr_t>(ownerMethods.data());std::memcpy(material.data(),&methods,4);
        context.fill(0);emptyLog.fill(0);const auto systemTable=reinterpret_cast<uintptr_t>(systemMethods.data()),storage=reinterpret_cast<uintptr_t>(emptyLog.data());const uint32_t cap=8;
        std::memcpy(context.data(),&systemTable,4);std::memcpy(context.data()+0x20,&storage,4);std::memcpy(context.data()+0x24,&cap,4);context[0x35]=8;
        word(0x1017f8,reinterpret_cast<uintptr_t>(context.data()));
        config.fill(0);phong.fill(0);fog.fill(0);config[13]=settings.configFlagsAt34;phong[12]=uint32_t(settings.matPhong);fog[12]=uint32_t(settings.forceVertexFog);level=hardwareLevel;tools=settings.toolsSwitchPresent;stdBase[0xbebdc]=0;stdBase[0xbebd8]=0xa5;
        events.clear();lookups.clear();iterations=0;shaderWarnings=0;
        using Loop=void*(__thiscall*)(void*,void*,void*);void*selected=reinterpret_cast<Loop>(0x10006f10)(material.data(),tree[0].data(),nullptr);
        if(leaseDepth)ExitProcess(156);for(const auto&r:stdCode)if(std::memcmp(r.first,r.second.data(),r.second.size()))ExitProcess(157);image.verify();return selected;
    }
    void cleanupResult(){
        uintptr_t array=0;std::memcpy(&array,material.data()+0x24,4);const auto n=material[0x22];
        if(array){auto**vars=reinterpret_cast<void**>(array);for(unsigned i=0;i<n;++i)cleanup(vars[i]);MaterialConstructionNativeFixture::free(nullptr,nullptr,reinterpret_cast<void*>(array));}
        if(*reinterpret_cast<uint32_t*>(image.base+0x10db14))ExitProcess(158);
    }
};
