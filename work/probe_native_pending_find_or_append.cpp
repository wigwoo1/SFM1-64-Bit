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


static uint32_t bucket,index,result,findCalls,appendCalls,seenIndex;static bool matched;
static bool __fastcall findEntry(void*,void*,const void*,uint32_t*b,uint32_t*i){++findCalls;*b=bucket;if(matched)*i=index;return matched;}
static uint32_t __fastcall appendEntry(void*,void*,uint32_t i,const void*){++appendCalls;seenIndex=i;return result;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x10354D00,reinterpret_cast<void*>(&findEntry));redirect(0x10357880,reinterpret_cast<void*>(&appendEntry));
using Insert=uint32_t(__thiscall*)(void*,const void*);auto insert=reinterpret_cast<Insert>(0x1035ACA0);
std::array<std::array<uint32_t,5>,4> descriptors{};std::array<uint32_t,10> table{},key{};table[0]=uint32_t(descriptors.data());unsigned cases=0;
for(bucket=0;bucket<4;++bucket)for(index=0;index<8;++index)for(result=0;result<8;++result)for(unsigned hit=0;hit<2;++hit){matched=hit!=0;for(unsigned b=0;b<4;++b)descriptors[b][3]=b+9;findCalls=appendCalls=0;seenIndex=0xffffffffu;uint32_t actual=insert(table.data(),key.data());std::array<uint32_t,4> record{actual,findCalls,appendCalls,seenIndex};oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));++cases;}
std::cout<<"native_pending_find_or_append_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
