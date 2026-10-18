#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


static unsigned phase,queries,prepares;
static uint32_t __fastcall getPhase(void*,void*){++queries;return phase;}
static void __fastcall prepare(void*,void*,void*){++prepares;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x1035e3a0,reinterpret_cast<void*>(&prepare));std::array<uint32_t,9>vtable{};vtable[8]=uint32_t(&getPhase);uint32_t model=uint32_t(vtable.data());*reinterpret_cast<uint32_t*>(0x10e9ccf8)=uint32_t(&model);using Begin=bool(__thiscall*)(void*);unsigned cases=0;
for(phase=0;phase<8;++phase)for(unsigned flags=0;flags<65536;++flags){std::array<uint32_t,20>owner{};owner[7]=123;owner[15]=0xa500beefu|((flags&255)<<16);std::array<uint32_t,8>attribute{};attribute[2]=uint32_t(owner.data());attribute[4]=0xabcd0000u|flags;std::array<uint32_t,64>framework{};std::array<uint32_t,8>queue{};framework[0x64/4]=uint32_t(queue.data());framework[0x68/4]=8;*reinterpret_cast<uint32_t*>(0x10e9ccf4)=uint32_t(framework.data());queries=prepares=0;bool result=reinterpret_cast<Begin>(0x103623b0)(attribute.data());std::array<uint32_t,7>out{attribute[4],owner[15],framework[0x70/4],queue[0],uint32_t(result),prepares,queries};oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_incoming_attribute_invalidation_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
