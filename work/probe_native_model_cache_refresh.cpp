#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static uint32_t acquireResult;static std::vector<uint32_t> events;
static uint32_t __fastcall acquireModel(void*,void*,const char*name){if(strcmp(name,"models/test.mdl"))throw std::runtime_error("name mismatch");events.push_back(1);return acquireResult;}
static void __fastcall releaseModel(void*,void*,uint32_t handle){events.push_back(2);events.push_back(handle);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"x86 required");
std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
if(argc!=2||std::ifstream(argv[1]).good())throw std::runtime_error("fresh output required");std::ofstream out(argv[1],std::ios::binary);if(!out)throw std::runtime_error("output failed");
std::array<uint32_t,16> table{};table[0x24/4]=uint32_t(reinterpret_cast<uintptr_t>(&acquireModel));table[0x2c/4]=uint32_t(reinterpret_cast<uintptr_t>(&releaseModel));uint32_t cache=uint32_t(reinterpret_cast<uintptr_t>(table.data()));*reinterpret_cast<uint32_t*>(0x10f4c7f8)=uint32_t(reinterpret_cast<uintptr_t>(&cache));using Refresh=void(__thiscall*)(void*);
for(unsigned nameKind=0;nameKind<4;++nameKind)for(uint16_t old:{uint16_t(0xffff),uint16_t(0),uint16_t(7)})for(uint32_t next:{0xffffu,0u,7u,0x12340007u})for(unsigned dirty:{0u,1u}){std::array<uint32_t,80>model{};model[0xd4/4]=nameKind==0?0xffffffffu:nameKind==1?0u:uint32_t(reinterpret_cast<uintptr_t>(nameKind==2?"":"models/test.mdl"));auto bytes=reinterpret_cast<unsigned char*>(model.data());memcpy(bytes+0x128,&old,2);bytes[0x12a]=static_cast<unsigned char>(dirty);acquireResult=next;events.clear();reinterpret_cast<Refresh>(0x105c2640)(model.data());uint16_t resulting;memcpy(&resulting,bytes+0x128,2);for(auto v:{uint32_t(resulting),uint32_t(bytes[0x12a]),uint32_t(events.size())})out.write(reinterpret_cast<const char*>(&v),4);for(auto v:events)out.write(reinterpret_cast<const char*>(&v),4);}
std::cout<<"native model cache refresh cases=96\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

