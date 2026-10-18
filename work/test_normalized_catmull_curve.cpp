#include "float_curve_evaluation.h"
#include "curve_evaluation_envelope.h"
#include <iostream>
#include <stdexcept>
#include <limits>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");std::array<float,4>t{0,1,3,7},v{0,1,3,7};for(float f:{0.f,.25f,.5f,.75f,1.f})if(evaluateRecoveredNormalizedCatmull(t,v,f)!=1+2*f)throw std::runtime_error("uneven linear timing failed");
 std::array<float,4>equal{0,1,2,3},shape{0,0,1,0};if(evaluateRecoveredNormalizedCatmull(equal,shape,.5f)!=.5625f)throw std::runtime_error("uniform timing failed");
 std::array<float,4>duplicate{1,1,3,3};if(evaluateRecoveredNormalizedCatmull(duplicate,shape,.5f)!=.5625f)throw std::runtime_error("duplicate neighbor timing failed");
 std::array<float,4>zeroSpan{0,1,1,7};if(evaluateRecoveredNormalizedCatmull(zeroSpan,shape,.5f)!=.5625f)throw std::runtime_error("zero span failed");
 auto eval=[&](uint32_t code,float f){if(code>1)throw std::runtime_error("wrong default dispatch");return evaluateRecoveredNormalizedCatmull(t,v,f);};if(evaluateRecoveredCurveEnvelope(.5f,1,3,0,1<<8,0,10,eval)!=2)throw std::runtime_error("default envelope failed");
 if(evaluateRecoveredFloatCurve(t,v,{0,0,256,0},.5f,0,10)!=2)throw std::runtime_error("combined default dispatch failed"); t[0]=std::numeric_limits<float>::quiet_NaN();auto result=evaluateRecoveredNormalizedCatmull(t,v,.5f);if(result==result)throw std::runtime_error("unordered normalization skipped");
 std::cout<<"normalized_catmull_checks=10 default_codes=0,1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

