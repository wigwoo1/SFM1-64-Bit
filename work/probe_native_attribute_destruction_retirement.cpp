#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}


static std::array<uint32_t,8>attribute;static std::array<uint32_t,36>out;
static void event(uint32_t code,uint32_t value){unsigned i=out[0]++;out[1+i*2]=code;out[2+i*2]=value;}
static void __fastcall detach(void*,void*,uint32_t target,void*self){if(self!=attribute.data())throw std::runtime_error("attribute");event(1,target);}
static uint32_t __fastcall appendFree(void*,void*,const uint32_t*index){event(5,*index);return 0;}
static void __fastcall releaseStorage(void*self,void*,void*storage){if(uint32_t(storage)!=attribute[1]||attribute[3]!=0xffffffffu)throw std::runtime_error("release ordering");event(3,static_cast<uint32_t*>(self)[1]);}
static void __fastcall releaseAttribute(void*pool,void*,void*self){if(uint32_t(pool)!=0x10f49cacu||self!=attribute.data())throw std::runtime_error("pool");event(4,0);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");




redirect(0x10359940,reinterpret_cast<void*>(&detach));redirect(0x10354c10,reinterpret_cast<void*>(&appendFree));redirect(0x1097a990,reinterpret_cast<void*>(&releaseAttribute));std::array<uint32_t,2>storageVtable{0,uint32_t(&releaseStorage)};std::array<std::array<uint32_t,2>,32>types{};for(unsigned i=0;i<32;++i){types[i]={uint32_t(storageVtable.data()),i};*reinterpret_cast<uint32_t*>(0x10f49bd0+i*4)=uint32_t(types[i].data());}using Destroy=void(__cdecl*)(void*);unsigned cases=0;
for(unsigned type=0;type<32;++type)for(unsigned pattern=0;pattern<8;++pattern)for(unsigned count=0;count<=4;++count){attribute={};out={};std::array<uint32_t,4>handles{0xffffffffu,7,7,9};std::array<uint32_t,4>storage{};storage[0]=type==1?handles[count%4]:uint32_t(handles.data());storage[3]=count;attribute[1]=(pattern&2)&&type!=1&&type!=15?0:uint32_t(storage.data());std::array<uint32_t,0xc240/4>framework{};std::array<uint32_t,2>slot{0,uint32_t(attribute.data())};framework[0xc0ec/4]=1;framework[(0xc0ec+4)/4]=uint32_t(slot.data());framework[(0xc0ec+16)/4]=1;*reinterpret_cast<uint32_t*>(0x10e9a518)=uint32_t(framework.data());attribute[3]=0;attribute[4]=0xabcd0000u|type|(pattern&1?0x200:0)|(pattern&4?0x1000:0);reinterpret_cast<Destroy>(0x1038b4f0)(attribute.data());out[28]=attribute[3];out[29]=attribute[1]!=0;out[30]=attribute[4];out[31]=slot[0];out[32]=slot[1]!=0;out[33]=framework[0xc0ec/4];oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
std::cout<<"native_attribute_destruction_retirement_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
