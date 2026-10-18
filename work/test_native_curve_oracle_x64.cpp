#include <fstream>
#include <iostream>
#include <array>
#include <cstring>
#include <stdexcept>
#include "catmull_curve_component.h"
#include "bspline_curve_component.h"
#include "catmull_derivative_curve.h"
#include "normalized_catmull_curve.h"
#include "chord_catmull_curve.h"
#include "parameterized_curve_component.h"
inline float oracleParameterized(const std::array<float,4>&times,const std::array<float,4>&v,float fraction,unsigned mode){auto n=normalizeRecoveredCurveValues(times,v);return evaluateRecoveredParameterizedComponent(n[0],n[1],n[2],n[3],fraction,recoveredParameterizedCurve(mode+2));}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");unsigned checks=0;for(;;){uint32_t mode;float fraction,expected;std::array<float,4>v,times;f.read(reinterpret_cast<char*>(&mode),4);if(f.gcount()==0&&f.eof())break;if(!f||mode>7)throw std::runtime_error("invalid oracle mode");f.read(reinterpret_cast<char*>(&fraction),4);f.read(reinterpret_cast<char*>(v.data()),16);f.read(reinterpret_cast<char*>(times.data()),16);f.read(reinterpret_cast<char*>(&expected),4);if(!f)throw std::runtime_error("truncated oracle");float actual=mode==0?evaluateRecoveredCatmullComponent(v[0],v[1],v[2],v[3],fraction):mode==1?evaluateRecoveredBSplineComponent(v[0],v[1],v[2],v[3],fraction):mode==2?evaluateRecoveredCatmullDerivative(v[0],v[1],v[2],v[3],fraction):mode==3?evaluateRecoveredNormalizedCatmull(times,v,fraction):mode==4?evaluateRecoveredChordCatmull(times,v,fraction):oracleParameterized(times,v,fraction,mode);if(std::memcmp(&actual,&expected,4)){std::cerr<<"oracle mismatch case="<<checks<<" mode="<<mode<<'\n';return 2;}++checks;}if(checks!=720)throw std::runtime_error("unexpected case count");std::cout<<"native_x86_vs_x64_checks="<<checks<<" bitwise_equal=1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
