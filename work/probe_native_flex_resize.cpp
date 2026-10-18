#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}
static std::vector<uint32_t> weights;
static unsigned appends,removes;
static uint32_t* countOf(void* wrapper){return reinterpret_cast<uint32_t*>(static_cast<char*>(wrapper)+12);}
static void __fastcall append(void* wrapper,void*,const uint32_t* v){if(*v!=0)throw std::runtime_error("nonzero append");weights.push_back(*v);++*countOf(wrapper);++appends;}
static void __fastcall removeLast(void* wrapper,void*,unsigned index){if(index+1!=weights.size())throw std::runtime_error("non-tail removal");weights.pop_back();--*countOf(wrapper);++removes;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"x86 required");
std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
if(argc!=2||std::ifstream(argv[1]).good())throw std::runtime_error("fresh output required");std::ofstream out(argv[1],std::ios::binary);if(!out)throw std::runtime_error("output failed");
redirect(0x1036f980,reinterpret_cast<void*>(&append));redirect(0x10385720,reinterpret_cast<void*>(&removeLast));
using Resize=void(__thiscall*)(void*,unsigned);
for(unsigned old=0;old<32;++old)for(unsigned requested=0;requested<32;++requested){std::array<uint32_t,64> model{};model[0xc8/4]=old;weights.clear();for(unsigned i=0;i<old;++i)weights.push_back(0x3f000000u+i);appends=removes=0;reinterpret_cast<Resize>(0x105c2d10)(model.data(),requested);if(model[0xc8/4]!=requested||weights.size()!=requested)throw std::runtime_error("wrong count");out.write(reinterpret_cast<char*>(&appends),4);out.write(reinterpret_cast<char*>(&removes),4);for(auto v:weights)out.write(reinterpret_cast<char*>(&v),4);}
std::cout<<"native flex resize cases=1024\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
