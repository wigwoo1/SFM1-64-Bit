#include "material_object_parameters_native_fixture.h"
#include <iostream>
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("Private material DLL, private library directory, disposable mount required");
    MaterialObjectParametersNativeFixture f(argv[1],argv[2],argv[3]);
    using Count=int(__thiscall*)(void*);using Get=const uint32_t*(__thiscall*)(void*,int);
    for(size_t i=0;i<f.shaders.size();++i)if(f.shaderNames[i]=="LightmappedGeneric"){
        auto*p=f.shaders[i];auto t=*static_cast<void***>(p);const int n=reinterpret_cast<Count>(t[2])(p);
        std::cout<<"shader "<<f.shaderNames[i]<<" count "<<n<<" shared "<<std::hex<<f.relative(t[4])<<" concrete "<<f.relative(t[12])<<std::dec<<'\n';
        for(int j=0;j<n;++j){auto d=reinterpret_cast<Get>(t[3])(p,j);f.relative(d);std::cout<<j<<'\t'<<reinterpret_cast<const char*>(d[0])<<'\t'<<d[2]<<'\n';}
        std::array<int32_t,60>mapping{};using Map=void(__thiscall*)(void*,void*);reinterpret_cast<Map>(f.stdBase+0x28020)(p,mapping.data());
        for(size_t j=0;j<mapping.size();++j)std::cout<<"mapping "<<std::hex<<j*4<<std::dec<<' '<<mapping[j]<<'\n';
        for(size_t a:{0xc4394u,0xc43ecu,0xc449cu}){uint32_t ptr=0;std::memcpy(&ptr,f.stdBase+a,4);std::cout<<"cvar "<<std::hex<<a<<std::dec<<" default "<<*reinterpret_cast<int32_t*>(ptr+0x30)<<" name "<<*reinterpret_cast<const char**>(ptr+12)<<'\n';}
    }
    f.verifyCode();std::cout<<"PASS registry executable sections unchanged\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
