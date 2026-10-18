#include "two_point_curve_evaluation.h"
#include "curve_evaluation_envelope.h"
#include <iostream>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");for(auto code:{4u,6u}){if(evaluateRecoveredTwoPointCurve(code,0,2,10)!=2||evaluateRecoveredTwoPointCurve(code,1,2,10)!=10||evaluateRecoveredTwoPointCurve(code,.5f,2,10)!=6)throw std::runtime_error("endpoint/midpoint failed");}
 if(evaluateRecoveredTwoPointCurve(4,.25f,2,10)!=3.25f||evaluateRecoveredTwoPointCurve(6,.25f,2,10)!=4)throw std::runtime_error("mode distinction failed");
 auto eval=[](uint32_t code,float f){return evaluateRecoveredTwoPointCurve(code,f,2,10);};if(evaluateRecoveredCurveEnvelope(.25f,2,10,4,6<<8,-100,100,eval)!=3.4375f)throw std::runtime_error("real mixed evaluator failed");
 if(std::abs(evaluateRecoveredTwoPointCurve(2,.5f,0,1)-.70710678f)>1e-7f||std::abs(evaluateRecoveredTwoPointCurve(3,.5f,0,1)-.29289322f)>1e-7f)throw std::runtime_error("sine modes failed");
 for(auto code:{2u,3u})if(std::abs(evaluateRecoveredTwoPointCurve(code,0,0,1))>1e-7f||std::abs(evaluateRecoveredTwoPointCurve(code,1,0,1)-1)>1e-7f)throw std::runtime_error("sine endpoints failed");
 for(float f:{0.f,.25f,.5f,.75f,1.f})if(std::abs(evaluateRecoveredTwoPointCurve(10,f,2,10)-evaluateRecoveredTwoPointCurve(4,f,2,10))>1e-6f)throw std::runtime_error("mode10 polynomial failed");
 bool rejected=false;try{evaluateRecoveredTwoPointCurve(0,.5f,2,10);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("unknown code silently approximated");std::cout<<"native_two_point_modes=2,3,4,6 checks=16\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
