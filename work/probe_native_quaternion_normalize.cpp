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
 for(int i=0;i<1001;++i){std::array<float,4>q{float(i)/1000.f,-3.7f,float(i)-500.f,.70710677f};auto before=q;using Normalize=float(__cdecl*)(float*);reinterpret_cast<Normalize>(0x10499870)(q.data());oracle.write(reinterpret_cast<const char*>(before.data()),16);oracle.write(reinterpret_cast<const char*>(q.data()),16);++checks;}
 std::cout<<"native_quaternion_normalize_checks="<<checks<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
