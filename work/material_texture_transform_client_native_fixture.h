#pragma once
#include "material_sine_client_native_fixture.h"
#include "material_texture_transform_matrix_candidate.h"
// Original methods run unchanged. Shared missing-var data is an explicit
// snapshot of the separately proved material-system cache, not a zero stub.
struct MaterialTextureTransformClientNativeFixture:MaterialSineClientNativeFixture {
    using MaterialSineClientNativeFixture::MaterialSineClientNativeFixture;
    size_t parameterCount=0;
    static MaterialTextureTransformClientNativeFixture&typed(){return *static_cast<MaterialTextureTransformClientNativeFixture*>(active);}
    static void* __fastcall findTexture(void*object,void*,const char*name,bool*found,bool){
        auto&f=typed();if(object!=&f.material)ExitProcess(96);const auto key=sourceMaterialPatchLowerCandidate(name?name:"");
        for(size_t i=f.parameterCount;i>0;--i)if(sourceMaterialPatchLowerCandidate(f.variables[i-1].value.name)==key){if(found)*found=true;return f.variables[i-1].raw.data();}
        if(found)*found=false;return f.variables.back().raw.data();
    }
    static void __fastcall getTextureVector(void*p,void*,uint32_t*out,int count){
        auto&f=typed();if(count<1||count>4)ExitProcess(98);const auto&v=f.variables[f.index(p)].value.value.numeric.vector;
        for(int i=0;i<count;++i)out[i]=materialTextureTransformReadCandidate(v[size_t(i)]);
    }
    void applyTexture(void*p,uint32_t op,std::array<uint32_t,4>args,uint32_t count=0){
        const auto i=index(p);if(i>=parameterCount)throw std::runtime_error("Result writes must target found parameters");
        if(!materialTypedNumericWriteCandidate(variables[i].value,op,args,count))throw std::runtime_error("Reference write outside checked value domain");
        writes.push_back({i,op,count,args});refresh(i);
    }
    static void __fastcall textureFloat(void*p,void*,uint32_t bits){typed().applyTexture(p,1,{bits,0,0,0});}
    static void __fastcall textureInt(void*p,void*,uint32_t bits){typed().applyTexture(p,0,{bits,0,0,0});}
    static void __fastcall textureVector(void*p,void*,const uint32_t*bits,int count){if(count<1||count>4)ExitProcess(99);std::array<uint32_t,4>a{};std::copy_n(bits,count,a.begin());typed().applyTexture(p,5,a,uint32_t(count));}
    static void __fastcall textureMatrix(void*p,void*,const uint32_t*bits){
        auto&f=typed();const auto i=f.index(p);if(i>=f.parameterCount)throw std::runtime_error("No matrix write through missing result");MaterialMatrixMathCandidate m;std::copy_n(bits,16,m.begin());
        if(!materialTypedMatrixSetCandidate(f.variables[i].value,m))throw std::runtime_error("Reference matrix write outside checked value domain");MaterialProxyWriteCandidate event;event.index=i;event.operation=9;event.count=16;event.matrix=m;f.writes.push_back(std::move(event));f.refresh(i);
    }
    void prepareTexture(const MaterialObjectParameterStateCandidate&state){
        prepare(state);parameterCount=variables.size();variables.emplace_back();auto&v=variables.back().value;v.created=true;v.name="<shared missing cache>";v.value.numeric.typeAndMetadata=0xff44;for(size_t i=0;i<variables.size();++i)refresh(unsigned(i));
        const uint32_t owner=0;std::memcpy(variables.back().raw.data()+32,&owner,4);
        materialMethods[11]=reinterpret_cast<uintptr_t>(&findTexture);varMethods[28]=reinterpret_cast<uintptr_t>(&getTextureVector);varMethods[3]=reinterpret_cast<uintptr_t>(&textureFloat);varMethods[4]=reinterpret_cast<uintptr_t>(&textureInt);varMethods[12]=reinterpret_cast<uintptr_t>(&textureVector);varMethods[19]=reinterpret_cast<uintptr_t>(&textureMatrix);
    }
    void missingCache(const std::array<uint32_t,4>&bits){variables.back().value.value.numeric.vector=bits;const auto owner=uint32_t(0);refresh(unsigned(parameterCount));std::memcpy(variables.back().raw.data()+32,&owner,4);}
    void*createTexture(const std::string&name){
        if(name!="TextureTransform")return createFunction(name);using Create=void*(__cdecl*)();auto*p=reinterpret_cast<Create>(0x10137e40)();if(!p||*static_cast<uintptr_t*>(p)!=0x105001a4)throw std::runtime_error("Actual TextureTransform factory/vtable");return p;
    }
};
