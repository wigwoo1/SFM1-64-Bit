#pragma once
#include "material_parameter_orchestration_native_fixture.h"
#include <deque>

// Private original caller/file-loader oracle. Real original KeyValues methods
// allocate, construct, mutate and destroy nodes; symbol/heap services remain
// explicit single-thread fixtures. The filesystem object is the real original.
struct MaterialFallbackNativeFixture:MaterialParameterOrchestrationNativeFixture {
    std::array<uintptr_t,10>kvMethods{};uintptr_t kvObject=0;
    std::map<std::string,int>ci;std::deque<std::string>cs;
    HMODULE fileModule=nullptr;void*filesystem=nullptr;
    std::vector<std::pair<const uint8_t*,std::vector<uint8_t>>>fileCode;
    std::array<uint8_t,0x80>errorMaterial{};
    std::string ownerName="fixture/root";
    unsigned nullFrees=0;
    static MaterialFallbackNativeFixture&self(){return *static_cast<MaterialFallbackNativeFixture*>(active);}
    static void* __cdecl kv(){return &self().kvObject;}
    static void* __fastcall kvAlloc(void*,void*,int bytes){if(bytes!=36)ExitProcess(170);return allocate(nullptr,nullptr,36);}
    static void __fastcall kvFree(void*,void*,void*p){free(nullptr,nullptr,p);}
    static void __fastcall heapFree(void*,void*,void*p){if(!p){++self().nullFrees;return;}free(nullptr,nullptr,p);}
    static int __fastcall ciSymbol(void*,void*,const char*name,bool create){auto&f=self();auto s=lower(name?name:"");auto i=f.ci.find(s);if(i!=f.ci.end())return i->second;if(!create)return -1;const auto id=int(f.ci.size()+1);f.ci[s]=id;return id;}
    static int __fastcall csSymbol(void*,void*,int*insensitive,const char*name,bool create){if(!name){*insensitive=-1;return -1;}auto&f=self();*insensitive=ciSymbol(nullptr,nullptr,name,create);for(size_t i=0;i<f.cs.size();++i)if(f.cs[i]==name)return int(i);if(!create)return -1;f.cs.emplace_back(name);return int(f.cs.size()-1);}
    static const char* __fastcall csName(void*,void*,int id){auto&f=self();if(id<0||size_t(id)>=f.cs.size())ExitProcess(171);return f.cs[size_t(id)].c_str();}
    static const char* __fastcall name(void*,void*){return self().ownerName.c_str();}
    void resolveWindowsImports(const char*path){
        // Resolve unchanged KERNEL32 IAT entries as the Windows loader would.
        // Retain the previously verified explicit TLS/error service bindings.
        std::ifstream in(path,std::ios::binary);std::vector<uint8_t>raw((std::istreambuf_iterator<char>(in)),{});if(raw.size()<sizeof(IMAGE_DOS_HEADER))throw std::runtime_error("Import PE header");
        auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(raw.data());if(dos->e_lfanew<0||size_t(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>raw.size())throw std::runtime_error("Import PE extent");auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(raw.data()+dos->e_lfanew);auto imports=reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(image.base+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        for(;imports->Name;++imports)if(!_stricmp(reinterpret_cast<const char*>(image.base+imports->Name),"KERNEL32.dll")){
            auto source=reinterpret_cast<const uint32_t*>(image.base+imports->OriginalFirstThunk);auto target=reinterpret_cast<uint32_t*>(image.base+imports->FirstThunk);
            for(size_t i=0;source[i];++i)if(target[i]==source[i]){if(source[i]&IMAGE_ORDINAL_FLAG32)throw std::runtime_error("Unexpected ordinal kernel import");auto name=reinterpret_cast<const char*>(image.base+source[i]+2);auto proc=GetProcAddress(GetModuleHandleA("KERNEL32.dll"),name);if(!proc)throw std::runtime_error("Kernel import missing");target[i]=reinterpret_cast<uintptr_t>(proc);}
        }
    }
    explicit MaterialFallbackNativeFixture(const char*material,const char*directory,const char*mount):MaterialParameterOrchestrationNativeFixture(material,directory){
        resolveWindowsImports(material);
        // Original DLL static constructors for escaped and literal quoted-token
        // conversion tables. No table bytes are synthesized or patched here.
        // Without these, the raw private image consumes zero quote characters.
        using Initialize=void(__cdecl*)();reinterpret_cast<Initialize>(0x10094370)();reinterpret_cast<Initialize>(0x100943d0)();
        uint32_t escapedLength=0,literalLength=0;std::memcpy(&escapedLength,image.base+0x1348d8+12,4);std::memcpy(&literalLength,image.base+0x1352f0+12,4);
        if(escapedLength!=1||literalLength!=1)throw std::runtime_error("Original quoted-token initialization");
        kvMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));kvMethods[1]=reinterpret_cast<uintptr_t>(&kvAlloc);kvMethods[2]=reinterpret_cast<uintptr_t>(&kvFree);kvMethods[3]=reinterpret_cast<uintptr_t>(&ciSymbol);kvMethods[4]=reinterpret_cast<uintptr_t>(&csName);kvMethods[9]=reinterpret_cast<uintptr_t>(&csSymbol);kvObject=reinterpret_cast<uintptr_t>(kvMethods.data());word(0x95208,reinterpret_cast<uintptr_t>(&kv));ownerMethods[0]=reinterpret_cast<uintptr_t>(&name);allocators[5]=reinterpret_cast<uintptr_t>(&heapFree);
        fileModule=LoadLibraryExA((std::string(directory)+"\\filesystem_stdio.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!fileModule)throw std::runtime_error("Original filesystem load");
        using Factory=void*(__cdecl*)(const char*,int*);auto factory=reinterpret_cast<Factory>(GetProcAddress(fileModule,"CreateInterface"));int status=-1;filesystem=factory?factory("VFileSystem017",&status):nullptr;if(!filesystem||status)throw std::runtime_error("Original filesystem object");
        auto base=reinterpret_cast<const uint8_t*>(fileModule);auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto&s=IMAGE_FIRST_SECTION(nt)[i];if(s.Characteristics&IMAGE_SCN_MEM_EXECUTE)fileCode.push_back({base+s.VirtualAddress,{base+s.VirtualAddress,base+s.VirtualAddress+s.Misc.VirtualSize}});}
        auto methods=*static_cast<void***>(filesystem);using Add=void(__thiscall*)(void*,const char*,const char*,int);reinterpret_cast<Add>(methods[10])(filesystem,mount,"GAME",1);word(0x1255c4,reinterpret_cast<uintptr_t>(filesystem));
        word(0x1059ec,reinterpret_cast<uintptr_t>(errorMaterial.data()));
    }
    void*build(const SourceMaterialPatchNodeCandidate&n){
        void*p=allocate(nullptr,nullptr,36);std::memset(p,0,36);using Construct=void*(__thiscall*)(void*,const char*);reinterpret_cast<Construct>(0x10070a10)(p,n.key.c_str());
        if(n.kind==1){using Set=void(__thiscall*)(void*,const char*,const char*);reinterpret_cast<Set>(0x10070db0)(p,nullptr,n.text.c_str());}
        else if(n.kind==2){using Set=void(__thiscall*)(void*,const char*,uint32_t);reinterpret_cast<Set>(0x10070de0)(p,nullptr,n.bits);}
        else if(n.kind==3){float f;std::memcpy(&f,&n.bits,4);using Set=void(__thiscall*)(void*,const char*,float);reinterpret_cast<Set>(0x10070e00)(p,nullptr,f);}
        else if(n.kind)throw std::runtime_error("Initial node kind unsupported");
        using Append=void(__thiscall*)(void*,void*);for(const auto&c:n.children)reinterpret_cast<Append>(0x10070c00)(p,build(c));return p;
    }
    void destroyTree(void*p){using Destroy=void(__thiscall*)(void*);reinterpret_cast<Destroy>(0x10071280)(p);}
    SourceMaterialPatchNodeCandidate extract(void*p,unsigned depth=0){
        if(depth>16)throw std::runtime_error("Native final tree extent");SourceMaterialPatchNodeCandidate n;using Name=const char*(__thiscall*)(void*);n.key=reinterpret_cast<Name>(0x100705f0)(p);auto*b=static_cast<uint8_t*>(p);n.kind=b[16];
        uintptr_t data=0;if(n.kind==1){std::memcpy(&data,b+4,4);n.text=data?reinterpret_cast<const char*>(data):"";}else if(n.kind==2||n.kind==3)std::memcpy(&n.bits,b+12,4);else if(n.kind==7){std::memcpy(&data,b+4,4);std::memcpy(&n.pointer,reinterpret_cast<void*>(data),8);}else if(n.kind)throw std::runtime_error("Native result kind unsupported");
        void*child=nullptr;std::memcpy(&child,b+24,4);while(child){n.children.push_back(extract(child,depth+1));std::memcpy(&child,static_cast<uint8_t*>(child)+20,4);}return n;
    }
    void*runTree(void*root,void*aggregate,void*error,const MaterialParameterCollectionEnvironmentCandidate&env,const VertexLitInitSettingsCandidate&s,int32_t hardwareLevel,bool prefix){
        MaterialParameterCollectionNativeFixture::prepare(env,{}, {},true);material.fill(0);const auto table=reinterpret_cast<uintptr_t>(ownerMethods.data());std::memcpy(material.data(),&table,4);material[0x20]=prefix?0x20:0;
        context.fill(0);emptyLog.fill(0);const auto systemTable=reinterpret_cast<uintptr_t>(systemMethods.data()),storage=reinterpret_cast<uintptr_t>(emptyLog.data());const uint32_t cap=8;std::memcpy(context.data(),&systemTable,4);std::memcpy(context.data()+0x20,&storage,4);std::memcpy(context.data()+0x24,&cap,4);context[0x35]=8;word(0x1017f8,reinterpret_cast<uintptr_t>(context.data()));
        config.fill(0);phong.fill(0);fog.fill(0);config[13]=s.configFlagsAt34;phong[12]=uint32_t(s.matPhong);fog[12]=uint32_t(s.forceVertexFog);level=hardwareLevel;tools=s.toolsSwitchPresent;stdBase[0xbebdc]=0;stdBase[0xbebd8]=0xa5;events.clear();lookups.clear();iterations=0;shaderWarnings=0;
        const auto e=reinterpret_cast<uintptr_t>(error);std::memcpy(errorMaterial.data()+0x7c,&e,4);using Loop=void*(__thiscall*)(void*,void*,void*);auto selected=reinterpret_cast<Loop>(0x10006f10)(material.data(),root,aggregate);if(leaseDepth)ExitProcess(172);verifyCode();return selected;
    }
    void verifyCode(){image.verify();for(const auto&r:stdCode)if(std::memcmp(r.first,r.second.data(),r.second.size()))ExitProcess(173);for(const auto&r:fileCode)if(std::memcmp(r.first,r.second.data(),r.second.size()))ExitProcess(174);}
    ~MaterialFallbackNativeFixture(){if(filesystem){using Remove=void(__thiscall*)(void*);reinterpret_cast<Remove>((*static_cast<void***>(filesystem))[12])(filesystem);}if(fileModule)FreeLibrary(fileModule);}
};
