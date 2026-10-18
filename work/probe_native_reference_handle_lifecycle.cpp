#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static std::array<std::array<uint32_t,64>,3> slaves{},dags{},transforms{};
static std::array<float,3> target{},pole{};
static void* __fastcall resolve(void*,void*,uint32_t h){if(h>=1&&h<=3)return slaves[h-1].data();if(h>=11&&h<=13)return dags[h-11].data();return nullptr;}
static void* __fastcall getTransform(void*self,void*){for(unsigned i=0;i<3;++i)if(self==dags[i].data())return transforms[i].data();return nullptr;}
static void* __fastcall getControl(void*,void*){return nullptr;}
static void __fastcall getFrame(void*,void*,float*out){std::memset(out,0,48);out[0]=out[5]=out[10]=1;}
static float __fastcall getAverage(void*,void*,float*out){std::memcpy(out,target.data(),12);return .75f;}
static void __fastcall getPole(void*,void*,float*out){std::memcpy(out,pole.data(),12);}
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




using Allocate=uint32_t(__thiscall*)(void*);using Bind=void(__thiscall*)(void*,uint32_t,uint32_t);using Retire=void(__thiscall*)(void*,uint32_t);using Resolve=uint32_t(__thiscall*)(void*,uint32_t);auto allocateHandle=reinterpret_cast<Allocate>(0x1035AB20);auto bind=reinterpret_cast<Bind>(0x10354AD0);auto retire=reinterpret_cast<Retire>(0x10356E70);auto resolveHandle=reinterpret_cast<Resolve>(0x10354B20);unsigned cases=0;
for(unsigned sequence=0;sequence<64;++sequence){std::array<std::array<uint32_t,2>,128>slots{};std::array<uint32_t,128>ring{},handles{};unsigned count=0;std::array<uint32_t,11>table{0,uint32_t(slots.data()),128,0,0,uint32_t(slots.data()),uint32_t(ring.data()),128,0,0xffffffffu,0xffffffffu};for(unsigned step=0;step<128;++step){uint32_t result=0;unsigned selection=count?(step*7+sequence)%count:0;if(step%4==0){result=allocateHandle(table.data())&0x7fffffffu;handles[count++]=result;}else if(step%4==1){bind(table.data(),handles[selection],step+123);}else if(step%4==2){retire(table.data(),handles[selection]);}else{result=resolveHandle(table.data(),handles[selection]);}
std::array<uint32_t,5>state{result,table[0],table[4],table[9],table[10]};oracle.write(reinterpret_cast<const char*>(state.data()),sizeof(state));for(unsigned i=0;i<table[4];++i)oracle.write(reinterpret_cast<const char*>(slots[i].data()),8);oracle.write(reinterpret_cast<const char*>(ring.data()),sizeof(ring));++cases;}}
std::cout<<"native_reference_handle_lifecycle_steps="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
