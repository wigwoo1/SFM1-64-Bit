#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


static std::array<uint32_t,64>out;
static void* __fastcall resolve(void*self,void*,uint32_t handle){out[16+out[6]++]=handle;using Resolve=void*(__thiscall*)(void*,uint32_t);return reinterpret_cast<Resolve>(0x10354b20)(static_cast<char*>(self)+0xc0c0,handle);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




std::array<uint32_t,20>vtable{};vtable[18]=uint32_t(&resolve);using First=uint32_t(__thiscall*)(void*);using Next=uint32_t(__thiscall*)(void*,uint32_t);unsigned cases=0;
for(unsigned seed=0;seed<4096;++seed){std::array<uint32_t,0xc240/4>framework{};framework[0]=uint32_t(vtable.data());std::array<std::array<uint32_t,2>,8>slots{};uint32_t r=seed+1;for(auto&s:slots){r=r*1664525u+1013904223u;s[0]=((r>>8)%513)|(r&0x80000000u);s[1]=r&4?0x1234:0;}framework[0xc0c4/4]=uint32_t(slots.data());framework[0xc0d0/4]=8;out={};out[0]=reinterpret_cast<First>(0x10356040)(framework.data());std::array<uint32_t,5>current{0,3,7,0xffffffffu,0x80000000u};for(unsigned i=0;i<5;++i)out[1+i]=reinterpret_cast<Next>(0x103560d0)(framework.data(),current[i]);oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_live_handle_iteration_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
