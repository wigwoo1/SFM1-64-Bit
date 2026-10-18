#include <Windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include "native_private_mdlcache_fixture.h"
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"filesystem fault eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"original x86");if(argc!=6)throw std::runtime_error("datacache library-directory mount relative-file output required");AddVectoredExceptionHandler(1,fault);NativePrivateMdlCacheFixture image(argv[1]);std::string dir=argv[2];if(!SetDllDirectoryA(dir.c_str()))throw std::runtime_error("DLL directory");HMODULE tier=LoadLibraryExA((dir+"\\tier0.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!tier)throw std::runtime_error("tier0 load");HMODULE vstdlib=LoadLibraryExA((dir+"\\vstdlib.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!vstdlib)throw std::runtime_error("vstdlib load");HMODULE fs=LoadLibraryExA((dir+"\\filesystem_stdio.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!fs)throw std::runtime_error("filesystem load");using Factory=void*(__cdecl*)(const char*,int*);const auto factory=reinterpret_cast<Factory>(GetProcAddress(fs,"CreateInterface"));if(!factory)throw std::runtime_error("filesystem factory");int status=-1;void*object=factory("VFileSystem017",&status);if(!object||status)throw std::runtime_error("filesystem interface");const auto table=*reinterpret_cast<uintptr_t**>(object);
    if(table[10]-uintptr_t(fs)!=0xea40||table[12]-uintptr_t(fs)!=0xb190||table[16]-uintptr_t(fs)!=0xb6a0)throw std::runtime_error("filesystem path ABI changed");
    using AddPath=void(__thiscall*)(void*,const char*,const char*,int);reinterpret_cast<AddPath>(table[10])(object,argv[3],"GAME",0);
    std::array<char,65536>searchPaths{};using SearchPaths=int(__thiscall*)(void*,const char*,bool,char*,int);const int pathBytes=reinterpret_cast<SearchPaths>(table[16])(object,"GAME",false,searchPaths.data(),int(searchPaths.size()));if(pathBytes<=1||!searchPaths[0])throw std::runtime_error("explicit filesystem mount not published");
    std::vector<std::pair<const uint8_t*,std::vector<uint8_t>>>code;const auto base=reinterpret_cast<const uint8_t*>(fs);const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto&s=IMAGE_FIRST_SECTION(nt)[i];if(s.Characteristics&IMAGE_SCN_MEM_EXECUTE)code.push_back({base+s.VirtualAddress,std::vector<uint8_t>(base+s.VirtualAddress,base+s.VirtualAddress+s.Misc.VirtualSize)});}
    *reinterpret_cast<uintptr_t*>(image.base+0x3fedc)=uintptr_t(object);image.seal();uintptr_t control=0;using Start=int(__thiscall*)(void*,const char*,const char*,bool,uintptr_t*);const int start=reinterpret_cast<Start>(image.base+0x63b0)(nullptr,argv[4],"GAME",false,&control);int result=-5,bytes=0;void*data=nullptr;const bool hadControl=control!=0;
    if(control){using Finish=int(__thiscall*)(void*,uintptr_t,bool);reinterpret_cast<Finish>(table[51])(object,control,true);using Get=int(__thiscall*)(void*,uintptr_t,void**,int*);result=reinterpret_cast<Get>(table[52])(object,control,&data,&bytes);}
    if(bytes<0||bytes>512*1024*1024||(!data&&bytes))throw std::runtime_error("invalid read result span");std::ofstream out(argv[5],std::ios::binary);if(!out)throw std::runtime_error("result output");if(bytes)out.write(static_cast<const char*>(data),bytes);out.flush();if(!out)throw std::runtime_error("result write");
    if(data){using Free=void(__thiscall*)(void*,void*);reinterpret_cast<Free>(table[87])(object,data);data=nullptr;}if(control){using Release=void(__thiscall*)(void*,uintptr_t);reinterpret_cast<Release>(table[57])(object,control);control=0;}
    image.verify();for(const auto&r:code)if(std::memcmp(r.first,r.second.data(),r.second.size()))throw std::runtime_error("loaded filesystem code changed");using RemovePaths=void(__thiscall*)(void*);reinterpret_cast<RemovePaths>(table[12])(object);FreeLibrary(fs);FreeLibrary(vstdlib);FreeLibrary(tier);SetDllDirectoryA(nullptr);std::cout<<"{\"start\":"<<start<<",\"result\":"<<result<<",\"bytes\":"<<bytes<<",\"had_control\":"<<hadControl<<",\"released\":true}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
