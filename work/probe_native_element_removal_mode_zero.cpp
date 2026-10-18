#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


static unsigned pattern;static std::array<uint32_t,8>attribute;static std::array<uint32_t,32>out;
static void event(uint32_t code){out[2+out[1]++]=code;}
static uint32_t __fastcall phase(void*,void*){event(1);return pattern%4;}
static uint32_t __fastcall firstReverse(void*,void*,uint32_t){event(2);return pattern&32?1:0;}
static void* __fastcall getAttribute(void*,void*,uint32_t){event(3);return pattern&64?nullptr:attribute.data();}
static uint32_t __fastcall nextReverse(void*,void*,uint32_t){event(5);return 0;}
static bool __fastcall blocked(void*,void*){event(4);return true;}
static bool __fastcall lookup(void*,void*,void*,uint32_t*,uint32_t*){event(8);return false;}
static void __fastcall beforeDestroy(void*,void*){event(9);}
static void __fastcall finalize(void*,void*){event(10);}
static void __fastcall destroy(void*,void*,uint32_t){event(11);}
static uint32_t __fastcall appendFree(void*,void*,const uint32_t*){event(12);return 0;}
static void __cdecl cookie(uint32_t){}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x10354710,reinterpret_cast<void*>(&lookup));redirect(0x1035f900,reinterpret_cast<void*>(&finalize));redirect(0x10354c10,reinterpret_cast<void*>(&appendFree));redirect(0x10a0e353,reinterpret_cast<void*>(&cookie));std::array<uint32_t,120>vtable{};vtable[0x1dc/4]=uint32_t(&blocked);vtable[0x1cc/4]=uint32_t(&firstReverse);vtable[0x164/4]=uint32_t(&getAttribute);vtable[0x1d0/4]=uint32_t(&nextReverse);uint32_t model=uint32_t(vtable.data());*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(&model);std::array<uint32_t,9>phaseVtable{};phaseVtable[8]=uint32_t(&phase);uint32_t phaseModel=uint32_t(phaseVtable.data());*reinterpret_cast<uint32_t*>(0x10e9ccf8)=uint32_t(&phaseModel);std::array<uint32_t,12>elementVtable{};elementVtable[11]=uint32_t(&beforeDestroy);std::array<uint32_t,2>factoryVtable{0,uint32_t(&destroy)};uint32_t factory=uint32_t(factoryVtable.data());using Remove=void(__thiscall*)(void*,uint32_t,uint32_t);unsigned cases=0;
for(pattern=0;pattern<1024;++pattern){std::array<uint32_t,0xc240/4>framework{};std::array<uint32_t,24>element{},owner{};std::array<uint32_t,2>slot{};std::array<uint32_t,8>queue{};uint32_t generation=pattern&512?511:7;uint32_t handle=(generation+(pattern&4?1:0))<<22;if(pattern&2)handle=0xffffffffu;slot={generation|(pattern&8?0x80000000u:0),pattern&16?0:uint32_t(element.data())};framework[0xc0c0/4]=1;framework[0xc0c4/4]=uint32_t(slot.data());framework[0xc0d0/4]=1;framework[0xc198/4]=4;framework[0x40/4]=uint32_t(&factory);framework[0x64/4]=uint32_t(queue.data());framework[0x68/4]=8;*reinterpret_cast<uint32_t*>(0x10e9ccf4)=uint32_t(framework.data());element[0]=uint32_t(elementVtable.data());element[7]=handle;element[10]=0xffffffffu;owner[7]=123;owner[15]=pattern&256?0x10000u:0;attribute={};attribute[2]=uint32_t(owner.data());attribute[4]=pattern&128?0x20:0x2000;out={};reinterpret_cast<Remove>(0x1035b220)(framework.data(),handle,0);out[0]=slot[0];out[24]=slot[1]!=0;out[25]=framework[0xc0c0/4];out[26]=framework[0xc180/4];out[27]=attribute[4];out[28]=owner[15];out[29]=framework[0x70/4];out[30]=queue[0];oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_mode_zero_removal_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
