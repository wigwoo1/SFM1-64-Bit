#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}
static void* modelPointer;static void* attributePointer;static std::vector<uint32_t>events;
static void* __fastcall resolve(void*,void*,uint32_t){events.push_back(2);return modelPointer;}
static uint32_t __fastcall lookup(void*,void*){events.push_back(1);return 7;}
static void* __fastcall findFlexWeights(void*,void*,const char*name){if(strcmp(name,"flexWeights"))throw std::runtime_error("wrong attribute");events.push_back(3);return attributePointer;}
static uint32_t __fastcall cacheAttribute(void*,void*,unsigned retain){if(retain!=1)throw std::runtime_error("wrong retain flag");events.push_back(4);return 0x4567;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"x86 required");
std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
if(argc!=2||std::ifstream(argv[1]).good())throw std::runtime_error("fresh output required");std::ofstream out(argv[1],std::ios::binary);if(!out)throw std::runtime_error("output failed");
redirect(0x105c2f70,reinterpret_cast<void*>(&lookup));redirect(0x1035dd50,reinterpret_cast<void*>(&findFlexWeights));redirect(0x10362bd0,reinterpret_cast<void*>(&cacheAttribute));std::array<uint32_t,32>table{};table[0x48/4]=uint32_t(reinterpret_cast<uintptr_t>(&resolve));uint32_t framework=uint32_t(reinterpret_cast<uintptr_t>(table.data()));*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(reinterpret_cast<uintptr_t>(&framework));using Prepare=void(__thiscall*)(void*);
for(uint32_t index:{0xffffffffu,0x80000000u,0u,7u})for(uint32_t destination:{0xffffffffu,0x1234u})for(unsigned model=0;model<2;++model)for(unsigned attribute=0;attribute<2;++attribute){std::array<uint32_t,40>op{};op[0x7c/4]=index;op[0x78/4]=destination;modelPointer=model?op.data():nullptr;attributePointer=attribute?op.data():nullptr;events.clear();reinterpret_cast<Prepare>(0x105c36b0)(reinterpret_cast<char*>(op.data())+4);for(auto v:{op[0x78/4],op[0x7c/4],uint32_t(events.size())})out.write(reinterpret_cast<const char*>(&v),4);for(auto v:events)out.write(reinterpret_cast<const char*>(&v),4);}
std::cout<<"native lazy flex binding cases=32\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

