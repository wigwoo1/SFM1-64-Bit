#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


#include "owner_preparation_fixture.h"
static OwnerPreparationFixture fixture;static unsigned blockCalls,phaseCalls,phase;
static uint32_t __fastcall getPhase(void*,void*){++phaseCalls;return phase;}
static bool __fastcall blocked(void*,void*){++blockCalls;return fixture.blocked;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




std::array<uint32_t,120>vtable{};vtable[0x1dc/4]=uint32_t(&blocked);uint32_t model=uint32_t(vtable.data());*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(&model);std::array<uint32_t,9>phaseVtable{};phaseVtable[8]=uint32_t(&getPhase);uint32_t phaseModel=uint32_t(phaseVtable.data());*reinterpret_cast<uint32_t*>(0x10e9ccf8)=uint32_t(&phaseModel);using Invalidate=bool(__thiscall*)(void*);unsigned cases=0;
for(unsigned seed=0;seed<4096;++seed){fixture=ownerPreparationFixture(seed);blockCalls=phaseCalls=0;phase=(seed/16)%8;std::array<uint32_t,24>owner{};owner[7]=123;owner[15]=0xa500beefu|((seed&255)<<16);std::array<uint32_t,8>attribute{};attribute[2]=uint32_t(owner.data());attribute[4]=0xabcd0000u|(((seed+1)*1664525u+1013904223u)>>16);std::array<std::array<uint32_t,3>,32>nodes{};for(unsigned i=0;i<32;++i){nodes[i][0]=i;nodes[i][1]=fixture.keys[i]==fixture.key?uint32_t(attribute.data()):fixture.keys[i]+1;}for(unsigned i=0;i<fixture.sourceLength;++i)nodes[i][2]=i+1<fixture.sourceLength?uint32_t(nodes[i+1].data()):0;for(unsigned i=0;i<fixture.destinationLength;++i)nodes[16+i][2]=i+1<fixture.destinationLength?uint32_t(nodes[17+i].data()):0;owner[0x48/4]=fixture.sourceLength?uint32_t(nodes[0].data()):0;owner[0x4c/4]=fixture.destinationLength?uint32_t(nodes[16].data()):0;std::array<uint32_t,64>framework{};std::array<uint32_t,8>queue{},registrationQueue{};framework[0x64/4]=uint32_t(registrationQueue.data());framework[0x68/4]=8;framework[0x8c/4]=uint32_t(queue.data());framework[0x90/4]=8;*reinterpret_cast<uint32_t*>(0x10e9ccf4)=uint32_t(framework.data());bool result=reinterpret_cast<Invalidate>(0x103623b0)(attribute.data());std::array<uint32_t,48>out{};for(unsigned which=0;which<2;++which){auto*node=reinterpret_cast<uint32_t*>(owner[(0x48+which*4)/4]);unsigned baseIndex=which?17:0;while(node){unsigned count=out[baseIndex]++;if(count>=21)throw std::runtime_error("cycle");out[baseIndex+1+count]=node[0];node=reinterpret_cast<uint32_t*>(node[2]);}}out[39]=framework[0x98/4];out[40]=queue[0];out[41]=blockCalls;out[42]=attribute[4];out[43]=owner[15];out[44]=result;out[45]=framework[0x70/4];out[46]=registrationQueue[0];out[47]=phaseCalls;oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_incoming_preparation_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
