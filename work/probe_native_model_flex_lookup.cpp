#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}
static std::vector<uint32_t> weights;
static unsigned appends,removes;
static uint32_t* countOf(void* wrapper){return reinterpret_cast<uint32_t*>(static_cast<char*>(wrapper)+12);}
static void __fastcall append(void* wrapper,void*,const uint32_t* v){if(*v!=0)throw std::runtime_error("nonzero append");weights.push_back(*v);++*countOf(wrapper);++appends;}
static void __fastcall removeLast(void* wrapper,void*,unsigned index){if(index+1!=weights.size())throw std::runtime_error("non-tail removal");weights.pop_back();--*countOf(wrapper);++removes;}
static void* modelPointer;static void* headerPointer;static unsigned registryCalls;
static void* __fastcall resolve(void*,void*,uint32_t){return modelPointer;}
static void* __fastcall header(void*,void*,uint32_t){return headerPointer;}
static uint32_t __fastcall registerName(void*,void*,const char*name){++registryCalls;return !strcmp(name,"Alpha")?7u:(!strcmp(name,"Beta")?20u:4u);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"x86 required");
std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
if(argc!=2||std::ifstream(argv[1]).good())throw std::runtime_error("fresh output required");std::ofstream out(argv[1],std::ios::binary);if(!out)throw std::runtime_error("output failed");
redirect(0x1036f980,reinterpret_cast<void*>(&append));redirect(0x10385720,reinterpret_cast<void*>(&removeLast));
std::array<uint32_t,32> table{},cache{},registry{};table[0x48/4]=uint32_t(reinterpret_cast<uintptr_t>(&resolve));cache[0x34/4]=uint32_t(reinterpret_cast<uintptr_t>(&header));registry[0]=uint32_t(reinterpret_cast<uintptr_t>(&registerName));uint32_t framework=uint32_t(reinterpret_cast<uintptr_t>(table.data())),modelCache=uint32_t(reinterpret_cast<uintptr_t>(cache.data())),registryObject=uint32_t(reinterpret_cast<uintptr_t>(registry.data()));*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(reinterpret_cast<uintptr_t>(&framework));*reinterpret_cast<uint32_t*>(0x10f4c7f8)=uint32_t(reinterpret_cast<uintptr_t>(&modelCache));
const char* names[]={"Alpha","Beta","Gamma"};const char* queries[]={"alpha","BETA","gamma","Missing"};using Lookup=uint32_t(__thiscall*)(void*);
for(unsigned scenario=0;scenario<6;++scenario)for(unsigned initial:{0u,8u,32u})for(auto query:queries){std::array<uint32_t,96> model{};std::array<uint32_t,40> op{};std::array<unsigned char,512> mdl{};auto put=[&](size_t p,uint32_t v){memcpy(mdl.data()+p,&v,4);};put(0x10c,3);put(0x110,0x120);for(unsigned i=0;i<3;++i){auto p=0x120+i*20;auto stringAt=0x180+i*16;put(p+4,stringAt-p);put(p+8,scenario==5?(i==0?7u:i==1?20u:4u):0xffffffffu);memcpy(mdl.data()+stringAt,names[i],strlen(names[i])+1);}model[0xc8/4]=initial;*reinterpret_cast<unsigned short*>(reinterpret_cast<char*>(model.data())+0x128)=scenario==1?0xffff:1;op[0x6c/4]=1;op[0x18/4]=uint32_t(reinterpret_cast<uintptr_t>(query));modelPointer=scenario==0?nullptr:model.data();headerPointer=scenario==2?nullptr:mdl.data();*reinterpret_cast<uint32_t*>(0x10f64470)=scenario==3?0:uint32_t(reinterpret_cast<uintptr_t>(&registryObject));weights.clear();for(unsigned i=0;i<initial;++i)weights.push_back(0x3f000000u+i);appends=removes=registryCalls=0;auto index=reinterpret_cast<Lookup>(0x105c2f70)(op.data());for(auto v:{index,model[0xc8/4],registryCalls})out.write(reinterpret_cast<const char*>(&v),4);for(unsigned i=0;i<3;++i)out.write(reinterpret_cast<char*>(mdl.data()+0x120+i*20+8),4);for(auto v:weights)out.write(reinterpret_cast<const char*>(&v),4);}
std::cout<<"native model flex lookup cases=72\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
