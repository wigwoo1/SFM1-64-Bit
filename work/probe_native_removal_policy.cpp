#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


static void marker(uint32_t address,uint32_t value){auto*p=reinterpret_cast<unsigned char*>(address);p[0]=0xb8;std::memcpy(p+1,&value,4);p[5]=0xc3;FlushInstructionCache(GetCurrentProcess(),p,6);}
__declspec(naked) uint32_t policy(void*,uint32_t){__asm{push ebp
 mov ebp,esp
 mov eax,dword ptr[ebp+8]
 mov edx,1035b384h
 call edx
 pop ebp
 ret
}}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




marker(0x1035b3b5,0);marker(0x1035b411,1);unsigned cases=0;
for(unsigned mode=0;mode<8;++mode)for(unsigned bits=0;bits<256;++bits){std::array<uint32_t,5>record{};record[1]=(bits&1?1:0)|(bits&2?0x400:0)|(bits&4?0x100000:0)|(bits&8?0x40000000:0)|(bits&16?0x80000000:0);record[3]=bits&32?1:0xffffffffu;auto result=policy(record.data(),mode);oracle.write(reinterpret_cast<const char*>(&result),4);++cases;}
std::cout<<"native_removal_retirement_policy_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
