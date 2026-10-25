#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
using Factory = void* (__cdecl*)(const char*, int*);
int main(int argc, char** argv) {
 static_assert(sizeof(void*)==4,"x86 only");
 if(argc!=3)return 2;
 if(!SetDllDirectoryA(argv[1]))return 3;
 HMODULE m=LoadLibraryExA((std::string(argv[1])+"\\vstdlib.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 if(!m)return 4;
 auto f=reinterpret_cast<Factory>(GetProcAddress(m,"CreateInterface")); if(!f)return 5;
 int status=-1; void* object=f("VEngineCvar007",&status);
 std::ofstream out(argv[2],std::ios::trunc); if(!out||!object||status)return 6;
 auto base=reinterpret_cast<std::uintptr_t>(m); auto** vt=*reinterpret_cast<void***>(object);
 out<<"interface=VEngineCvar007 status="<<status<<" object=0x"<<std::hex<<reinterpret_cast<std::uintptr_t>(object)<<" module_base=0x"<<base<<"\n";
 for(unsigned i=0;i<12;++i){auto p=reinterpret_cast<std::uintptr_t>(vt[i]); out<<"vtable["<<std::dec<<i<<"]=0x"<<std::hex<<p<<" rva=0x"<<(p-base)<<"\n";}
 out.close(); FreeLibrary(m); SetDllDirectoryA(nullptr); std::cout<<"vtable captured\n"; return 0;
}