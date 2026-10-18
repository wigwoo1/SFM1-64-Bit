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
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
 if(argc!=2)throw std::runtime_error("oracle output path required");if(std::ifstream(argv[1]).good())throw std::runtime_error("refuse overwrite oracle");std::ofstream oracle(argv[1],std::ios::binary);if(!oracle)throw std::runtime_error("oracle output unavailable");


 unsigned checks=0;
 for(unsigned flags=0;flags<8;++flags)for(unsigned side=0;side<2;++side){std::array<uint32_t,64>info{};reinterpret_cast<unsigned char*>(info.data())[0x74]=(flags&1)!=0;reinterpret_cast<unsigned char*>(info.data())[0x7C]=(flags&2)!=0;reinterpret_cast<unsigned char*>(info.data())[0x84]=(flags&4)!=0;
 std::array<float,3>left{1,2,3},right{4,5,6},zero{7,8,9};std::memcpy(reinterpret_cast<char*>(info.data())+0x8C,left.data(),12);std::memcpy(reinterpret_cast<char*>(info.data())+0x9C,right.data(),12);std::memcpy(reinterpret_cast<char*>(info.data())+0xC4,zero.data(),12);
 std::array<float,3>global{-2,-3,-4};std::memcpy(reinterpret_cast<void*>(0x10F4ED24),global.data(),12);
 std::array<float,3>out{};using Edge=void(__thiscall*)(void*,uint32_t,float*);reinterpret_cast<Edge>(0x10507760)(info.data(),side,out.data());oracle.write(reinterpret_cast<const char*>(&flags),4);oracle.write(reinterpret_cast<const char*>(&side),4);oracle.write(reinterpret_cast<const char*>(out.data()),12);++checks;}
 std::cout<<"native_vector_edge_cases="<<checks<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
