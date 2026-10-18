#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


static std::array<uint32_t,8>out;static unsigned pattern;
static void event(uint32_t c){out[2+out[1]++]=c;}
static bool __fastcall blocked(void*,void*){event(1);return(pattern&1)!=0;}
static void __fastcall queue(void*,void*,uint32_t){event(2);}
static void __fastcall cacheQueue(void*,void*,uint32_t){event(3);}
static uint32_t __fastcall firstReverse(void*,void*,uint32_t){event(4);return 0;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x1038f580,reinterpret_cast<void*>(&queue));redirect(0x1038f5e0,reinterpret_cast<void*>(&cacheQueue));std::array<uint32_t,120>vtable{};vtable[0x1dc/4]=uint32_t(&blocked);vtable[0x1cc/4]=uint32_t(&firstReverse);uint32_t model=uint32_t(vtable.data());*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(&model);using Dirty=void(__thiscall*)(void*,uint32_t);unsigned cases=0;
for(unsigned state=0;state<256;++state)for(unsigned flags=0;flags<16;++flags)for(unsigned request=0;request<4;++request)for(pattern=0;pattern<4;++pattern){std::array<uint32_t,24>element{};element[15]=0xa500beefu|(state<<16);element[16]=0x87654320u|flags;element[17]=pattern&2?1:0;out={};reinterpret_cast<Dirty>(0x1035e400)(element.data(),request);out[0]=element[15];out[7]=element[16];oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_dirty_state_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
