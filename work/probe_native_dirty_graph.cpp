#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


#include "root_mark_fixture.h"
static std::array<std::array<uint32_t,24>,4>elements;static std::array<std::array<uint32_t,5>,8>attrs;static RootFixture fixture;static uint32_t trace;static unsigned seed;
static bool __fastcall blocked(void*,void*){rootTrace(trace,1);return(seed&16)!=0;}
static void __fastcall queue(void*,void*,uint32_t h){rootTrace(trace,10+h);}
static void __fastcall cacheQueue(void*,void*,uint32_t h){rootTrace(trace,20+h);}
static uint32_t __fastcall firstReverse(void*,void*,uint32_t h){rootTrace(trace,30+h);return h*2+1;}
static void* __fastcall attribute(void*,void*,uint32_t token){rootTrace(trace,40+token);unsigned k=token-1;return fixture.handles[k][1]==5?nullptr:attrs[k].data();}
static uint32_t __fastcall nextReverse(void*,void*,uint32_t token){rootTrace(trace,50+token);return token&1?token+1:0;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x1038f580,reinterpret_cast<void*>(&queue));redirect(0x1038f5e0,reinterpret_cast<void*>(&cacheQueue));std::array<uint32_t,120>vtable{};vtable[0x1dc/4]=uint32_t(&blocked);vtable[0x1cc/4]=uint32_t(&firstReverse);vtable[0x164/4]=uint32_t(&attribute);vtable[0x1d0/4]=uint32_t(&nextReverse);uint32_t model=uint32_t(vtable.data());*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(&model);using Dirty=void(__thiscall*)(void*,uint32_t);unsigned cases=0;
for(seed=0;seed<4096;++seed){fixture=rootFixture(seed);trace=2166136261u;for(unsigned i=0;i<4;++i){elements[i]={};elements[i][7]=i;elements[i][15]=0xa500beefu|(uint32_t(fixture.masks[i])<<16);elements[i][16]=0x87654320u|(fixture.masks[(i+1)%4]&15);elements[i][17]=fixture.accepts[i]?1:0;for(unsigned j=0;j<2;++j){unsigned k=i*2+j;attrs[k]={};attrs[k][4]=fixture.flags[k];auto h=fixture.handles[k][0];attrs[k][2]=h<4?uint32_t(elements[h].data()):0;}}reinterpret_cast<Dirty>(0x1035e400)(elements[0].data(),seed%4);std::array<uint32_t,9>out{};for(unsigned i=0;i<4;++i){out[i*2]=elements[i][15];out[i*2+1]=elements[i][16];}out[8]=trace;oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_dirty_graph_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
