#pragma once
#include <deque>
#include <set>
#include "native_private_world_image_fixture.h"
#include "material_proxy_evaluation_candidate.h"
// Private original client factory/Init/OnBind/Release execute unchanged RX.
// IMaterial/IMaterialVar and KV symbol/heap/TLS services are explicit spies;
// variable value operations are separately checked against materialsystem.dll.
struct MaterialEqualsClientNativeFixture {
    NativePrivateWorldImageFixture image;
    inline static MaterialEqualsClientNativeFixture*active=nullptr;
    struct Variable {std::array<uint8_t,40>raw{};MaterialTypedDispatchCandidate value;};
    struct Key {std::array<uint8_t,36>raw{};std::string text;};
    std::vector<Variable>variables;
    std::deque<Key>keys;std::vector<std::string>symbols;
    std::array<uintptr_t,32>varMethods{};std::array<uintptr_t,130>materialMethods{};
    std::array<uintptr_t,6>heapMethods{};std::array<uintptr_t,5>keyMethods{};
    uintptr_t material=0,heap=0,heapPointer=0,keySystem=0;
    std::array<uint8_t,0x214>thread{};
    std::set<void*>blocks;unsigned allocations=0,releases=0,alphaFlags=0;
    std::vector<MaterialProxyWriteCandidate>writes;
    static void __fastcall unexpected(void*,void*){ExitProcess(91);}
    static MaterialEqualsClientNativeFixture&self(){return *active;}
    void word(size_t offset,uintptr_t value){std::memcpy(image.base+offset,&value,4);}
    static void* __fastcall allocate(void*,void*,uint32_t n){auto&f=self();if(!n||n>4096)ExitProcess(92);auto*p=std::malloc(n);if(!p)ExitProcess(93);f.blocks.insert(p);++f.allocations;return p;}
    static void __fastcall release(void*,void*,void*p){auto&f=self();if(!p)return;if(f.blocks.erase(p)!=1)ExitProcess(94);++f.releases;std::free(p);}
    static void* __cdecl keyValues(){return &self().keySystem;}
    static int __fastcall symbol(void*,void*,const char*name,bool create){auto&f=self();auto s=sourceMaterialPatchLowerCandidate(name?name:"");auto i=std::find(f.symbols.begin(),f.symbols.end(),s);if(i!=f.symbols.end())return int(i-f.symbols.begin()+1);if(!create)return -1;f.symbols.push_back(s);return int(f.symbols.size());}
    static void* __stdcall tlsThread(DWORD){return self().thread.data();}
    static void* __stdcall getTls(DWORD index){if(index!=78)ExitProcess(95);return reinterpret_cast<void*>(&tlsThread);}
    static DWORD __stdcall getError(){return 0;}
    static void __stdcall setError(DWORD){}
    unsigned index(void*p){for(size_t i=0;i<variables.size();++i)if(variables[i].raw.data()==p)return unsigned(i);throw std::runtime_error("Unknown variable service receiver");}
    static void* __fastcall find(void*object,void*,const char*name,bool*found,bool){auto&f=self();if(object!=&f.material)ExitProcess(96);auto key=sourceMaterialPatchLowerCandidate(name?name:"");for(size_t i=f.variables.size();i>0;--i)if(sourceMaterialPatchLowerCandidate(f.variables[i-1].value.name)==key){if(found)*found=true;return f.variables[i-1].raw.data();}if(found)*found=false;return nullptr;}
    static void __fastcall alphaFlag(void*object,void*,uint32_t flag,bool enabled){auto&f=self();if(object!=&f.material||flag!=0x40000000||!enabled)ExitProcess(97);++f.alphaFlags;}
    static void* __fastcall owning(void*,void*){return &self().material;}
    static float __fastcall getFloat(void*p,void*){auto&f=self();float x;auto n=f.variables[f.index(p)].value.value.numeric.vector[0];std::memcpy(&x,&n,4);return x;}
    static uint32_t __fastcall getInt(void*p,void*){auto&f=self();return f.variables[f.index(p)].value.value.numeric.integer;}
    static int __fastcall getCount(void*p,void*){auto&f=self();return int((f.variables[f.index(p)].value.value.numeric.typeAndMetadata>>4)&7u);}
    static void __fastcall getVector(void*p,void*,void*out,int count){auto&f=self();if(count<1||count>4)ExitProcess(98);std::memcpy(out,f.variables[f.index(p)].value.value.numeric.vector.data(),size_t(count)*4);}
    void refresh(unsigned index){auto&v=variables.at(index);const auto table=reinterpret_cast<uintptr_t>(varMethods.data()),owner=reinterpret_cast<uintptr_t>(&material);std::memcpy(v.raw.data(),&table,4);std::memcpy(v.raw.data()+8,&v.value.value.numeric.integer,4);std::memcpy(v.raw.data()+12,v.value.value.numeric.vector.data(),16);std::memcpy(v.raw.data()+28,&v.value.value.numeric.typeAndMetadata,4);std::memcpy(v.raw.data()+32,&owner,4);}
    void apply(void*p,uint32_t op,std::array<uint32_t,4>args,uint32_t count=0){
        const auto i=index(p);auto&v=variables[i].value;
        bool ok=op==8?materialVectorComponentCandidate(v.value.numeric,count,args[0]):materialStringSetCandidate(v.value,op,args,count);
        if(!ok)throw std::runtime_error("Native service operation outside independently checked value domain");writes.push_back({i,op,count,args});refresh(i);
    }
    static void __fastcall setFloat(void*p,void*,uint32_t bits){self().apply(p,1,{bits,0,0,0});}
    static void __fastcall setInt(void*p,void*,uint32_t bits){self().apply(p,0,{bits,0,0,0});}
    static void __fastcall setVector(void*p,void*,const uint32_t*bits,int count){if(count<1||count>4)ExitProcess(99);std::array<uint32_t,4>a{};std::copy_n(bits,count,a.begin());self().apply(p,5,a,uint32_t(count));}
    static void __fastcall setComponent(void*p,void*,uint32_t bits,uint32_t component){self().apply(p,8,{bits,0,0,0},component);}
    explicit MaterialEqualsClientNativeFixture(const char*path):image(path,0x100000){
        if(active)throw std::runtime_error("One client fixture");active=this;
        varMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));materialMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));heapMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));keyMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));
        varMethods[3]=reinterpret_cast<uintptr_t>(&setFloat);varMethods[4]=reinterpret_cast<uintptr_t>(&setInt);varMethods[12]=reinterpret_cast<uintptr_t>(&setVector);varMethods[24]=reinterpret_cast<uintptr_t>(&owning);varMethods[25]=reinterpret_cast<uintptr_t>(&setComponent);varMethods[26]=reinterpret_cast<uintptr_t>(&getInt);varMethods[27]=reinterpret_cast<uintptr_t>(&getFloat);varMethods[28]=reinterpret_cast<uintptr_t>(&getVector);varMethods[30]=reinterpret_cast<uintptr_t>(&getCount);
        materialMethods[11]=reinterpret_cast<uintptr_t>(&find);materialMethods[29]=reinterpret_cast<uintptr_t>(&alphaFlag);material=reinterpret_cast<uintptr_t>(materialMethods.data());
        heapMethods[1]=reinterpret_cast<uintptr_t>(&allocate);heapMethods[5]=reinterpret_cast<uintptr_t>(&release);heap=reinterpret_cast<uintptr_t>(heapMethods.data());heapPointer=reinterpret_cast<uintptr_t>(&heap);word(0x4ae2c4,reinterpret_cast<uintptr_t>(&heapPointer));
        keyMethods[3]=reinterpret_cast<uintptr_t>(&symbol);keySystem=reinterpret_cast<uintptr_t>(keyMethods.data());word(0x4ae2e4,reinterpret_cast<uintptr_t>(&keyValues));
        word(0x4ae020,reinterpret_cast<uintptr_t>(&getError));word(0x4ae0cc,reinterpret_cast<uintptr_t>(&setError));word(0x4ae0d8,reinterpret_cast<uintptr_t>(&getTls));word(0x6a3500,78);word(0x6a34fc,77);
        const uint32_t locale=0x106a3418,multibyte=0x106a2d88,flags=2;std::memcpy(thread.data()+0x6c,&locale,4);std::memcpy(thread.data()+0x68,&multibyte,4);std::memcpy(thread.data()+0x70,&flags,4);image.seal();
    }
    void*build(const SourceMaterialPatchNodeCandidate&node){
        keys.emplace_back();auto&k=keys.back();const auto id=uint32_t(symbol(nullptr,nullptr,node.key.c_str(),true));std::memcpy(k.raw.data(),&id,4);k.raw[16]=uint8_t(node.kind);k.text=node.text;auto t=reinterpret_cast<uintptr_t>(k.text.c_str());std::memcpy(k.raw.data()+4,&t,4);std::memcpy(k.raw.data()+12,&node.bits,4);
        uint8_t*last=nullptr;for(const auto&child:node.children){auto*p=static_cast<uint8_t*>(build(child));auto address=reinterpret_cast<uintptr_t>(p);std::memcpy(last?last+20:k.raw.data()+24,&address,4);last=p;}return k.raw.data();
    }
    void prepare(const MaterialObjectParameterStateCandidate&state){if(!blocks.empty())throw std::runtime_error("Unreleased client proxy");keys.clear();symbols.clear();variables.clear();variables.resize(state.material.parameters.values.size());for(size_t i=0;i<variables.size();++i){variables[i].value=state.material.parameters.values[i];refresh(unsigned(i));}writes.clear();alphaFlags=0;}
    void*create(){using F=void*(__cdecl*)();auto*p=reinterpret_cast<F>(0x10137430)();if(!p||*static_cast<uintptr_t*>(p)!=0x104fff3c)throw std::runtime_error("Actual Equals factory/vtable");return p;}
    bool initialize(void*p,const SourceMaterialPatchNodeCandidate&node){using F=bool(__thiscall*)(void*,void*,void*);return reinterpret_cast<F>(0x100e5fa0)(p,&material,build(node));}
    void evaluate(void*p){using F=void(__thiscall*)(void*,void*);reinterpret_cast<F>(0x101373a0)(p,nullptr);image.verify();}
    void scalar(void*p,uint32_t bits){using F=void(__thiscall*)(void*,uint32_t);reinterpret_cast<F>(0x100e5eb0)(p,bits);image.verify();}
    bool materialIdentity(void*p){using F=void*(__thiscall*)(void*);return reinterpret_cast<F>(0x100e5f90)(p)==&material;}
    void destroy(void*p){using F=void(__thiscall*)(void*);reinterpret_cast<F>(0x10175ed0)(p);image.verify();}
    ~MaterialEqualsClientNativeFixture(){if(!blocks.empty()||allocations!=releases)std::terminate();image.verify();VirtualFree(image.base,0,MEM_RELEASE);active=nullptr;}
};
