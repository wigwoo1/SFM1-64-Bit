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


static bool liveHit;static uint32_t notices,lastNotice,lastCount;static uint32_t*pendingDescriptors;
static bool __fastcall findLive(void*,void*,const void*,uint32_t*b,uint32_t*i){*b=0;if(liveHit)*i=0;return liveHit;}
static void __fastcall notify(void*,void*,uint32_t h){++notices;lastNotice=h&0x7fffffffu;lastCount=0;for(unsigned b=0;b<16;++b)lastCount+=pendingDescriptors[b*5+3];}
static void __fastcall checkCookie(void*,void*){}
static std::array<std::array<uint32_t,5>,12>liveElements{};
static void* __fastcall resolveLiveElement(void*,void*,uint32_t h){if(h<101||h>112||(h-101)%5==0)return nullptr;return liveElements[h-101].data();}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x10A0E353,reinterpret_cast<void*>(&checkCookie));using Create=uint32_t(__thiscall*)(void*,const void*);auto create=reinterpret_cast<Create>(0x1035B7B0);unsigned cases=0;
std::array<uint32_t,128>vtable{};vtable[0x1bc/4]=uint32_t(&notify);vtable[0x48/4]=uint32_t(&resolveLiveElement);
for(unsigned sequence=0;sequence<64;++sequence){std::array<uint32_t,0xc240/4>object{};object[0]=uint32_t(vtable.data());std::array<std::array<uint32_t,2>,64>slots{};auto*handleTable=object.data()+0xc0c0/4;handleTable[1]=uint32_t(slots.data());handleTable[2]=64;handleTable[9]=handleTable[10]=0xffffffffu;
std::array<std::array<std::array<uint32_t,9>,64>,16>entries{};std::array<std::array<uint32_t,5>,16>desc{};for(unsigned b=0;b<16;++b)desc[b]={uint32_t(entries[b].data()),64,0,0,0};pendingDescriptors=desc[0].data();auto*table=object.data()+0xc1ec/4;table[0]=uint32_t(desc.data());table[3]=16;table[5]=0x103535C0;table[6]=0x1039A2D0;table[7]=1;table[8]=15;
*reinterpret_cast<uint32_t*>(0x10E9A51C)=uint32_t(object.data());std::array<std::array<uint32_t,64>,16>liveEntries{};std::array<std::array<uint32_t,5>,16>liveDesc{};for(unsigned b=0;b<16;++b)liveDesc[b]={uint32_t(liveEntries[b].data()),64,0,0,0};auto*liveTable=object.data()+0xc1c8/4;liveTable[0]=uint32_t(liveDesc.data());liveTable[3]=16;liveTable[5]=0x10362C30;liveTable[6]=0x103534B0;liveTable[7]=1;liveTable[8]=15;using InsertLive=uint32_t(__thiscall*)(void*,const void*);for(unsigned i=0;i<12;++i){liveElements[i]={0,i%4,i,sequence,0x12345678};uint32_t h=101+i;reinterpret_cast<InsertLive>(0x101411E0)(liveTable,&h);}
notices=0;lastNotice=lastCount=0xffffffffu;
for(unsigned step=0;step<64;++step){unsigned id=(step*13+sequence)%23;std::array<uint32_t,4>identity{id%4,id,sequence,0x12345678};uint32_t result=create(object.data(),identity.data())&0x7fffffffu;std::array<uint32_t,7>state{result,object[0xc180/4],handleTable[0],handleTable[4],notices,lastNotice,lastCount};oracle.write(reinterpret_cast<const char*>(state.data()),sizeof(state));for(unsigned b=0;b<16;++b){oracle.write(reinterpret_cast<const char*>(&desc[b][3]),4);for(unsigned i=0;i<desc[b][3];++i){auto e=entries[b][i];e[4]&=0x7fffffffu;oracle.write(reinterpret_cast<const char*>(e.data()),sizeof(e));}}++cases;}}
std::cout<<"native_pending_live_handle_creation_steps="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
