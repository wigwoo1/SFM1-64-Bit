#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static std::array<float,12> positionParent{},rotationParent{};
static bool sharedParent=false;
static void* __fastcall getPositionParent(void*,void*){return positionParent.data();}
static void* __fastcall getRotationParent(void*,void*){return sharedParent?positionParent.data():rotationParent.data();}
static void __cdecl getParentWorld(void* parent,float*out){std::memcpy(out,parent,48);}
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");


 redirect(0x104C3ED0,reinterpret_cast<void*>(&getPositionParent));redirect(0x104C3F60,reinterpret_cast<void*>(&getRotationParent));redirect(0x104C5A00,reinterpret_cast<void*>(&getParentWorld));
 unsigned checks=0;using Convert=void(__thiscall*)(void*,const float*,float*);using Scale=float(__cdecl*)(const float*);
 std::array<uint32_t,64> object{};
 for(uint32_t shared=0;shared<2;++shared)for(uint32_t pattern=0;pattern<1024;++pattern){sharedParent=shared!=0;std::array<float,12> world{},out{};for(unsigned i=0;i<12;++i){positionParent[i]=float(int((pattern*17+i*13)%127)-63)*.03125f;rotationParent[i]=float(int((pattern*23+i*7)%113)-56)*.0625f;world[i]=float(int((pattern*19+i*11)%109)-54)*.125f;}
 float ps=reinterpret_cast<Scale>(0x1007FDF0)(positionParent.data());float rs=reinterpret_cast<Scale>(0x1007FDF0)((shared?positionParent:rotationParent).data());reinterpret_cast<Convert>(0x104C6AD0)(object.data(),world.data(),out.data());oracle.write(reinterpret_cast<const char*>(&pattern),4);oracle.write(reinterpret_cast<const char*>(&shared),4);oracle.write(reinterpret_cast<const char*>(&ps),4);oracle.write(reinterpret_cast<const char*>(&rs),4);oracle.write(reinterpret_cast<const char*>(out.data()),48);++checks;}
 std::cout<<"native_world_to_local_checks="<<checks<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
