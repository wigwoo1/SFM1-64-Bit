#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}
static unsigned calls;static uint32_t queued;static unsigned char observed;
static void* currentElement;
static void __fastcall enqueue(void*,void*,uint32_t handle){++calls;queued=handle;observed=*(static_cast<unsigned char*>(currentElement)+0x3e);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"x86 required");
std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
if(argc!=2||std::ifstream(argv[1]).good())throw std::runtime_error("fresh output required");std::ofstream out(argv[1],std::ios::binary);if(!out)throw std::runtime_error("output failed");
redirect(0x1038f520,reinterpret_cast<void*>(&enqueue));*reinterpret_cast<uint32_t*>(0x10e9ccf4)=0;using Mark=void(__thiscall*)(void*);
for(unsigned flags=0;flags<256;++flags){std::array<uint32_t,20>element{};element[0x1c/4]=0x12345678;currentElement=element.data();auto bytes=reinterpret_cast<unsigned char*>(element.data());bytes[0x3e]=static_cast<unsigned char>(flags);calls=0;queued=0;observed=0;for(unsigned pass=0;pass<2;++pass){reinterpret_cast<Mark>(0x1035db90)(element.data());for(auto v:{uint32_t(bytes[0x3e]),calls,queued,uint32_t(observed)})out.write(reinterpret_cast<const char*>(&v),4);}}
std::cout<<"native resolve queue dirty marks=512\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

