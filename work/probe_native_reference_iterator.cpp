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



static std::array<uint32_t,8> record{};static bool present=false;
static void* __fastcall getRecord(void*,void*,uint32_t,bool create){if(create)throw std::runtime_error("iterator allocated record");return present?record.data():nullptr;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");





 redirect(0x103596D0,reinterpret_cast<void*>(&getRecord));using First=void*(__thiscall*)(void*,uint32_t);using Next=void*(__thiscall*)(void*,void*);auto first=reinterpret_cast<First>(0x10359A80);auto next=reinterpret_cast<Next>(0x10353A60);std::array<uint32_t,4>node{};unsigned cases=0;
 for(unsigned i=0;i<1024;++i){present=i&1;record[3]=(i&2)?0xffffffffu:i;node[1]=(i&4)?uint32_t(record.data()+3):0;void*p=first(nullptr,i);void*q=next(nullptr,(i&8)?nullptr:node.data());std::array<uint32_t,2>out{p?1u:0u,q?1u:0u};if(p&&p!=record.data()+3)throw std::runtime_error("first token offset mismatch");if(q&&q!=record.data()+3)throw std::runtime_error("next link mismatch");oracle.write(reinterpret_cast<const char*>(out.data()),8);++cases;}
 std::cout<<"native_reference_iterator_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
