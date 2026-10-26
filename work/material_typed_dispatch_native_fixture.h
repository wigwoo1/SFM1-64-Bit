#pragma once
// Test-only: complete original dispatch/constructors execute unpatched.
// Controlled services: KeyValues name provider, allocator, null queue and
// single-thread lock shim. The original symbol table itself is initialized and
// executed; the original pool consumes/recycles one supplied free object.
#include "material_construction_native_fixture.h"
struct MaterialTypedDispatchNativeFixture:MaterialConstructionNativeFixture {
    std::string inputName="$fixture";
    std::array<uint8_t,40>object{};
    unsigned lockDepth=0,locks=0;
    static MaterialTypedDispatchNativeFixture&self(){return *static_cast<MaterialTypedDispatchNativeFixture*>(active);}
    static void* __fastcall allocate(void*,void*,uint32_t size){
        auto&f=self();if(!size||size>0x100000)ExitProcess(101);
        void*p=std::malloc(size);if(!p)ExitProcess(102);f.blocks.push_back({p,true});++f.allocations;return p;
    }
    static void* __fastcall resize(void*,void*,void*p,uint32_t size){
        auto&f=self();if(!p)return allocate(nullptr,nullptr,size);
        if(!size){free(nullptr,nullptr,p);return nullptr;}
        if(size>0x100000)ExitProcess(103);
        size_t i=0;for(;i<f.blocks.size();++i)if(f.blocks[i].p==p&&f.blocks[i].live)break;
        if(i==f.blocks.size())ExitProcess(104);
        void*next=std::realloc(p,size);if(!next)ExitProcess(105);
        f.blocks[i].live=false;f.blocks.push_back({next,true});++f.releases;++f.allocations;return next;
    }
    static const char* __fastcall name(void*,void*,uint32_t symbol){
        auto&f=self();if(symbol)ExitProcess(106);++f.nameLookups;return f.inputName.c_str();
    }
    static void __fastcall lock(void*,void*){auto&f=self();++f.locks;if(f.lockDepth++)ExitProcess(107);}
    static void __fastcall unlock(void*,void*){auto&f=self();if(f.lockDepth!=1)ExitProcess(108);--f.lockDepth;}
    explicit MaterialTypedDispatchNativeFixture(const char*path):MaterialConstructionNativeFixture(path,true){
        allocators[1]=reinterpret_cast<uintptr_t>(&allocate);allocators[3]=reinterpret_cast<uintptr_t>(&resize);
        keyValuesServices[4]=reinterpret_cast<uintptr_t>(&name);
        for(size_t offset:{0x95174u,0x9517cu})word(offset,reinterpret_cast<uintptr_t>(&lock));
        for(size_t offset:{0x95178u,0x95180u})word(offset,reinterpret_cast<uintptr_t>(&unlock));
        using Init=void(__cdecl*)();reinterpret_cast<Init>(0x1007d4e7)();
        using Symbols=void*(__thiscall*)(void*,int,int,bool);
        reinterpret_cast<Symbols>(0x10073600)(image.base+0x1102e8,0,32,true);
    }
    void prepare(bool numericSse2,bool trigSse2,bool available,uint8_t fill){
        object.fill(fill);const uintptr_t end=0;std::memcpy(object.data(),&end,4);
        // Original pool allocation's checked hot path: exact block size and a
        // real available free slot; no replacement factory/constructor call.
        std::memset(image.base+0x10db04,0,0x28);
        word(0x10db04,available?40:39);word(0x10db10,reinterpret_cast<uintptr_t>(object.data()));
        word(0x13ff3c,numericSse2?1:0);word(0x13ff40,trigSse2?1:0);
        warnings=nameLookups=locks=0;if(lockDepth)ExitProcess(109);
    }
    void destroy(void*p){
        using Destroy=void*(__thiscall*)(void*,unsigned);reinterpret_cast<Destroy>(0x10018e60)(p,1);
        if(*reinterpret_cast<uint32_t*>(image.base+0x10db14)!=0||lockDepth)ExitProcess(110);
    }
    void finish(){
        using Clear=void(__thiscall*)(void*);reinterpret_cast<Clear>(0x10073580)(image.base+0x1102e8);
        for(const auto&b:blocks)if(b.live)ExitProcess(111);
        if(allocations!=releases||lockDepth)ExitProcess(112);image.verify();
    }
};
