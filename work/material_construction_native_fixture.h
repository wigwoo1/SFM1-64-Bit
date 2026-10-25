#pragma once
// Verification-only services for unchanged private material constructors.
// Caller supplies Windows/CRT/STL headers. Never used by owned x64 runtime.
#include "native_private_world_image_fixture.h"
struct MaterialConstructionNativeFixture{
    NativePrivateWorldImageFixture image;
    std::array<uint8_t,0x214>thread{};
    std::array<uintptr_t,145>services{};
    std::array<uintptr_t,6>allocators{};
    uintptr_t service=0,allocator=0,allocatorPointer=0;
    std::array<uintptr_t,5>keyValuesServices{};uintptr_t keyValuesService=0;
    unsigned warnings=0,nameLookups=0;
    unsigned allocations=0,releases=0,queues=0;
    struct Block{void*p;bool live;};std::vector<Block>blocks;
    inline static MaterialConstructionNativeFixture*active=nullptr;
    static void __fastcall unexpected(void*,void*){ExitProcess(92);}
    static void* __fastcall noQueue(void*,void*){++active->queues;return nullptr;}
    static void* __fastcall alloc(void*,void*,uint32_t size){
        if(size>8192)ExitProcess(93);void*p=std::malloc(size);if(!p)ExitProcess(94);
        active->blocks.push_back({p,true});++active->allocations;return p;
    }
    static void __fastcall free(void*,void*,void*p){
        for(auto i=active->blocks.rbegin();i!=active->blocks.rend();++i)if(i->p==p&&i->live){i->live=false;++active->releases;std::free(p);return;}
        ExitProcess(95);
    }
    static void* __stdcall getThread(DWORD){return active->thread.data();}
    static void* __stdcall getTls(DWORD index){if(index!=77)ExitProcess(96);return reinterpret_cast<void*>(&getThread);}
    static DWORD __stdcall getError(){return 0;}
    static void __stdcall setError(DWORD){}
    static void* __cdecl keyValues(){return &active->keyValuesService;}
    static const char* __fastcall keyName(void*,void*,uint32_t symbol){if(symbol)ExitProcess(97);++active->nameLookups;return "$fixture";}
    static void __cdecl warning(const char*,...){++active->warnings;}
    void word(size_t offset,uintptr_t v){std::memcpy(image.base+offset,&v,4);}
    explicit MaterialConstructionNativeFixture(const char*path,bool vectorServices=false):image(path,0x100000){
        if(active)throw std::runtime_error("Single private fixture required");active=this;
        services.fill(reinterpret_cast<uintptr_t>(&unexpected));services[144]=reinterpret_cast<uintptr_t>(&noQueue);
        service=reinterpret_cast<uintptr_t>(services.data());word(0xfc7b0,reinterpret_cast<uintptr_t>(&service));
        allocators.fill(reinterpret_cast<uintptr_t>(&unexpected));allocators[1]=reinterpret_cast<uintptr_t>(&alloc);allocators[5]=reinterpret_cast<uintptr_t>(&free);
        allocator=reinterpret_cast<uintptr_t>(allocators.data());allocatorPointer=reinterpret_cast<uintptr_t>(&allocator);word(0x951ec,reinterpret_cast<uintptr_t>(&allocatorPointer));
        word(0x95078,reinterpret_cast<uintptr_t>(&getTls));word(0x9511c,reinterpret_cast<uintptr_t>(&getError));word(0x9506c,reinterpret_cast<uintptr_t>(&setError));
        word(0x104be8,77);word(0x104be4,78);
        const uint32_t locale=0x10104b00,multibyte=0x10104470,flags=2;
        std::memcpy(thread.data()+0x6c,&locale,4);std::memcpy(thread.data()+0x68,&multibyte,4);std::memcpy(thread.data()+0x70,&flags,4);
        if(vectorServices){
            keyValuesServices.fill(reinterpret_cast<uintptr_t>(&unexpected));keyValuesServices[4]=reinterpret_cast<uintptr_t>(&keyName);
            keyValuesService=reinterpret_cast<uintptr_t>(keyValuesServices.data());
            word(0x95208,reinterpret_cast<uintptr_t>(&keyValues));word(0x951e4,reinterpret_cast<uintptr_t>(&warning));
        }
        image.seal();
    }
    void begin(bool sse2){
        for(const auto&b:blocks)if(b.live)throw std::runtime_error("Leaked previous allocation");blocks.clear();allocations=releases=queues=0;
        word(0x13ff3c,sse2?1:0);
    }
    ~MaterialConstructionNativeFixture(){if(image.base)VirtualFree(image.base,0,MEM_RELEASE);active=nullptr;}
};
