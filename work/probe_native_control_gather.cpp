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



static std::array<uint32_t,64> storage{},slaveList{},calls{};
static unsigned slaveCount=0,callCount=0;static bool transformPresent=false;
static void* __fastcall resolveTransform(void*,void*,uint32_t){return transformPresent?reinterpret_cast<void*>(10):nullptr;}
static void __fastcall release(void*,void*,void*){}
static void __cdecl collectChannels(uint32_t*out,void*source,uint32_t name,bool filter,int mode){if(name!=0xffffffffu||filter||mode)throw std::runtime_error("unexpected gather filters");uint32_t id=uint32_t(source);calls[callCount++]=id;if(id){storage[out[3]++]=id%3+100;storage[out[3]++]=id%3+100;}}
static void __cdecl collectSlaves(uint32_t*out,void*,uint32_t name,bool filter,int mode){if(name!=0xffffffffu||filter||mode)throw std::runtime_error("unexpected slave filters");out[0]=uint32_t(slaveList.data());out[1]=64;out[2]=0;out[3]=slaveCount;out[4]=out[0];}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");





 std::array<uint32_t,6> av{};av[5]=uint32_t(&release);uint32_t ao=uint32_t(av.data()),ap=uint32_t(&ao);*reinterpret_cast<uint32_t*>(0x10A7E484)=uint32_t(&ap);
 std::array<uint32_t,19> rv{};rv[18]=uint32_t(&resolveTransform);uint32_t ro=uint32_t(rv.data());*reinterpret_cast<uint32_t*>(0x10E9A51C)=uint32_t(&ro);
 redirect(0x101E3200,reinterpret_cast<void*>(&collectChannels));redirect(0x103F86B0,reinterpret_cast<void*>(&collectSlaves));
 using Gather=void(__thiscall*)(void*,uint32_t*);auto gather=reinterpret_cast<Gather>(0x104C6510);std::array<uint32_t,64>dag{};unsigned cases=0;
 for(slaveCount=0;slaveCount<=4;++slaveCount)for(unsigned mask=0;mask<(1u<<slaveCount);++mask)for(unsigned variant=0;variant<4;++variant)for(unsigned present=0;present<2;++present){transformPresent=present!=0;for(unsigned i=0;i<slaveCount;++i)slaveList[i]=(mask&(1u<<i))?0:20+(i+variant)%3;callCount=0;storage.fill(0);std::array<uint32_t,5>out{uint32_t(storage.data()),64,0,0,uint32_t(storage.data())};gather(dag.data(),out.data());std::array<uint32_t,32>record{};record[0]=out[3];record[1]=callCount;for(unsigned i=0;i<out[3];++i)record[2+i]=storage[i];for(unsigned i=0;i<callCount;++i)record[18+i]=calls[i];oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));++cases;}
 std::cout<<"native_control_gather_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
