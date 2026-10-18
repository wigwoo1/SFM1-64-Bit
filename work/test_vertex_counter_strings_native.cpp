#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <string>
#include <cstdlib>
#include <cstdio>
#include "vertex_counter_strings_native_init.h"
static void check(bool b){if(!b)std::abort();}
int main(){static_assert(sizeof(void*)==4,"original x86");auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code!=nullptr);
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 // Read current installed bytes using PE section RVA mapping.
 unsigned pe=0;dll.seekg(60);dll.read(reinterpret_cast<char*>(&pe),4);unsigned short sections=0,optional=0;dll.seekg(pe+6);dll.read(reinterpret_cast<char*>(&sections),2);dll.seekg(pe+20);dll.read(reinterpret_cast<char*>(&optional),2);
 auto load=[&](unsigned va,const unsigned char* bytes,unsigned size,unsigned placement){unsigned offset=0;for(unsigned i=0;i<sections;++i){std::array<unsigned,10> section{};dll.seekg(pe+24+optional+40*i);dll.read(reinterpret_cast<char*>(section.data()),40);unsigned rva=va-0x10000000;if(rva>=section[3]&&rva-section[3]+size<=section[4])offset=section[5]+rva-section[3];}check(offset!=0);std::array<unsigned char,512> raw{};dll.seekg(offset);dll.read(reinterpret_cast<char*>(raw.data()),size);check(bool(dll)&&!std::memcmp(raw.data(),bytes,size));std::memcpy(code+placement,raw.data(),size);};
 load(0x100d7470,vertexStrings0,sizeof(vertexStrings0),0);load(0x100d7640,vertexStrings1,sizeof(vertexStrings1),512);load(0x100e5bb0,vertexStrings2,sizeof(vertexStrings2),1024);
 unsigned delta=static_cast<unsigned>((code+1024)-(code+512+vertexStringCallOffset+5));std::memcpy(code+512+vertexStringCallOffset+1,&delta,4);DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,4096));
 auto copy=reinterpret_cast<void(__cdecl*)(char*,const char*)>(code);auto append=reinterpret_cast<void(__cdecl*)(char*,const char*,int,int)>(code+512);unsigned cases=0;
 for(const char* prefix:{"TexGroup_global_","TexGroup_frame_"})for(unsigned length:{0u,1u,239u,240u,255u,300u})for(unsigned alignment=0;alignment<4;++alignment){std::array<char,320> storage;storage.fill('!');std::string suffix(length,'x');auto expected=storage;std::string name=(std::string(prefix)+suffix).substr(0,255);std::memcpy(expected.data()+8+alignment,name.c_str(),name.size()+1);char* destination=storage.data()+8+alignment;copy(destination,prefix);append(destination,suffix.c_str(),256,-1);check(storage==expected);++cases;}
 check(VirtualFree(code,0,MEM_RELEASE));std::printf("original_vertex_counter_string_cases=%u passed\n",cases);
}
