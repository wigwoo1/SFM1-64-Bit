#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
#include "flex_registry_fixture.h"
static std::vector<void*> allocations;
static void* __fastcall allocate(void*,void*,unsigned size){void*p=HeapAlloc(GetProcessHeap(),0,size);if(!p)throw std::runtime_error("allocation failed");allocations.push_back(p);return p;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"x86 required");
std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\tf\\bin\\client.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
if(argc!=2||std::ifstream(argv[1]).good())throw std::runtime_error("fresh output required");std::ofstream out(argv[1],std::ios::binary);if(!out)throw std::runtime_error("output failed");
std::array<uint32_t,4> table{};table[1]=uint32_t(reinterpret_cast<uintptr_t>(&allocate));uint32_t allocator=uint32_t(reinterpret_cast<uintptr_t>(table.data()));uint32_t allocatorPointer=uint32_t(reinterpret_cast<uintptr_t>(&allocator));*reinterpret_cast<uint32_t*>(0x104ae2c4)=uint32_t(reinterpret_cast<uintptr_t>(&allocatorPointer));*reinterpret_cast<uint32_t*>(0x106b4e80)=0;
using Register=uint32_t(__cdecl*)(const char*);
for(unsigned i=0;i<1200;++i){auto name=flexRegistryFixtureName(i);auto index=reinterpret_cast<Register>(0x100451d0)(name.c_str());auto count=*reinterpret_cast<uint32_t*>(0x106b4e80);out.write(reinterpret_cast<char*>(&index),4);out.write(reinterpret_cast<char*>(&count),4);for(unsigned j=0;j<count;++j){auto p=reinterpret_cast<const char**>(0x106b3e80)[j];uint32_t length=uint32_t(std::strlen(p));out.write(reinterpret_cast<char*>(&length),4);out.write(p,length);}}
for(auto p:allocations)HeapFree(GetProcessHeap(),0,p);std::cout<<"native registry cases=1200\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
