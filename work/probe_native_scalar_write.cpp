#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}




static std::array<uint32_t,16>events{};static unsigned count=0;static uint32_t*current=nullptr;static bool allowed=false;
static void event(uint32_t code,uint32_t h){events[count++]=code;events[count++]=h;events[count++]=*current;}
static bool __fastcall permission(void*,void*){event(1,0);return allowed;}
static bool __fastcall undo(void*,void*,void*){event(2,0);return false;}
static void __fastcall removeTarget(void*,void*,uint32_t h,void*){event(3,h);}
static void __fastcall addTarget(void*,void*,uint32_t h,void*){event(4,h);}
static void __fastcall dirty(void*,void*,uint32_t value){event(5,value&255);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");






 redirect(0x103623B0,reinterpret_cast<void*>(&permission));redirect(0x10359940,reinterpret_cast<void*>(&removeTarget));redirect(0x103597E0,reinterpret_cast<void*>(&addTarget));redirect(0x1035EFF0,reinterpret_cast<void*>(&dirty));
 std::array<uint32_t,100>vtable{};vtable[0xf8/4]=uint32_t(&undo);std::array<uint32_t,1>model{uint32_t(vtable.data())};*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(model.data());std::vector<uint32_t>framework(0xc184/4+1);*reinterpret_cast<uint32_t*>(0x10e9a518)=uint32_t(framework.data());
 using Write=void(__thiscall*)(void*,const uint32_t*);auto write=reinterpret_cast<Write>(0x10366f90);
 unsigned cases=0;for(unsigned old=0;old<17;++old)for(unsigned incoming=0;incoming<17;++incoming)for(unsigned allow=0;allow<2;++allow){std::array<uint32_t,8>attribute{};std::array<uint32_t,3>storage{old==16?0xffffffffu:old,0xffffffffu,0};attribute[1]=uint32_t(storage.data());attribute[4]=1;current=storage.data();uint32_t newValue=incoming==16?0xffffffffu:incoming;allowed=allow!=0;events.fill(0);count=0;framework[0xc180/4]=0;write(attribute.data(),&newValue);std::array<uint32_t,19>out{};std::copy(events.begin(),events.end(),out.begin());out[16]=storage[0];out[17]=count;out[18]=framework[0xc180/4];oracle.write(reinterpret_cast<const char*>(out.data()),sizeof(out));++cases;}
 std::cout<<"native_scalar_write_cases="<<cases<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
