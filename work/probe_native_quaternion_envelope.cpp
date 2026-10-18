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
 using Native=void(__cdecl*)(const float*,const float*,const float*,const float*,float,float*);std::array<uintptr_t,5>addresses{0x1049A500,0x1049BA80,0x1049A7C0,0x1049AC20,0x1049AA70};unsigned checks=0;std::vector<float>fractions;for(int i=0;i<=100;++i)fractions.push_back(static_cast<float>(i)/100.f);fractions.push_back(-.25f);fractions.push_back(1.25f);
 for(unsigned mode=0;mode<256;++mode)for(float fraction:fractions)for(auto v: {std::array<float,4>{0,0,1,0},std::array<float,4>{-3.7f,9.2f,-5.1f,100.3f},std::array<float,4>{0,1,3,7}})for(auto times:{std::array<float,4>{0,1,3,7},std::array<float,4>{0,1,2,3},std::array<float,4>{1,1,3,3},std::array<float,4>{0,1,1,7},std::array<float,4>{100000000.f,100000008.f,100000024.f,100000056.f}}){std::array<std::array<float,4>,4>points{};for(unsigned i=0;i<4;++i)points[i]={0,v[i],v[(i+1)%4],1};std::array<float,4>out{};std::array<int32_t,4>ticks{};for(unsigned i=0;i<4;++i)ticks[i]=static_cast<int32_t>(times[i]);std::array<int32_t,4>kinds{0,static_cast<int32_t>(mode%16),static_cast<int32_t>((mode/16)<<8),0};using Envelope=float*(__cdecl*)(float*,float,const int32_t*,const float*,const int32_t*,float,float);reinterpret_cast<Envelope>(0x104FCF90)(out.data(),fraction,ticks.data(),points[0].data(),kinds.data(),0.f,1.f);oracle.write(reinterpret_cast<const char*>(&mode),4);oracle.write(reinterpret_cast<const char*>(&fraction),4);oracle.write(reinterpret_cast<const char*>(v.data()),16);oracle.write(reinterpret_cast<const char*>(times.data()),16);oracle.write(reinterpret_cast<const char*>(out.data()),16);++checks;}
 std::cout<<"original_x86_instruction_checks="<<checks<<" oracle_generated=1 DLL_entrypoint_not_called=1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
