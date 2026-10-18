#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


static std::array<std::array<uint32_t,2>,32>attributes;static std::array<uint32_t,40>out;static uint32_t mode;static uint32_t* framework;
static uint32_t __fastcall getMode(void*,void*){return mode;}
static void __fastcall setMode(void*,void*,uint32_t value){if(out[1]==0){if(value!=0)throw std::runtime_error("disable");out[1]=1;}else{if(!(framework[0xc180/4]&0x1000))throw std::runtime_error("restore before flag");out[1]=2;}mode=value;}
static void __cdecl destroyAttribute(void*self){if(mode!=0)throw std::runtime_error("mode enabled");auto*p=static_cast<uint32_t*>(self);out[4+out[2]++]=p[1];std::memset(p,0xcc,8);}
static void __fastcall dirty(void*,void*,uint32_t request){if(mode!=0||request!=3)throw std::runtime_error("dirty state");out[3]++;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x1038b4f0,reinterpret_cast<void*>(&destroyAttribute));redirect(0x1035e400,reinterpret_cast<void*>(&dirty));std::array<uint32_t,64>vtable{};vtable[0xf4/4]=uint32_t(&getMode);vtable[0xf0/4]=uint32_t(&setMode);uint32_t model=uint32_t(vtable.data());*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(&model);using Finalize=void(__thiscall*)(void*);unsigned cases=0;
for(unsigned length=0;length<=32;++length)for(unsigned initial=0;initial<256;++initial){std::array<uint32_t,0xc240/4>state{};framework=state.data();state[0xc180/4]=0xa5000000;*reinterpret_cast<uint32_t*>(0x10e9a518)=uint32_t(state.data());attributes={};for(unsigned i=0;i<length;++i){unsigned index=i*17%32;attributes[index][0]=i+1<length?uint32_t(attributes[(i+1)*17%32].data()):0;attributes[index][1]=index;}std::array<uint32_t,24>element{};element[12]=length?uint32_t(attributes[0].data()):0;mode=initial;out={};reinterpret_cast<Finalize>(0x1035f900)(element.data());out[0]=mode;out[36]=element[12]!=0;out[37]=state[0xc180/4];oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_element_finalization_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
