#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


static std::array<std::array<uint32_t,3>,64>nodes;static std::array<uint32_t,68>out;static uint32_t head,originalHead,expectedPool;
static void __fastcall release(void*pool,void*,void*node){if(uint32_t(pool)!=expectedPool||head!=originalHead)throw std::runtime_error("release contract");unsigned index=unsigned(static_cast<uint32_t*>(node)-nodes[0].data())/3;out[2+out[1]++]=index;std::memset(node,0xcc,12);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x1097a990,reinterpret_cast<void*>(&release));std::array<uint32_t,6>functions{0x1035e640,0x1035e680,0x1035e680,0x1035e700,0x1035e740,0x1035e6c0};std::array<uint32_t,6>pools{0x10f49a28,0x10f49a58,0x10f49a58,0x10f49b40,0x10f49ba0,0x10f49b70};using Clear=void(__thiscall*)(void*);unsigned cases=0;
for(unsigned kind=0;kind<6;++kind)for(unsigned length=0;length<=64;++length){nodes={};for(unsigned i=0;i<length;++i){unsigned index=(i*17)%64;unsigned next=((i+1)*17)%64;nodes[index][kind<3?2:1]=i+1<length?uint32_t(nodes[next].data()):0;}head=length?uint32_t(nodes[0].data()):0;originalHead=head;expectedPool=pools[kind];out={};reinterpret_cast<Clear>(functions[kind])(&head);out[0]=head;oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_cache_list_clear_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
