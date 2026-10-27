#include "lightmapped_parameters_native_fixture.h"
#include <iostream>
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("Private material DLL, library directory, disposable mount required");
    LightmappedParametersNativeFixture f(argv[1],argv[2],argv[3]);using Count=int(__thiscall*)(void*);using Get=const uint32_t*(__thiscall*)(void*,int);using Fallback=const char*(__thiscall*)(void*,void**);
    for(size_t i=0;i<f.shaders.size();++i)if(f.shaderNames[i].find("UnlitTwoTexture")==0){
        auto*p=f.shaders[i];auto t=*static_cast<void***>(p);const int n=reinterpret_cast<Count>(t[2])(p);const auto fallback=reinterpret_cast<Fallback>(t[1])(p,nullptr);
        std::cout<<"shader "<<f.shaderNames[i]<<" count "<<n<<" shared "<<std::hex<<f.relative(t[4])<<" concrete "<<f.relative(t[12])<<" fallback_method "<<f.relative(t[1])<<std::dec<<" fallback "<<(fallback?fallback:"<none>")<<'\n';
        for(int j=0;j<n;++j){auto d=reinterpret_cast<Get>(t[3])(p,j);f.relative(d);std::cout<<j<<'\t'<<reinterpret_cast<const char*>(d[0])<<'\t'<<d[2]<<'\n';}
    }
    f.verifyCode();std::cout<<"PASS original executable sections unchanged\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
