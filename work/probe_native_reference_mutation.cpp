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



static std::array<std::array<uint32_t,2>,128>pool{};static unsigned used=0;
static void* __fastcall allocateNode(void*,void*,unsigned bytes){if(bytes!=8||used>=pool.size())throw std::runtime_error("unexpected allocation");return pool[used++].data();}
static void __fastcall freeNode(void*,void*,void*){}
static uint32_t __fastcall token(void*self,void*,bool create){if(!create)throw std::runtime_error("unexpected token lookup");return uint32_t(self);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");





 redirect(0x1097AA70,reinterpret_cast<void*>(&allocateNode));redirect(0x1097A990,reinterpret_cast<void*>(&freeNode));redirect(0x10362BD0,reinterpret_cast<void*>(&token));using Mutate=void(__thiscall*)(void*,void*);auto add=reinterpret_cast<Mutate>(0x1035D9E0),remove=reinterpret_cast<Mutate>(0x1035DA30);unsigned cases=0;
 for(unsigned seq=0;seq<512;++seq){std::array<uint32_t,8>record{};record[3]=0xffffffffu;used=0;uint32_t seed=seq+1;for(unsigned step=0;step<16;++step){seed=seed*1664525u+1013904223u;uint32_t value=1+((seed>>16)%5);bool insertion=(seed&0x80000000u)!=0;(insertion?add:remove)(record.data(),reinterpret_cast<void*>(value));std::array<uint32_t,18>out{};auto node=record.data()+3;while(node&&node[0]!=0xffffffffu){if(out[0]>=16)throw std::runtime_error("cycle/list overflow");out[1+out[0]++]=node[0];node=reinterpret_cast<uint32_t*>(node[1]);}oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}}
 std::cout<<"native_reference_mutation_steps="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
