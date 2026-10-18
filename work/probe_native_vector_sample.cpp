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

 using Sample=const float*(__thiscall*)(void*,int32_t);
 unsigned checks=0;
 for(auto times:{std::array<int32_t,4>{0,10000,30000,70000},std::array<int32_t,4>{-70000,-30000,-10000,0},std::array<int32_t,4>{100000000,100000008,100000024,100000056}}){
 std::array<std::array<float,3>,4> values{{{0,-3.7f,100.3f},{1,9.2f,-5.1f},{3,-5.1f,9.2f},{7,100.3f,-3.7f}}};
 std::array<uint32_t,64> object{};object[0]=0x10B9211C;object[0x60/4]=uint32_t(-1);object[0x68/4]=reinterpret_cast<uint32_t>(times.data());object[0x74/4]=4;object[0x98/4]=reinterpret_cast<uint32_t>(values.data());
 for(int i=-50;i<=1050;++i){int32_t tick=times.front()+static_cast<int32_t>((int64_t(times.back())-times.front())*i/1000);
 auto out=reinterpret_cast<Sample>(0x1054F560)(object.data(),tick);
 oracle.write(reinterpret_cast<const char*>(&tick),4);oracle.write(reinterpret_cast<const char*>(times.data()),16);oracle.write(reinterpret_cast<const char*>(values.data()),48);oracle.write(reinterpret_cast<const char*>(out),12);++checks;
 }
 }
 std::cout<<"native_vector_sample_checks="<<checks<<" DLL_entrypoint_not_called=1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
