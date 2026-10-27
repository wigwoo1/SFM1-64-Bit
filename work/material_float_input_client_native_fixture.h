#pragma once
#include "material_equals_client_native_fixture.h"
// Actual FloatInput Init/getter; only the existing service/data fixture grows.
// No original instructions are replaced. Scanf's original CRT initializer
// fills its data-only conversion table, like the prior matrix scanf oracle.
struct MaterialFloatInputClientNativeFixture:MaterialEqualsClientNativeFixture {
    static void* __fastcall resizeScratch(void*,void*,void*p,uint32_t n){
        auto&f=self();if(!p||!n||n>8192||f.blocks.erase(p)!=1)ExitProcess(90);
        auto*next=std::realloc(p,n);if(!next)ExitProcess(89);f.blocks.insert(next);++f.allocations;++f.releases;return next;
    }
    explicit MaterialFloatInputClientNativeFixture(const char*path):MaterialEqualsClientNativeFixture(path){
        heapMethods[3]=reinterpret_cast<uintptr_t>(&resizeScratch);
        using Init=void(__cdecl*)();reinterpret_cast<Init>(0x1045b659)();image.verify();
    }
    bool initializeInput(std::array<uint32_t,3>&input,const SourceMaterialPatchNodeCandidate&declaration,uint32_t defaultBits){
        using Init=bool(__thiscall*)(void*,void*,void*,const char*,uint32_t);
        return reinterpret_cast<Init>(0x100e60d0)(input.data(),&material,build(declaration),"input",defaultBits);
    }
    uint32_t readInput(std::array<uint32_t,3>&input){
        using Read=float(__thiscall*)(void*);float value=reinterpret_cast<Read>(0x100e5d20)(input.data());uint32_t bits;std::memcpy(&bits,&value,4);image.verify();return bits;
    }
};
