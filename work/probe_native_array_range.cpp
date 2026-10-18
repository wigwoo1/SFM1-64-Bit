#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}




#include "array_reference_range_fixture.h"
static std::array<uint32_t,128> output{};static unsigned cursor=0;
static uint32_t* current=nullptr;static bool allowed=false,rejected=false;
static void emit(uint32_t code,uint32_t arg){output[cursor++]=code;output[cursor++]=arg;output[cursor++]=arrayRangeHash(current);}
static bool __fastcall permission(void*,void*){emit(1,0);return allowed;}
static bool __fastcall undo(void*,void*,void*){return false;}
static std::array<std::array<uint32_t,2>,4> objects{};
static void* __fastcall resolveTarget(void*,void*,uint32_t h){emit(4,h);return h>=1&&h<=4?objects[h-1].data():nullptr;}
static bool __fastcall matchTarget(void*self,void*,uint32_t cls){auto h=static_cast<uint32_t*>(self)[1];if(cls!=42)throw std::runtime_error("required class");emit(5,h);return !(rejected&&(h==2||h==4));}
static void __fastcall removeTarget(void*,void*,uint32_t h,void*){emit(2,h);}
static void __fastcall addTarget(void*,void*,uint32_t h,void*){emit(3,h);}
static void __fastcall dirtyTarget(void*,void*,uint32_t value){emit(6,value&255);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");






 redirect(0x103623B0,reinterpret_cast<void*>(&permission));redirect(0x10359940,reinterpret_cast<void*>(&removeTarget));redirect(0x103597E0,reinterpret_cast<void*>(&addTarget));redirect(0x1035EFF0,reinterpret_cast<void*>(&dirtyTarget));
 std::array<uint32_t,100> modelVtable{};modelVtable[0x48/4]=uint32_t(&resolveTarget);modelVtable[0xf8/4]=uint32_t(&undo);std::array<uint32_t,1> model{uint32_t(modelVtable.data())};*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(model.data());
 std::vector<uint32_t> framework(0xc184/4+1);*reinterpret_cast<uint32_t*>(0x10e9a518)=uint32_t(framework.data());std::array<uint32_t,5> classVtable{};classVtable[4]=uint32_t(&matchTarget);for(unsigned i=0;i<4;++i)objects[i]={uint32_t(classVtable.data()),i+1};
 using Assign=void(__thiscall*)(void*,int,int,const uint32_t*);auto assign=reinterpret_cast<Assign>(0x1036d480);
 for(unsigned seed=0;seed<5376;++seed){auto fixture=arrayRangeFixture(seed);auto values=fixture.old;current=values.data();allowed=fixture.allow;rejected=fixture.reject;output.fill(0);cursor=0;framework[0xc180/4]=0;
 std::array<uint32_t,8> attribute{};attribute[4]=15;std::array<uint32_t,6> storage{};storage[0]=uint32_t(values.data());storage[3]=5;storage[5]=42;std::array<uint32_t,3>wrapper{0,uint32_t(attribute.data()),uint32_t(storage.data())};
 assign(wrapper.data(),fixture.start,fixture.count,fixture.incoming.data());for(unsigned i=0;i<5;++i)output[120+i]=values[i];output[125]=cursor;output[126]=framework[0xc180/4];oracle.write(reinterpret_cast<const char*>(output.data()),sizeof(output));
 }
 std::cout<<"native_array_range_cases=5376\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
