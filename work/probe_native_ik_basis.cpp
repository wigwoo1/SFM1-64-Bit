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


 unsigned checks=0;using Convert=void(__stdcall*)(const float*,const float*,float*);
 for(uint32_t pattern=0;pattern<4096;++pattern){std::array<float,3>d{},g{};std::array<float,4>out{};
 for(unsigned i=0;i<3;++i){d[i]=float(int((pattern*17+i*13)%127)-63)*.03125f;g[i]=float(int((pattern*23+i*7)%113)-56)*.0625f;}
 if(pattern<3){d={1,0,0};g={0,0,1};if(pattern==1)d={-1,0,0};if(pattern==2){d={-1,0,0};g={0,0,-1};}}
 reinterpret_cast<Convert>(0x105CE130)(d.data(),g.data(),out.data());
 oracle.write(reinterpret_cast<const char*>(&pattern),4);oracle.write(reinterpret_cast<const char*>(out.data()),16);++checks;}
 std::cout<<"native_ik_basis_checks="<<checks<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
