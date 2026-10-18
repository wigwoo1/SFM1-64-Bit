#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}




static std::array<uint32_t,4>record{};static std::array<uint32_t,16>element{};static bool present=false;static unsigned context=0,queries=0;
static void* __fastcall lookup(void*,void*,uint32_t h,void*optional){if(h!=7||optional)throw std::runtime_error("record lookup");return present?record.data():nullptr;}
static void* __fastcall resolveTarget(void*,void*,uint32_t h){if(h!=7)throw std::runtime_error("resolve");++queries;return context==2?nullptr:element.data();}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");






 redirect(0x103596d0,reinterpret_cast<void*>(&lookup));std::vector<uint32_t>framework(0xc19c/4+1);std::array<uint32_t,32>vt{};vt[0x48/4]=uint32_t(&resolveTarget);framework[0]=uint32_t(vt.data());using Count=void(__thiscall*)(void*,uint32_t,uint32_t);
 for(unsigned seed=0;seed<24576;++seed){context=seed/2048%3;present=(seed/6144%2)!=0;record={7,(seed%1024)<<20|0xc00abcdeu,(seed/12288%2)?0xfeu:0xffu,0xffffffffu};element[0x38/4]=context==0?0xffffffffu:0;framework[0xc198/4]=0xa5;queries=0;reinterpret_cast<Count>((seed/1024%2)?0x103599a0:0x10359850)(framework.data(),7,2);std::array<uint32_t,4>out{record[1],record[2],framework[0xc198/4],queries};oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));}
 std::cout<<"native_undo_target_count_cases=24576\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
