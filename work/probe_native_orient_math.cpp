#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static std::array<std::array<uint32_t,64>,4> targets{}, dags{};
static void* __fastcall resolve(void*,void*,uint32_t h){if(h>=1&&h<=4)return targets[h-1].data();if(h>=11&&h<=14)return dags[h-11].data();return nullptr;}
static bool __fastcall typeCheck(void*,void*,uint32_t){return true;}
static void __fastcall worldPosition(void* self,void*,float*out){std::memcpy(out,static_cast<char*>(self)+0x20,12);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");


 for(float input:{0.0f,.25f,1.0f,2.0f}){std::cout<<"input="<<input;for(uint32_t address:{0x10A13A91u,0x10A157DAu,0x10A0E7B2u,0x10A0E65Bu}){float output;
 __asm {movss xmm0,input
 mov eax,address
 call eax
 movss output,xmm0}
 std::cout<<" "<<std::hex<<address<<std::dec<<"="<<output;}
 std::cout<<std::endl;}
 for(double y:{.25,1.0,2.0}){double x=1.0,out;uint32_t address=0x10A0E901;
 __asm {movsd xmm0,y
 movsd xmm1,x
 mov eax,address
 call eax
 movsd out,xmm0}
 std::cout<<"atan_candidate y="<<y<<" x="<<x<<" output="<<out<<std::endl;}
 return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
