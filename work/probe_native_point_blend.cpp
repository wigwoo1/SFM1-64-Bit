#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
static std::array<uint32_t,64> slave{},dag{};
static std::array<float,3> average{},slavePosition{},captured{};
static float influence;
static void* __fastcall resolve(void*,void*,uint32_t handle){return handle==1?static_cast<void*>(slave.data()):static_cast<void*>(dag.data());}
static void __fastcall profileBegin(void*,void*,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t){}
static void __fastcall profileEnd(void*,void*){}
static float __fastcall getAverage(void*,void*,float*out){std::memcpy(out,average.data(),12);return influence;}
static void __fastcall slaveMatrix(void*,void*,float*out){std::memset(out,0,48);out[0]=out[5]=out[10]=1;out[3]=slavePosition[0];out[7]=slavePosition[1];out[11]=slavePosition[2];}
static void __fastcall writePosition(void*,void*,const float*value){std::memcpy(captured.data(),value,12);}
static void redirect(uint32_t address,void* function){auto p=reinterpret_cast<unsigned char*>(address);p[0]=0xE9;int32_t relative=int32_t(reinterpret_cast<uintptr_t>(function)-address-5);std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");


 std::array<uint32_t,64> table{},dm{},object{};table[0x48/4]=reinterpret_cast<uint32_t>(&resolve);dm[0]=reinterpret_cast<uint32_t>(table.data());*reinterpret_cast<uint32_t*>(0x10E9A51C)=reinterpret_cast<uint32_t>(dm.data());object[0x80/4]=1;slave[0x5C/4]=2;
 *reinterpret_cast<uint32_t*>(0x10A7E460)=reinterpret_cast<uint32_t>(&profileBegin);*reinterpret_cast<uint32_t*>(0x10A7E48C)=reinterpret_cast<uint32_t>(&profileEnd);
 redirect(0x105CF4A0,reinterpret_cast<void*>(&getAverage));redirect(0x105CE7B0,reinterpret_cast<void*>(&slaveMatrix));redirect(0x104C6F80,reinterpret_cast<void*>(&writePosition));
 unsigned checks=0;using Evaluate=void(__thiscall*)(void*);
 for(uint32_t pattern=0;pattern<32;++pattern)for(int32_t step=-100;step<=200;++step){influence=float(step)*.01f;for(unsigned c=0;c<3;++c){average[c]=float(int(pattern*7+c*3)-55)*.125f;slavePosition[c]=float(int(pattern*11+c*5)-73)*.0625f;}reinterpret_cast<Evaluate>(0x105CF9A0)(object.data());oracle.write(reinterpret_cast<const char*>(&pattern),4);oracle.write(reinterpret_cast<const char*>(&influence),4);oracle.write(reinterpret_cast<const char*>(captured.data()),12);++checks;}
 std::cout<<"native_point_blend_checks="<<checks<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
