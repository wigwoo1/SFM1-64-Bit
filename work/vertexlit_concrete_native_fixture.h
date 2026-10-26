#pragma once
#include "material_parameter_completion_native_fixture.h"

// Real ShaderDLL004 registry/object/descriptor/InitParams methods, with normal
// Windows relocation. No original instruction bytes patched or embedded.
// Only explicit config/hardware/command-line and warning service data is set.
struct VertexLitConcreteNativeFixture:MaterialParameterCompletionNativeFixture {
    HMODULE tierModule=nullptr,shaderModule=nullptr;
    uint8_t*stdBase=nullptr;void*shaderObject=nullptr;
    std::vector<std::pair<const uint8_t*,std::vector<uint8_t>>>stdCode;
    std::array<uint32_t,32>config{},phong{},fog{};
    std::array<uintptr_t,18>levelMethods{};uintptr_t levelObject=0;
    std::array<uintptr_t,4>commandMethods{};uintptr_t commandObject=0;
    int32_t level=92;bool tools=false,seenTools=false;uint32_t shaderWarnings=0;
    std::vector<uint32_t>types;
    static VertexLitConcreteNativeFixture&self(){return *static_cast<VertexLitConcreteNativeFixture*>(active);}
    static int32_t __fastcall getLevel(void*,void*){return self().level;}
    static void* __cdecl commandLine(){return &self().commandObject;}
    static int __fastcall checkTools(void*,void*,const char*key,void*value){auto&f=self();if(!key||std::strcmp(key,"-tools")||value)ExitProcess(130);f.seenTools=true;return f.tools?7:0;}
    static void __cdecl stdWarning(const char*format,...){
        auto&f=self();if(format!=reinterpret_cast<const char*>(f.stdBase+0x9bdd0)&&format!=reinterpret_cast<const char*>(f.stdBase+0x9bd78))ExitProcess(131);
        ++f.shaderWarnings;
    }
    void stdWord(size_t offset,uintptr_t value){std::memcpy(stdBase+offset,&value,4);}
    void import(size_t offset,uintptr_t value){
        DWORD old=0,ignored=0;if(!VirtualProtect(stdBase+offset,4,PAGE_READWRITE,&old))throw std::runtime_error("Fixture IAT protection");
        stdWord(offset,value);if(!VirtualProtect(stdBase+offset,4,old,&ignored))throw std::runtime_error("Fixture IAT restore");
    }
    uintptr_t relative(const void*p)const{
        const auto a=reinterpret_cast<uintptr_t>(p),b=reinterpret_cast<uintptr_t>(stdBase);
        const auto*dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(stdBase);const auto*nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(stdBase+dos->e_lfanew);
        if(a<b||a-b>=nt->OptionalHeader.SizeOfImage)throw std::runtime_error("Shader pointer bound");return a-b;
    }
    explicit VertexLitConcreteNativeFixture(const char*materialPath,const char*dir):MaterialParameterCompletionNativeFixture(materialPath){
        if(!SetDllDirectoryA(dir))throw std::runtime_error("Private DLL directory");
        tierModule=LoadLibraryExA((std::string(dir)+"\\tier0.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
        shaderModule=LoadLibraryExA((std::string(dir)+"\\stdshader_dx9.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
        if(!tierModule||!shaderModule)throw std::runtime_error("Normal private shader DLL load");stdBase=reinterpret_cast<uint8_t*>(shaderModule);
        const auto*dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(stdBase);const auto*nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(stdBase+dos->e_lfanew);
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto&s=IMAGE_FIRST_SECTION(nt)[i];if(s.Characteristics&IMAGE_SCN_MEM_EXECUTE){const auto*b=stdBase+s.VirtualAddress;stdCode.push_back({b,{b,b+s.Misc.VirtualSize}});}}
        using Factory=void*(__cdecl*)(const char*,int*);auto factory=reinterpret_cast<Factory>(GetProcAddress(shaderModule,"CreateInterface"));if(!factory)throw std::runtime_error("Real shader factory");
        int status=-1;void*registry=factory("ShaderDLL004",&status);if(!registry||status)throw std::runtime_error("ShaderDLL004");auto methods=*static_cast<void***>(registry);
        using Count=int(__thiscall*)(void*);using Get=void*(__thiscall*)(void*,int);using Name=const char*(__thiscall*)(void*);
        const int n=reinterpret_cast<Count>(methods[2])(registry);if(n!=157)throw std::runtime_error("Registry inventory");
        for(int i=0;i<n;++i){void*registeredObject=reinterpret_cast<Get>(methods[3])(registry,i);auto table=*static_cast<void***>(registeredObject);if(!std::strcmp(reinterpret_cast<Name>(table[0])(registeredObject),"VertexLitGeneric")){shaderObject=registeredObject;break;}}
        if(!shaderObject)throw std::runtime_error("Actual VertexLitGeneric object");auto table=*static_cast<void***>(shaderObject);
        if(relative(table[4])!=0x64a30||relative(table[12])!=0x53420||reinterpret_cast<Count>(table[2])(shaderObject)!=125)throw std::runtime_error("Concrete shader entry/schema");
        parameters.clear();for(int i=0;i<125;++i){const auto*p=static_cast<const uint32_t*>(reinterpret_cast<Get>(table[3])(shaderObject,i));relative(p);relative(reinterpret_cast<void*>(p[0]));parameters.emplace_back(reinterpret_cast<const char*>(p[0]));types.push_back(p[2]);}
        levelMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));levelMethods[17]=reinterpret_cast<uintptr_t>(&getLevel);levelObject=reinterpret_cast<uintptr_t>(levelMethods.data());
        commandMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));commandMethods[3]=reinterpret_cast<uintptr_t>(&checkTools);commandObject=reinterpret_cast<uintptr_t>(commandMethods.data());
        stdWord(0xdb9dc,reinterpret_cast<uintptr_t>(config.data()));stdWord(0xcaff4,reinterpret_cast<uintptr_t>(phong.data()));stdWord(0xcb054,reinterpret_cast<uintptr_t>(fog.data()));stdWord(0xdb9d8,reinterpret_cast<uintptr_t>(&levelObject));
        import(0x890fc,reinterpret_cast<uintptr_t>(&commandLine));import(0x890f8,reinterpret_cast<uintptr_t>(&stdWarning));
    }
    uint32_t completeActual(void**params,const VertexLitInitSettingsCandidate&s,int32_t hardwareLevel){
        config.fill(0);phong.fill(0);fog.fill(0);config[13]=s.configFlagsAt34;phong[12]=uint32_t(s.matPhong);fog[12]=uint32_t(s.forceVertexFog);level=hardwareLevel;tools=s.toolsSwitchPresent;seenTools=false;shaderWarnings=0;
        stdBase[0xbebdc]=0;stdBase[0xbebd8]=0xa5;const auto c=config,p=phong,f=fog;
        context.fill(0);emptyLog.fill(0);const uintptr_t storage=reinterpret_cast<uintptr_t>(emptyLog.data());const uint32_t capacity=8;
        std::memcpy(context.data()+0x20,&storage,4);std::memcpy(context.data()+0x24,&capacity,4);context[0x35]=8;events.clear();
        using Complete=void(__thiscall*)(void*,void*,void**,const char*);
        reinterpret_cast<Complete>(0x10036bb0)(context.data(),shaderObject,params,"$concrete_fixture");++stageCalls;
        uint32_t current=1,shaderParams=1;std::memcpy(&current,context.data()+0x54,4);std::memcpy(&shaderParams,stdBase+0xdb944,4);
        if(current||shaderParams||config!=c||phong!=p||fog!=f||events!=std::vector<unsigned>{1,2,3,5})ExitProcess(132);
        for(const auto&r:stdCode)if(std::memcmp(r.first,r.second.data(),r.second.size()))ExitProcess(133);
        return seenTools?1u:0u;
    }
    ~VertexLitConcreteNativeFixture(){if(shaderModule)FreeLibrary(shaderModule);if(tierModule)FreeLibrary(tierModule);SetDllDirectoryA(nullptr);}
};
