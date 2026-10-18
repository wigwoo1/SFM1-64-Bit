#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}
static void* modelPointer;static unsigned resolveCalls,lookupCalls,bindCalls;static uint32_t lookupResult;
static void* __fastcall resolve(void*,void*,uint32_t handle){if(handle!=0x1234u)throw std::runtime_error("wrong model handle");++resolveCalls;return modelPointer;}
static uint32_t __fastcall lookup(void*,void*){++lookupCalls;return lookupResult;}
static void __fastcall bindFlexDestination(void*op,void*){++bindCalls;reinterpret_cast<uint32_t*>(op)[0x78/4]=0xabcdef01u;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"x86 required");
std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
if(argc!=2||std::ifstream(argv[1]).good())throw std::runtime_error("fresh output required");std::ofstream out(argv[1],std::ios::binary);if(!out)throw std::runtime_error("output failed");
redirect(0x105c2f70,reinterpret_cast<void*>(&lookup));redirect(0x105c2550,reinterpret_cast<void*>(&bindFlexDestination));std::array<uint32_t,32> table{};table[0x48/4]=uint32_t(reinterpret_cast<uintptr_t>(&resolve));uint32_t framework=uint32_t(reinterpret_cast<uintptr_t>(table.data()));*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(reinterpret_cast<uintptr_t>(&framework));using Notify=void(__thiscall*)(void*,void*);
for(unsigned registry=0;registry<2;++registry)for(unsigned matches=0;matches<2;++matches)for(unsigned model=0;model<2;++model)for(uint32_t result:{7u,0xffffffffu}){std::array<uint32_t,40>op{};op[0x6c/4]=0x1234;op[0x74/4]=0x4567;op[0x78/4]=0x87654321;op[0x7c/4]=0x11223344;modelPointer=model?op.data():nullptr;lookupResult=result;resolveCalls=lookupCalls=bindCalls=0;*reinterpret_cast<uint32_t*>(0x10f64470)=registry;reinterpret_cast<Notify>(0x105c3700)(reinterpret_cast<char*>(op.data())+4,reinterpret_cast<void*>(matches?0x4567:0x9999));for(auto v:{op[0x78/4],op[0x7c/4],resolveCalls,lookupCalls,bindCalls})out.write(reinterpret_cast<const char*>(&v),4);}
std::cout<<"native flex binding lifecycle cases=16\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

