#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}




#include "pack_color_fixture.h"
static uint32_t packed=0,calls=0;static void __fastcall writeColor(void*,void*,const uint32_t*value){packed=*value;++calls;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);

 if(argc!=2||std::ifstream(argv[1]).good())throw std::runtime_error("fresh output required");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("output failed");redirect(0x1036cd90,reinterpret_cast<void*>(&writeColor));using Execute=void(__thiscall*)(void*);
 for(uint32_t seed=0;seed<4096;++seed){std::array<uint32_t,40>object{};for(unsigned i=0;i<4;++i){auto bits=packColorFixtureBits(seed,i);object[(0x6c+i*8)/4]=bits;}packed=0;calls=0;reinterpret_cast<Execute>(0x105c8530)(object.data());if(calls!=1)throw std::runtime_error("setter call count");oracle.write(reinterpret_cast<const char*>(&packed),4);}std::cout<<"native pack_color_cases=4096\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
