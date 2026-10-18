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


static uint32_t hashes,equals;
static uint32_t __cdecl hashKey(const uint32_t*k){++hashes;return k[0];}
static bool __cdecl equalKey(const uint32_t*e,const uint32_t*k){++equals;return e[0]==k[1];}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




using Find=bool(__thiscall*)(void*,const void*,uint32_t*,uint32_t*);auto find=reinterpret_cast<Find>(0x10354D00);
std::array<std::array<std::array<uint32_t,9>,6>,4> entries{};std::array<std::array<uint32_t,5>,4> desc{};std::array<uint32_t,9> table{};table[0]=uint32_t(desc.data());table[5]=uint32_t(&equalKey);table[6]=uint32_t(&hashKey);table[8]=3;unsigned cases=0;
for(unsigned mode=0;mode<2;++mode)for(uint32_t hash=0;hash<32;++hash)for(unsigned count=0;count<=6;++count)for(unsigned pattern=0;pattern<64;++pattern){table[7]=mode;table[3]=mode?4:3;for(unsigned b=0;b<4;++b){desc[b][0]=uint32_t(entries[b].data());desc[b][3]=count;for(unsigned i=0;i<6;++i)entries[b][i][0]=(pattern&(1u<<i))?77:88;}
std::array<uint32_t,2>key{hash,77};uint32_t bucket=0xffffffffu,index=0xabcdef01u;hashes=equals=0;bool hit=find(table.data(),key.data(),&bucket,&index);std::array<uint32_t,5>r{uint32_t(hit),bucket,index,hashes,equals};oracle.write(reinterpret_cast<const char*>(r.data()),sizeof(r));++cases;}
std::cout<<"native_pending_identity_lookup_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
