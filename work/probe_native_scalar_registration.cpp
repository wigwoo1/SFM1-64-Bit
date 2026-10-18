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



static std::array<std::array<uint32_t,8>,4>attrs{};static std::array<uint32_t,8>obj{},events{};static unsigned gates=0,eventCount=0;
static void* __fastcall attributes(void*,void*){return attrs[0].data();}
static void* __fastcall scalar(void*self,void*){return static_cast<uint32_t*>(self)+6;}
static void* __fastcall resolveTarget(void*,void*,uint32_t){return (gates&1)?obj.data():nullptr;}
static bool __fastcall isElement(void*,void*,uint32_t){return gates&2;}
static bool __fastcall suppressed(void*,void*){return gates&4;}
static bool __fastcall eligible(void*,void*,uint32_t){return gates&8;}
static void* __fastcall getRecord(void*,void*){return obj.data();}
static void __fastcall added(void*,void*,void*attr){events[eventCount++]=1+unsigned((static_cast<uint32_t*>(attr)-attrs[0].data())/8);}
static void __fastcall removed(void*self,void*edx,void*attr){added(self,edx,attr);events[eventCount-1]|=0x100;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");





 std::array<uint32_t,19>vt{};vt[18]=uint32_t(&resolveTarget);uint32_t dm=uint32_t(vt.data());*reinterpret_cast<uint32_t*>(0x10E9A51C)=uint32_t(&dm);std::array<uint32_t,5>ov{};ov[4]=uint32_t(&isElement);obj[0]=uint32_t(ov.data());
 redirect(0x1012C420,reinterpret_cast<void*>(&attributes));redirect(0x10002780,reinterpret_cast<void*>(&scalar));redirect(0x1035DCD0,reinterpret_cast<void*>(&suppressed));redirect(0x1035DC80,reinterpret_cast<void*>(&eligible));redirect(0x1035DC10,reinterpret_cast<void*>(&getRecord));redirect(0x1035D9E0,reinterpret_cast<void*>(&added));redirect(0x1035DA30,reinterpret_cast<void*>(&removed));using Register=void(__cdecl*)(void*,uint32_t,bool);auto registration=reinterpret_cast<Register>(0x103568D0);unsigned cases=0;
 for(unsigned mask=0;mask<16;++mask)for(gates=0;gates<16;++gates)for(unsigned remove=0;remove<2;++remove){for(unsigned i=0;i<4;++i){attrs[i].fill(0);attrs[i][0]=i<3?uint32_t(attrs[i+1].data()):0;attrs[i][4]=(mask&(1u<<i))?1:3;attrs[i][6]=1;}events.fill(0);eventCount=0;registration(obj.data(),7,remove!=0);std::array<uint32_t,8>out{};out[0]=eventCount;for(unsigned i=0;i<eventCount;++i)out[i+1]=events[i];oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
 std::cout<<"native_scalar_registration_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
