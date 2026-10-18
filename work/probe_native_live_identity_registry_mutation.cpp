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


static std::array<std::array<uint32_t,5>,24>elements{};
static void* __fastcall resolveElement(void*,void*,uint32_t h){return h%5?elements.at(h).data():nullptr;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




std::array<uint32_t,19>vtable{};vtable[0x48/4]=uint32_t(&resolveElement);uint32_t object=uint32_t(vtable.data());*reinterpret_cast<uint32_t*>(0x10E9A51C)=uint32_t(&object);
using Insert=uint32_t(__thiscall*)(void*,const void*);using Find=bool(__thiscall*)(void*,const void*,uint32_t*,uint32_t*);auto insert=reinterpret_cast<Insert>(0x101411E0);auto find=reinterpret_cast<Find>(0x10354710);unsigned cases=0;
for(unsigned h=1;h<24;++h)elements[h]={0,h%4,h%3,17,29};
for(unsigned sequence=0;sequence<64;++sequence){std::array<std::array<uint32_t,64>,16>entries{};std::array<std::array<uint32_t,5>,16>desc{};for(unsigned b=0;b<16;++b)desc[b]={uint32_t(entries[b].data()),64,0,0,0};std::array<uint32_t,9>table{};table[0]=uint32_t(desc.data());table[3]=16;table[5]=0x10362C30;table[6]=0x103534B0;table[7]=1;table[8]=15;
for(unsigned step=0;step<128;++step){uint32_t h=1+(step*13+sequence)%23;uint32_t result,bucket=0,index=0,found=0xffffffffu;if(step%3==2){unsigned b=(step+sequence)%4,i=(step/3)%8;result=i<desc[b][3];using Erase=void(__thiscall*)(void*,uint32_t);reinterpret_cast<Erase>(0x10354A80)(table.data(),(b<<16)|i);}else result=insert(table.data(),&h);if(find(table.data(),elements[h].data()+1,&bucket,&index))found=entries[bucket][index];std::array<uint32_t,2>state{result,found};oracle.write(reinterpret_cast<const char*>(state.data()),sizeof(state));for(unsigned b=0;b<16;++b){oracle.write(reinterpret_cast<const char*>(&desc[b][3]),4);oracle.write(reinterpret_cast<const char*>(entries[b].data()),desc[b][3]*4);}++cases;}}
std::cout<<"native_live_identity_registry_mutation_steps="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
