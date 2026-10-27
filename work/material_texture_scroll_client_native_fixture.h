#pragma once
#include "material_texture_transform_client_native_fixture.h"
struct MaterialTextureScrollClientNativeFixture:MaterialTextureTransformClientNativeFixture {
    using MaterialTextureTransformClientNativeFixture::MaterialTextureTransformClientNativeFixture;
    static void __fastcall vector3(void*p,void*,uint32_t x,uint32_t y,uint32_t z){typed().applyTexture(p,3,{x,y,z,0});}
    void configureScroll(){varMethods[10]=reinterpret_cast<uintptr_t>(&vector3);}
    void sharedSnapshot(const std::array<uint32_t,4>&bits,uint32_t count){
        if(count>4)throw std::runtime_error("Checked native shared count");missingCache(bits);auto&v=variables.back().value.value.numeric;v.typeAndMetadata=(v.typeAndMetadata&~0x70u)|(count<<4);refresh(unsigned(parameterCount));const uint32_t owner=0;std::memcpy(variables.back().raw.data()+32,&owner,4);
    }
    void*createScroll(const std::string&name){
        if(name!="TextureScroll")return createTexture(name);using Call=void*(__cdecl*)();auto*p=reinterpret_cast<Call>(0x10198b60)();
        if(!p||*static_cast<uintptr_t*>(p)!=0x1050d454)throw std::runtime_error("Actual TextureScroll factory/vtable");return p;
    }
};
