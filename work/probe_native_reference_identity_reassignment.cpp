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


static void __fastcall checkCookie(void*,void*){}
static void* __fastcall resolveTable(void*self,void*,uint32_t h){using Resolve=void*(__thiscall*)(void*,uint32_t);return reinterpret_cast<Resolve>(0x10354B20)(static_cast<char*>(self)+0xc0c0,h);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x10A0E353,reinterpret_cast<void*>(&checkCookie));using Insert=uint32_t(__thiscall*)(void*,const void*);using Find=bool(__thiscall*)(void*,const void*,uint32_t*,uint32_t*);using Reassign=uint32_t(__thiscall*)(void*,uint32_t,const void*,const void*);unsigned cases=0;
std::array<uint32_t,19>vtable{};vtable[18]=uint32_t(&resolveTable);
for(unsigned pattern=0;pattern<16;++pattern)for(unsigned variant=0;variant<64;++variant){std::array<uint32_t,0xc240/4>object{};object[0]=uint32_t(vtable.data());*reinterpret_cast<uint32_t*>(0x10E9A51C)=uint32_t(object.data());std::array<uint32_t,16>element{},collision{};std::array<uint32_t,4>oldId{1,2,3,4},newId{2,3,4,5};for(unsigned w=0;w<4;++w){element[w+1]=oldId[w];collision[w+1]=newId[w];}element[7]=0;element[8]=0xc0000000u|variant*1337;element[9]=0xa5;element[10]=123;element[11]=456;collision[7]=2;
std::array<std::array<uint32_t,2>,8>slots{};slots[0][1]=uint32_t(element.data());slots[2][1]=uint32_t(collision.data());std::array<uint32_t,8>ring{};auto*ht=object.data()+0xc0c0/4;ht[0]=4;ht[1]=uint32_t(slots.data());ht[2]=8;ht[4]=4;ht[6]=uint32_t(ring.data());ht[7]=8;ht[9]=ht[10]=0xffffffffu;
std::array<std::array<uint32_t,8>,16>liveEntries{};std::array<std::array<uint32_t,5>,16>ld{};std::array<std::array<std::array<uint32_t,9>,8>,16>pendingEntries{};std::array<std::array<uint32_t,5>,16>pd{};for(unsigned b=0;b<16;++b){ld[b]={uint32_t(liveEntries[b].data()),8,0,0,0};pd[b]={uint32_t(pendingEntries[b].data()),8,0,0,0};}auto*lt=object.data()+0xc1c8/4;lt[0]=uint32_t(ld.data());lt[3]=16;lt[5]=0x10362c30;lt[6]=0x103534b0;lt[7]=1;lt[8]=15;auto*pt=object.data()+0xc1ec/4;pt[0]=uint32_t(pd.data());pt[3]=16;pt[5]=0x103535c0;pt[6]=0x1039a2d0;pt[7]=1;pt[8]=15;
uint32_t h=0;if(pattern&1)reinterpret_cast<Insert>(0x101411e0)(lt,&h);h=2;if(pattern&4)reinterpret_cast<Insert>(0x101411e0)(lt,&h);std::array<uint32_t,9>key{2,3,4,5,1,variant*98765,variant&1,321,654};if(pattern&8)reinterpret_cast<Insert>(0x1035aca0)(pt,key.data());uint32_t result=reinterpret_cast<Reassign>(0x1035a440)(object.data(),pattern&2?3:0,oldId.data(),newId.data());std::array<uint32_t,33>out{};out[0]=result;out[1]=object[0xc180/4];out[2]=ht[0];for(unsigned w=0;w<4;++w)out[3+w]=element[1+w];for(unsigned w=0;w<5;++w)out[7+w]=element[7+w];for(unsigned i=0;i<4;++i){out[12+i*2]=slots[i][0];out[13+i*2]=slots[i][1]==uint32_t(element.data())?1:slots[i][1]==uint32_t(collision.data())?2:0;}out[20]=ht[9];out[21]=ht[10];for(unsigned i=0;i<8;++i)out[22+i]=ring[i];for(unsigned query=0;query<2;++query){uint32_t b=0,i=0;out[30+query]=reinterpret_cast<Find>(0x10354710)(lt,query?newId.data():oldId.data(),&b,&i)?liveEntries[b][i]:0xffffffffu;}for(unsigned b=0;b<16;++b)out[32]+=pd[b][3];oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_reference_identity_reassignment_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
