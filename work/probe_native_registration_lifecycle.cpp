#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


#include "registration_lifecycle_fixture.h"
static RegistrationFixture fixture;static uint32_t trace;static std::array<std::array<uint32_t,3>,6>nodes;
static bool __fastcall blocked(void*,void*){registrationTrace(trace,1);return fixture.blocked;}
static uint32_t __fastcall firstReverse(void*,void*,uint32_t){registrationTrace(trace,2);return 0;}
static void __fastcall notify(void*,void*,void*){registrationTrace(trace,3);}
static void __fastcall release(void*,void*,void*p){for(unsigned i=0;i<6;++i)if(p==nodes[i].data()){registrationTrace(trace,10+i);nodes[i]={};return;}throw std::runtime_error("unknown release");}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x1097a990,reinterpret_cast<void*>(&release));std::array<uint32_t,120>vtable{};vtable[0x1dc/4]=uint32_t(&blocked);vtable[0x1cc/4]=uint32_t(&firstReverse);uint32_t model=uint32_t(vtable.data());*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(&model);uint32_t observerVtable=uint32_t(&notify),observer=uint32_t(&observerVtable);using Update=void(__thiscall*)(void*,uint32_t);unsigned cases=0;
for(unsigned seed=0;seed<4096;++seed){fixture=registrationFixture(seed);trace=2166136261u;std::array<uint32_t,40>element{};std::array<uint32_t,64>framework{};std::array<std::array<uint32_t,8>,3>storage{};for(unsigned q=0;q<3;++q){auto*d=framework.data()+(0x64+q*20)/4;d[0]=uint32_t(storage[q].data());d[1]=8;}*reinterpret_cast<uint32_t*>(0x10e9ccf4)=uint32_t(framework.data());element[7]=123;element[15]=0xa500beefu|(uint32_t(fixture.state)<<16);element[16]=fixture.flags;nodes={};std::array<unsigned,6>offsets{0x44,0x48,0x4c,0x54,0x58,0x50};for(unsigned i=0;i<6;++i)if(fixture.present&(1u<<i)){nodes[i][0]=i==5?uint32_t(&observer):0;element[offsets[i]/4]=uint32_t(nodes[i].data());}reinterpret_cast<Update>(0x10360dc0)(element.data(),fixture.enabled);std::array<uint32_t,21>out{};out[0]=element[15];out[1]=element[16];for(unsigned i=0;i<6;++i)out[2+i]=element[offsets[i]/4]!=0;for(unsigned q=0;q<3;++q){out[8+q*4]=framework[(0x64+q*20)/4+3];for(unsigned i=0;i<3;++i)out[9+q*4+i]=storage[q][i];}out[20]=trace;oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_registration_lifecycle_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
