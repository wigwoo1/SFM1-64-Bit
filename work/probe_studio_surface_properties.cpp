#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include "source_surface_name_table_candidate.h"
#include "source_studio_surface_properties_candidate.h"
#ifdef NATIVE_STUDIO_SURFACE_PROPERTIES
#define NOMINMAX
#include <Windows.h>
#include "native_private_mdlcache_fixture.h"
#include "native_vphysics_library_fixture.h"
#endif
template<class T>static T word(const void*b,size_t p){T value;std::memcpy(&value,static_cast<const uint8_t*>(b)+p,sizeof(value));return value;}
template<class T>static void set(void*b,size_t p,T value){std::memcpy(static_cast<uint8_t*>(b)+p,&value,sizeof(value));}
template<class T>static void get(std::istream&i,T&value){if(!i.read(reinterpret_cast<char*>(&value),sizeof(value)))throw std::runtime_error("surface fixture truncated");}
template<class T>static void put(std::ostream&o,const T&value){o.write(reinterpret_cast<const char*>(&value),sizeof(value));}
static std::string string(std::istream&i){uint32_t n=0;get(i,n);if(n>1u<<24)throw std::runtime_error("surface fixture string extent");std::string result(n,'\0');if(!i.read(result.data(),n))throw std::runtime_error("surface fixture string truncated");return result;}
#ifdef NATIVE_STUDIO_SURFACE_PROPERTIES
static void invoke(uintptr_t function,void*header){
    __asm {
        push edi
        mov edi,header
        mov eax,function
        call eax
        pop edi
    }
}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"surface native fault eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
    if(argc!=5)throw std::runtime_error("datacache library-dir fixture output required");std::ifstream in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);if(!in||!out)throw std::runtime_error("surface fixture streams");uint32_t count=0;get(in,count);if(count>4096)throw std::runtime_error("surface script count");std::vector<SourceSurfaceScriptCandidate>scripts;for(uint32_t i=0;i<count;++i){auto name=string(in),text=string(in);scripts.push_back({std::move(name),std::move(text)});}
#ifdef NATIVE_STUDIO_SURFACE_PROPERTIES
    static_assert(sizeof(void*)==4,"original x86");AddVectoredExceptionHandler(1,fault);NativePrivateMdlCacheFixture cache(argv[1]);NativeVphysicsLibraryFixture physics(argv[2]);using Factory=void*(__cdecl*)(const char*,int*);const auto factory=reinterpret_cast<Factory>(GetProcAddress(physics.module,"CreateInterface"));int code=-1;void*surface=factory("VPhysicsSurfaceProps001",&code);if(!surface||code)throw std::runtime_error("surface interface unavailable");const auto table=*reinterpret_cast<uintptr_t**>(surface);if(physics.rva(table[1])!=0x16a50||physics.rva(table[2])!=0x16220||physics.rva(table[3])!=0x163b0)throw std::runtime_error("surface ABI changed");set(cache.base,0x3e7b8,surface);cache.seal();using Parse=int32_t(__thiscall*)(void*,const char*,const char*);using Lookup=int32_t(__thiscall*)(void*,const char*);using Count=int32_t(__thiscall*)(void*);
    put(out,count);for(const auto&s:scripts)put(out,reinterpret_cast<Parse>(table[1])(surface,s.name.c_str(),s.text.c_str()));put(out,reinterpret_cast<Count>(table[2])(surface));
#else
    static_assert(sizeof(void*)==8,"owned x64");const SourceSurfaceNameTableCandidate surface(scripts);put(out,count);for(auto n:surface.parseCounts())put(out,n);put(out,uint32_t(surface.size()));
#endif
    get(in,count);if(count>100000)throw std::runtime_error("surface lookup count");put(out,count);for(uint32_t i=0;i<count;++i){const auto name=string(in);
#ifdef NATIVE_STUDIO_SURFACE_PROPERTIES
        put(out,reinterpret_cast<Lookup>(table[3])(surface,name.c_str()));
#else
        put(out,surface.lookup(name));
#endif
    }
    get(in,count);if(count>65536)throw std::runtime_error("studio model count");put(out,count);size_t bones=0;
    for(uint32_t i=0;i<count;++i){const auto path=string(in);std::ifstream model(path,std::ios::binary);if(!model)throw std::runtime_error("model read");std::vector<uint8_t>mdl((std::istreambuf_iterator<char>(model)),{});const auto before=mdl;const SourceStudioPhysicsContentsCandidate checked(mdl);const size_t boneCount=checked.boneCount();bones+=boneCount;
#ifdef NATIVE_STUDIO_SURFACE_PROPERTIES
        invoke(uintptr_t(cache.base+0x5870),mdl.data());const int32_t result=word<int32_t>(mdl.data(),396);put(out,result);put(out,uint32_t(boneCount));auto allowed=before;set(allowed.data(),396,result);for(size_t j=0;j<boneCount;++j){const size_t p=word<uint32_t>(mdl.data(),160)+j*216+184;const auto value=word<int32_t>(mdl.data(),p);put(out,value);set(allowed.data(),p,value);}if(mdl!=allowed)throw std::runtime_error("native surface cache changed unrelated model data");
#else
        const auto result=sourceStudioResolveSurfacePropertiesCandidate(mdl,surface);put(out,result.model);put(out,uint32_t(result.bones.size()));for(auto value:result.bones)put(out,value);if(mdl!=before)throw std::runtime_error("owned resolver changed input model");
#endif
    }
    if(in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing surface fixture");out.flush();if(!out)throw std::runtime_error("surface output failure");
#ifdef NATIVE_STUDIO_SURFACE_PROPERTIES
    cache.verify();physics.verify();
#endif
    std::cout<<"PASS studio surface provider models="<<count<<" bones="<<bones<<" checked model/cache writes and input preservation\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
