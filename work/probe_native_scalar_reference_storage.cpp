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



static std::array<uint32_t,8>events{};static unsigned count=0;
static void __fastcall removeReference(void*,void*,uint32_t target,void*attribute){auto storage=reinterpret_cast<uint32_t*>(static_cast<uint32_t*>(attribute)[1]);events[count++]=1;events[count++]=target;events[count++]=storage[0];}
static void __fastcall addReference(void*,void*,uint32_t target,void*attribute){auto storage=reinterpret_cast<uint32_t*>(static_cast<uint32_t*>(attribute)[1]);events[count++]=2;events[count++]=target;events[count++]=storage[0];}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");





 redirect(0x10359940,reinterpret_cast<void*>(&removeReference));redirect(0x103597E0,reinterpret_cast<void*>(&addReference));using Assign=void(__thiscall*)(void*,const uint32_t*);auto assign=reinterpret_cast<Assign>(0x10365F90);std::array<uint32_t,8>attribute{};std::array<uint32_t,3>storage{};attribute[1]=uint32_t(storage.data());attribute[4]=1;unsigned cases=0;
 for(unsigned old=0;old<17;++old)for(unsigned incoming=0;incoming<17;++incoming){storage[0]=old==16?0xffffffffu:old;uint32_t newValue=incoming==16?0xffffffffu:incoming;events.fill(0);count=0;assign(attribute.data(),&newValue);events[6]=storage[0];events[7]=count;oracle.write(reinterpret_cast<const char*>(events.data()),sizeof(events));++cases;}
 std::cout<<"native_scalar_reference_storage_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
