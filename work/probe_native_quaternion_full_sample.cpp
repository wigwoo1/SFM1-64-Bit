#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include "float_curve_evaluation.h"
#include <limits>
#include "catmull_curve_component.h"
#include "bspline_curve_component.h"
#include "catmull_derivative_curve.h"
#include "normalized_catmull_curve.h"
#include "chord_catmull_curve.h"
#include "parameterized_curve_component.h"
inline float oracleParameterized(const std::array<float,4>&times,const std::array<float,4>&v,float fraction,unsigned mode){auto n=normalizeRecoveredCurveValues(times,v);return evaluateRecoveredParameterizedComponent(n[0],n[1],n[2],n[3],fraction,recoveredParameterizedCurve(mode+2));}
static void* curveTarget=nullptr;
static void* __fastcall resolveCurve(void*,void*,uint32_t){return curveTarget;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");


 std::array<uint32_t,64>dmTable{},dm{},owner{},info{},object{};
 dmTable[0x48/4]=reinterpret_cast<uint32_t>(&resolveCurve);dm[0]=reinterpret_cast<uint32_t>(dmTable.data());*reinterpret_cast<uint32_t*>(0x10E9A51C)=reinterpret_cast<uint32_t>(dm.data());curveTarget=info.data();owner[0x78/4]=1;
 std::array<int32_t,4>times{0,10000,30000,70000},kinds{};std::array<std::array<float,4>,4>values{{{0,0,0,1},{0,.70710677f,0,.70710677f},{0,0,0,-1},{.70710677f,0,0,.70710677f}}};
 object[0]=0x10B9220C;object[0x5C/4]=reinterpret_cast<uint32_t>(owner.data());object[0x60/4]=uint32_t(-1);object[0x68/4]=reinterpret_cast<uint32_t>(times.data());object[0x74/4]=4;object[0x80/4]=reinterpret_cast<uint32_t>(kinds.data());object[0x8C/4]=4;object[0x98/4]=reinterpret_cast<uint32_t>(values.data());
 info[0xC4/4]=80000;reinterpret_cast<unsigned char*>(info.data())[0x74]=1;reinterpret_cast<unsigned char*>(info.data())[0x7C]=1;reinterpret_cast<unsigned char*>(info.data())[0x84]=1;
 std::array<float,4>left{0,0,0,1},right{0,0,0,1};std::memcpy(reinterpret_cast<char*>(info.data())+0x8C,left.data(),16);std::memcpy(reinterpret_cast<char*>(info.data())+0xA0,right.data(),16);
 unsigned checks=0;using Sample=void(__thiscall*)(void*,int32_t,float*);
 for(uint32_t flags=0;flags<8;++flags)for(uint32_t mode=0;mode<16;++mode){reinterpret_cast<unsigned char*>(info.data())[0x74]=(flags&1)!=0;reinterpret_cast<unsigned char*>(info.data())[0x7C]=(flags&2)!=0;reinterpret_cast<unsigned char*>(info.data())[0x84]=(flags&4)!=0;std::array<float,4>disabled{-8,-10,-12},zero{0,0,0,1};std::memcpy(reinterpret_cast<void*>(0x10F4ED24),disabled.data(),12);std::memcpy(reinterpret_cast<char*>(info.data())+0xCC,zero.data(),16);info[0x5C/4]=mode*257;kinds.fill(mode*257);for(int32_t tick=-10000;tick<=90000;tick+=1000){std::array<float,4>out{};reinterpret_cast<Sample>(0x1054D4D0)(object.data(),tick,out.data());uint32_t code=mode+flags*16;oracle.write(reinterpret_cast<const char*>(&code),4);oracle.write(reinterpret_cast<const char*>(&tick),4);oracle.write(reinterpret_cast<const char*>(out.data()),16);++checks;}}
 std::cout<<"native_quaternion_full_sample_checks="<<checks<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
