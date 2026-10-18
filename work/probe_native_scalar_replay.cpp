#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}




static std::array<uint32_t,16>output{};static unsigned count=0;static bool ownerPresent=false,attributePresent=false;static std::array<uint32_t,8>attribute{};
static void emit(uint32_t c,uint32_t arg){output[count++]=c;output[count++]=arg;}
static void* __fastcall resolveOwner(void*,void*,uint32_t h){emit(1,h);return ownerPresent?reinterpret_cast<void*>(1):nullptr;}
static void* __fastcall findAttribute(void*,void*,uint32_t name){emit(2,name==0x10a8a4f0?0xffffffffu:name);return attributePresent?attribute.data():nullptr;}
static void __fastcall writeAttribute(void*self,void*,const uint32_t*value){if(self!=attribute.data())throw std::runtime_error("writer attribute");emit(3,*value);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");






 redirect(0x1035dd50,reinterpret_cast<void*>(&findAttribute));redirect(0x10366f90,reinterpret_cast<void*>(&writeAttribute));std::array<uint32_t,32>vt{};vt[0x48/4]=uint32_t(&resolveOwner);std::array<uint32_t,1>model{uint32_t(vt.data())};*reinterpret_cast<uint32_t*>(0x10e9a51c)=uint32_t(model.data());using Replay=void(__thiscall*)(void*);
 for(unsigned seed=0;seed<512;++seed){ownerPresent=(seed&1)!=0;attributePresent=(seed&2)!=0;attribute.fill(0);attribute[4]=(seed&4)?0x21:1;std::array<uint32_t,9>record{};record[5]=(seed&8)?0xffffffffu:42;record[6]=seed/16;record[7]=(seed&16)?0xffffffffu:seed+100;record[8]=(seed&32)?0xffffffffu:seed+200;output.fill(0);count=0;reinterpret_cast<Replay>((seed&64)?0x1036c190:0x1036c1f0)(record.data());output[15]=count;oracle.write(reinterpret_cast<const char*>(output.data()),sizeof(output));}
 std::cout<<"native_scalar_replay_cases=512\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
