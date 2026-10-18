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




using Insert=uint32_t(__thiscall*)(void*,const void*);auto insert=reinterpret_cast<Insert>(0x1035ACA0);
std::array<std::array<std::array<uint32_t,9>,64>,16> entries{};std::array<std::array<uint32_t,5>,16> desc{};std::array<uint32_t,9> table{};table[0]=uint32_t(desc.data());table[3]=16;table[5]=0x103535C0;table[6]=0x1039A2D0;table[7]=1;table[8]=15;unsigned cases=0;
for(unsigned sequence=0;sequence<64;++sequence){entries={};for(unsigned b=0;b<16;++b){desc[b]={uint32_t(entries[b].data()),64,0,0,0};}for(unsigned step=0;step<128;++step){unsigned id=(step*13+sequence)%23;std::array<uint32_t,9>key{id%4,id,sequence,0x12345678,step+100,step*987654u,step&1,step+200,step+300};uint32_t result;if(step%3==2){unsigned b=(step+sequence)%4,i=(step/3)%8;result=i<desc[b][3];using Erase=void(__thiscall*)(void*,uint32_t);reinterpret_cast<Erase>(0x10356F30)(table.data(),(b<<16)|i);}else result=insert(table.data(),key.data());oracle.write(reinterpret_cast<const char*>(&result),4);for(unsigned b=0;b<16;++b){oracle.write(reinterpret_cast<const char*>(&desc[b][3]),4);for(unsigned i=0;i<desc[b][3];++i)oracle.write(reinterpret_cast<const char*>(entries[b][i].data()),36);}++cases;}}
std::cout<<"native_pending_registry_mutation_steps="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
