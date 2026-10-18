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
static std::array<std::array<uint32_t,16>,4> elements;
static RootFixture fixture;static uint32_t trace;
static void* __fastcall resolveRoot(void*,void*,uint32_t h){rootTrace(trace,h+10);return h<4?elements[h].data():nullptr;}
static bool __fastcall acceptRoot(void*self,void*,uint32_t symbol){if(symbol!=123)throw std::runtime_error("class symbol mismatch");for(unsigned i=0;i<4;++i)if(self==elements[i].data()){rootTrace(trace,i+20);return fixture.accepts[i];}throw std::runtime_error("unknown element");}
static void* __fastcall scalarStorage(void*self,void*){return reinterpret_cast<char*>(self)+20;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x10002780,reinterpret_cast<void*>(&scalarStorage));
std::array<uint32_t,20> modelVtable{};modelVtable[18]=uint32_t(&resolveRoot);uint32_t model=uint32_t(modelVtable.data());*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(&model);*reinterpret_cast<uint32_t*>(0x10e9a684)=123;
std::array<uint32_t,5> elementVtable{};elementVtable[4]=uint32_t(&acceptRoot);
using Mark=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t);unsigned cases=0;
for(unsigned seed=0;seed<4096;++seed){fixture=rootFixture(seed);trace=2166136261u;std::array<std::array<uint32_t,8>,8>attrs{};std::array<std::array<uint32_t,4>,8>arrays{};
for(unsigned i=0;i<4;++i){elements[i]={};elements[i][0]=uint32_t(elementVtable.data());elements[i][12]=uint32_t(attrs[i*2].data());elements[i][15]=0xa5080000u|fixture.masks[i];for(unsigned j=0;j<2;++j){auto k=i*2+j;attrs[k][0]=j?0:uint32_t(attrs[k+1].data());attrs[k][4]=fixture.flags[k];attrs[k][5]=fixture.handles[k][0];arrays[k][0]=uint32_t(fixture.handles[k].data());arrays[k][3]=3;attrs[k][1]=uint32_t(arrays[k].data());}}
reinterpret_cast<Mark>(0x1035f0d0)(elements[0].data(),fixture.enabled,fixture.mode,fixture.context);std::array<uint32_t,5>result{};for(unsigned i=0;i<4;++i)result[i]=elements[i][15];result[4]=trace;oracle.write(reinterpret_cast<const char*>(result.data()),sizeof(result));++cases;}
std::cout<<"native_root_mark_graph_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
