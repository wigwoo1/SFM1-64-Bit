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


static std::array<uint32_t,35>out;static std::array<uint32_t,16>element;static unsigned pattern;
static void event(uint32_t code,uint32_t a=0,uint32_t b=0,uint32_t c=0){unsigned i=out[2]++;out[3+i*4]=code;out[4+i*4]=a;out[5+i*4]=b;out[6+i*4]=c;}
static void* __fastcall resolveElement(void*,void*,uint32_t){return pattern&1?element.data():nullptr;}
static uint32_t __fastcall first(void*,void*){return 1;}
static uint32_t __fastcall next(void*,void*,uint32_t){return 0xffffffffu;}
static uint32_t __fastcall dependencyCount(void*,void*){return 0;}
static void __fastcall mark(void*self,void*,uint32_t a,uint32_t b,uint32_t c){event(1,a,b,c);if(pattern&128){auto*s=static_cast<uint16_t*>(self);s[0x3c/2]=(s[0x3c/2]&~3u)|2;}}
static void __cdecl registerReferences(void*,uint32_t context,uint32_t enabled){event(2,context,enabled);}
static void __fastcall update(void*,void*,uint32_t enabled){event(3,enabled);}
static void __fastcall notifyOff(void*,void*){event(4,0);}
static void __fastcall notifyOn(void*,void*){event(4,1);}
static void __fastcall removeElement(void*,void*,uint32_t h,uint32_t mode){event(5,h,mode);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x1035F0D0,reinterpret_cast<void*>(&mark));redirect(0x103568D0,reinterpret_cast<void*>(&registerReferences));redirect(0x10360DC0,reinterpret_cast<void*>(&update));redirect(0x1035B220,reinterpret_cast<void*>(&removeElement));std::array<uint32_t,96>vtable{};vtable[18]=uint32_t(&resolveElement);vtable[0xe4/4]=uint32_t(&first);vtable[0xe8/4]=uint32_t(&next);vtable[0x170/4]=uint32_t(&dependencyCount);std::array<uint32_t,14>elementVtable{};elementVtable[12]=uint32_t(&notifyOff);elementVtable[13]=uint32_t(&notifyOn);using Pass=void(__thiscall*)(void*);unsigned cases=0;
for(pattern=0;pattern<256;++pattern){std::array<uint32_t,0xc240/4>framework{};framework[0]=uint32_t(vtable.data());element={};element[0]=uint32_t(elementVtable.data());element[8]=pattern&8?0x400:0;element[9]=pattern&4?1:0;element[14]=pattern&2?0:0xffffffffu;element[15]=(pattern&16?1:0)|(pattern&32?2:0)|(pattern&64?0x80000:0);out={};reinterpret_cast<Pass>(0x1035b920)(framework.data());out[0]=element[15]&0xffff;out[1]=(element[15]>>16)&0xff;oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_cleanup_iterator_phase_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
