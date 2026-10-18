#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


static std::array<uint32_t,24>element;static std::array<std::array<uint32_t,2>,16>nodes,observers;static std::array<uint32_t,20>out;static unsigned pattern;
static bool __fastcall blocked(void*,void*){return(pattern&1)!=0;}
static void __fastcall notify(void*self,void*,void*e){if(e!=element.data())throw std::runtime_error("element mismatch");out[1+out[0]++]=static_cast<uint32_t*>(self)[1];if(pattern&2){element[0x50/4]=0;for(auto&node:nodes)node={};}}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




std::array<uint32_t,120>vtable{};vtable[0x1dc/4]=uint32_t(&blocked);uint32_t model=uint32_t(vtable.data());*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(&model);uint32_t observerVtable=uint32_t(&notify);using Notify=void(__thiscall*)(void*);unsigned cases=0;
for(unsigned length=0;length<=16;++length)for(pattern=0;pattern<8;++pattern){element={};nodes={};observers={};out={};for(unsigned i=0;i<16;++i)observers[i]={uint32_t(&observerVtable),i};for(unsigned i=0;i<length;++i){unsigned id=pattern&4?i%3:i;nodes[i][0]=uint32_t(observers[id].data());nodes[i][1]=i+1<length?uint32_t(nodes[i+1].data()):0;}element[0x50/4]=length?uint32_t(nodes[0].data()):0;reinterpret_cast<Notify>(0x10360b20)(element.data());out[19]=element[0x50/4]!=0;oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_registration_observer_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
