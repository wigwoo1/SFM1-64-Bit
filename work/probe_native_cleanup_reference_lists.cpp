#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


#include "root_mark_fixture.h"
static std::array<std::array<uint32_t,24>,4> elements;
static RootFixture fixture;static uint32_t trace;
static void* __fastcall resolveRoot(void*,void*,uint32_t h){rootTrace(trace,h+10);return h<4?elements[h].data():nullptr;}
static bool __fastcall acceptRoot(void*self,void*,uint32_t symbol){if(symbol!=123)throw std::runtime_error("class symbol mismatch");for(unsigned i=0;i<4;++i)if(self==elements[i].data()){rootTrace(trace,i+20);return fixture.accepts[i];}throw std::runtime_error("unknown element");}
static void* __fastcall scalarStorage(void*self,void*){return reinterpret_cast<char*>(self)+20;}
static unsigned seed;
static unsigned indexOf(void*self){for(unsigned i=0;i<4;++i)if(self==elements[i].data())return i;throw std::runtime_error("unknown element");}
static uint32_t __fastcall first(void*,void*){return 0;}
static uint32_t __fastcall next(void*,void*,uint32_t h){return h<3?h+1:0xffffffffu;}
static uint32_t __fastcall dependencyCount(void*,void*){return 2;}
static uint32_t __fastcall dependencyItem(void*,void*,uint32_t i){return i==0&&(seed&1)?0xffffffffu:i;}
static uint32_t __fastcall dependencyHandle(void*,void*,uint32_t i){return i?5:0;}
static bool __fastcall blocked(void*,void*){rootTrace(trace,70);return true;}
static void __cdecl registerReferences(void*self,uint32_t context,uint32_t enabled){if(context)throw std::runtime_error("context");rootTrace(trace,80+indexOf(self)*2+enabled);}
static void __fastcall notifyOff(void*self,void*){rootTrace(trace,90+indexOf(self)*2);}
static void __fastcall notifyOn(void*self,void*){rootTrace(trace,91+indexOf(self)*2);}
static void __fastcall removeElement(void*,void*,uint32_t h,uint32_t mode){if(mode)throw std::runtime_error("remove mode");rootTrace(trace,100+h);}
static std::array<std::array<uint32_t,2>,128>pool;static unsigned used;static uintptr_t attributeBase;
static void* __fastcall allocateNode(void*,void*,uint32_t bytes){if(bytes!=8||used>=128)throw std::runtime_error("node allocation");return pool[used++].data();}
static void __fastcall freeNode(void*,void*,void*){}
static uint32_t __fastcall token(void*self,void*,bool create){if(!create)throw std::runtime_error("token");return uint32_t((uintptr_t(self)-attributeBase)/32+1);}
static void* __fastcall attributesHead(void*self,void*){return reinterpret_cast<uint32_t*>(self)[12]?reinterpret_cast<void*>(reinterpret_cast<uint32_t*>(self)[12]):nullptr;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x10002780,reinterpret_cast<void*>(&scalarStorage));redirect(0x1012c420,reinterpret_cast<void*>(&attributesHead));redirect(0x1097aa70,reinterpret_cast<void*>(&allocateNode));redirect(0x1097a990,reinterpret_cast<void*>(&freeNode));redirect(0x10362bd0,reinterpret_cast<void*>(&token));redirect(0x1035b220,reinterpret_cast<void*>(&removeElement));
std::array<uint32_t,120>modelVtable{};modelVtable[18]=uint32_t(&resolveRoot);modelVtable[0xe4/4]=uint32_t(&first);modelVtable[0xe8/4]=uint32_t(&next);modelVtable[0x170/4]=uint32_t(&dependencyCount);modelVtable[0x178/4]=uint32_t(&dependencyItem);modelVtable[0x194/4]=uint32_t(&dependencyHandle);modelVtable[0x1dc/4]=uint32_t(&blocked);uint32_t model=uint32_t(modelVtable.data());*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(&model);*reinterpret_cast<uint32_t*>(0x10e9a684)=123;
std::array<uint32_t,14>elementVtable{};elementVtable[4]=uint32_t(&acceptRoot);elementVtable[12]=uint32_t(&notifyOff);elementVtable[13]=uint32_t(&notifyOn);using Pass=void(__thiscall*)(void*);unsigned cases=0;
for(seed=0;seed<4096;++seed){fixture=rootFixture(seed);trace=2166136261u;std::array<std::array<uint32_t,8>,8>attrs{};std::array<std::array<uint32_t,4>,8>arrays{};attributeBase=uintptr_t(attrs.data());used=0;pool={};std::array<uint32_t,0xc240/4>framework{};framework[0]=uint32_t(modelVtable.data());std::array<std::array<uint32_t,2>,4>slots{};framework[0xc0c4/4]=uint32_t(slots.data());framework[0xc0d0/4]=4;std::array<uint32_t,8>queue{};framework[0x64/4]=uint32_t(queue.data());framework[0x68/4]=8;*reinterpret_cast<uint32_t*>(0x10e9ccf4)=uint32_t(framework.data());
for(unsigned i=0;i<4;++i){auto m=fixture.masks[i];elements[i]={};elements[i][0]=uint32_t(elementVtable.data());elements[i][7]=i;elements[i][8]=(m&4?0x100000:0)|(m&8?0x400:0);elements[i][9]=m&16?1:0;elements[i][10]=0xffffffffu;elements[i][12]=uint32_t(attrs[i*2].data());elements[i][14]=m&32?0:0xffffffffu;elements[i][15]=0xa5000000u|(uint32_t(m)<<16)|m;elements[i][16]=0x87654320u|(m&15);slots[i]={0,uint32_t(elements[i].data())};for(unsigned j=0;j<2;++j){auto k=i*2+j;attrs[k][0]=j?0:uint32_t(attrs[k+1].data());attrs[k][4]=fixture.flags[k];attrs[k][5]=fixture.handles[k][0];arrays[k][0]=uint32_t(fixture.handles[k].data());arrays[k][3]=3;attrs[k][1]=uint32_t(arrays[k].data());}}
reinterpret_cast<Pass>(0x1035b920)(framework.data());std::array<uint32_t,86>result{};for(unsigned i=0;i<4;++i){result[i*2]=elements[i][15];result[i*2+1]=elements[i][16];}result[8]=framework[0x70/4];for(unsigned i=0;i<4;++i)result[9+i]=queue[i];result[13]=trace;for(unsigned i=0;i<4;++i){auto*node=elements[i].data()+10;unsigned count=0;while(node&&node[0]!=0xffffffffu){if(count>=16)throw std::runtime_error("list overflow");result[15+i*18+count++]=node[0];node=reinterpret_cast<uint32_t*>(node[1]);}result[14+i*18]=count;}oracle.write(reinterpret_cast<const char*>(result.data()),sizeof(result));++cases;}
std::cout<<"native_cleanup_reference_list_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
