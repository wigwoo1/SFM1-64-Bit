#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static std::array<std::array<uint32_t,64>,4> targets{}, dags{};
static void* __fastcall resolve(void*,void*,uint32_t h){if(h>=1&&h<=4)return targets[h-1].data();if(h>=11&&h<=14)return dags[h-11].data();return nullptr;}
static bool __fastcall typeCheck(void*,void*,uint32_t){return true;}
static void __fastcall worldPosition(void* self,void*,float*out){std::memcpy(out,static_cast<char*>(self)+0x20,12);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");


 std::array<uint32_t,64> dmTable{},dm{},targetTable{},object{};
 dmTable[0x48/4]=reinterpret_cast<uint32_t>(&resolve);dm[0]=reinterpret_cast<uint32_t>(dmTable.data());*reinterpret_cast<uint32_t*>(0x10E9A51C)=reinterpret_cast<uint32_t>(dm.data());
 targetTable[0x10/4]=reinterpret_cast<uint32_t>(&typeCheck);
 // Replace only world-position traversal in this private mapped image.
 auto entry=reinterpret_cast<unsigned char*>(0x104C5900);entry[0]=0xE9;int32_t displacement=int32_t(reinterpret_cast<uintptr_t>(&worldPosition)-0x104C5905);std::memcpy(entry+1,&displacement,4);FlushInstructionCache(GetCurrentProcess(),entry,5);
 std::array<uint32_t,4> handles{1,2,3,4};object[0x64/4]=reinterpret_cast<uint32_t>(handles.data());
 using Average=float(__thiscall*)(void*,float*);
 unsigned checks=0;
 const std::array<float,8> weights{-2,-.5f,0,.125f,.5f,1,2,8};
 for(uint32_t count=0;count<=4;++count)for(uint32_t pattern=0;pattern<64;++pattern)for(uint32_t missing=0;missing<16;++missing){
  std::array<float,3> initial{-.25f,.5f,1.5f};std::memcpy(reinterpret_cast<void*>(0x10F4ED24),initial.data(),12);object[0x70/4]=count;
  for(unsigned i=0;i<4;++i){targets[i].fill(0);dags[i].fill(0);targets[i][0]=reinterpret_cast<uint32_t>(targetTable.data());targets[i][0x5C/4]=(missing&(1u<<i))?0:11+i;float weight=weights[(pattern+i*3)%8];std::memcpy(reinterpret_cast<char*>(targets[i].data())+0x68,&weight,4);std::array<float,3> pos{float(i*3)-4.125f,float(pattern)*.125f-2,float(i)*-.75f},offset{.125f*float(i),-.5f,1.25f};std::memcpy(reinterpret_cast<char*>(targets[i].data())+0x70,offset.data(),12);std::memcpy(reinterpret_cast<char*>(dags[i].data())+0x20,pos.data(),12);}
  std::array<float,4> out{};out[3]=reinterpret_cast<Average>(0x105CF4A0)(object.data(),out.data());oracle.write(reinterpret_cast<const char*>(&count),4);oracle.write(reinterpret_cast<const char*>(&pattern),4);oracle.write(reinterpret_cast<const char*>(&missing),4);oracle.write(reinterpret_cast<const char*>(out.data()),16);++checks;
 }
 std::cout<<"native_point_average_checks="<<checks<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
