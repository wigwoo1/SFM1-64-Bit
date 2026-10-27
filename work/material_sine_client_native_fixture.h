#pragma once
#include "material_float_input_client_native_fixture.h"
struct MaterialSineClientNativeFixture:MaterialFloatInputClientNativeFixture {
    std::array<uint32_t,16>clock{};
    explicit MaterialSineClientNativeFixture(const char*path):MaterialFloatInputClientNativeFixture(path){word(0x63ce48,reinterpret_cast<uintptr_t>(clock.data()));}
    void*createFunction(const std::string&name){
        const uintptr_t factory=name=="Sine"?0x10137370:name=="Equals"?0x10137430:0;
        if(!factory)throw std::runtime_error("Unchecked native proxy name");using Call=void*(__cdecl*)();auto*p=reinterpret_cast<Call>(factory)();if(!p||*static_cast<uintptr_t*>(p)!=(name=="Sine"?0x104fff24u:0x104fff3cu))throw std::runtime_error("Actual function factory");return p;
    }
    bool initializeFunction(void*p,const SourceMaterialPatchNodeCandidate&node){using Call=bool(__thiscall*)(void*,void*,void*);auto*v=*static_cast<uintptr_t**>(p);return reinterpret_cast<Call>(v[0])(p,&material,build(node));}
    void evaluateFunction(void*p,uint32_t time){clock[3]=time;using Call=void(__thiscall*)(void*,void*);auto*v=*static_cast<uintptr_t**>(p);reinterpret_cast<Call>(v[1])(p,nullptr);}
};
