#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


#include "reference_free_ring_capacity.h"
static uint32_t growthCalls;
static void __fastcall grow(void*self,void*,uint32_t addition){auto*d=static_cast<uint32_t*>(self);d[1]=recoveredReferenceStorageCapacityCandidate(d[1],int32_t(d[2]),addition,8);++growthCalls;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x102bdbd0,reinterpret_cast<void*>(&grow));using Append=void(__thiscall*)(void*,uint32_t);std::array<uint32_t,3>functions{0x1038f520,0x1038f580,0x1038f5e0};std::array<unsigned,3>offsets{0x64/4,0x78/4,0x8c/4};unsigned cases=0;
for(unsigned kind=0;kind<3;++kind){std::array<uint32_t,64>framework{};std::array<uint32_t,256>storage{};auto*d=framework.data()+offsets[kind];d[0]=uint32_t(storage.data());growthCalls=0;for(unsigned step=0;step<256;++step){uint32_t h=step%7==0?0xffffffffu:step%5;reinterpret_cast<Append>(functions[kind])(framework.data(),h);if(d[4]!=d[0])throw std::runtime_error("storage alias mismatch");std::array<uint32_t,259>out{};out[0]=d[3];out[1]=d[1];out[2]=growthCalls;std::memcpy(out.data()+3,storage.data(),1024);oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}}
std::cout<<"native_update_queue_steps="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
