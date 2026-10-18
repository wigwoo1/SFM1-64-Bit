#include <fstream>
#include <iostream>
#include <array>
#include <cstring>
#include <stdexcept>
#include "vector_curve_dispatch.h"
#include <limits>
#include "catmull_curve_component.h"
#include "bspline_curve_component.h"
#include "catmull_derivative_curve.h"
#include "normalized_catmull_curve.h"
#include "chord_catmull_curve.h"
#include "parameterized_curve_component.h"
inline float oracleParameterized(const std::array<float,4>&times,const std::array<float,4>&v,float fraction,unsigned mode){auto n=normalizeRecoveredCurveValues(times,v);return evaluateRecoveredParameterizedComponent(n[0],n[1],n[2],n[3],fraction,recoveredParameterizedCurve(mode+2));}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");unsigned checks=0,mismatches=0;std::array<unsigned,16>byMode{};for(;;){uint32_t mode;float fraction;std::array<float,3>expected;std::array<float,4>v,times;f.read(reinterpret_cast<char*>(&mode),4);if(f.gcount()==0&&f.eof())break;if(!f||mode>15)throw std::runtime_error("invalid oracle mode");f.read(reinterpret_cast<char*>(&fraction),4);f.read(reinterpret_cast<char*>(v.data()),16);f.read(reinterpret_cast<char*>(times.data()),16);f.read(reinterpret_cast<char*>(expected.data()),12);if(!f)throw std::runtime_error("truncated oracle");std::array<std::array<float,3>,4>points{};for(unsigned i=0;i<4;++i)points[i]={times[i],v[i],v[(i+1)%4]};auto actual=evaluateRecoveredVectorDispatch(mode,points,fraction);if(std::memcmp(actual.data(),expected.data(),12)){++mismatches;++byMode[mode];}++checks;}if(checks!=240720)throw std::runtime_error("unexpected case count");std::cout<<"native_x86_vs_x64_checks="<<checks<<" mismatches="<<mismatches<<"\n";for(unsigned i=0;i<16;++i)if(byMode[i])std::cout<<"mode="<<i<<" mismatches="<<byMode[i]<<'\n';return mismatches?2:0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
