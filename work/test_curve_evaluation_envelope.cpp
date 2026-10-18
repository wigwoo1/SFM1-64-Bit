#include "curve_evaluation_envelope.h"
#include <iostream>
#include <stdexcept>
#include <limits>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");for(uint32_t a=0;a<256;++a)for(uint32_t b=0;b<256;++b){auto d=decodeRecoveredCurveKinds(static_cast<int32_t>(0xabcd0000u|(a<<8)|b));if(d.incoming!=a||d.outgoing!=b)throw std::runtime_error("decode mismatch");}
 unsigned calls=0;auto eval=[&](uint32_t kind,float){++calls;return float(kind);};
 if(evaluateRecoveredCurveEnvelope(.25f,2,9,15,15<<8,0,10,eval)!=2||calls)throw std::runtime_error("hold priority failed");
 if(evaluateRecoveredCurveEnvelope(.25f,2,9,3,15<<8,0,10,eval)!=9||calls)throw std::runtime_error("right hold failed");
 if(evaluateRecoveredCurveEnvelope(.25f,2,9,3,3<<8,0,10,eval)!=3||calls!=1)throw std::runtime_error("shared evaluator failed");
 if(evaluateRecoveredCurveEnvelope(.25f,2,9,2,6<<8,0,10,eval)!=3||calls!=3)throw std::runtime_error("mixed evaluator blend failed");
 if(evaluateRecoveredCurveEnvelope(.25f,20,9,15,0,0,10,eval)!=10||evaluateRecoveredCurveEnvelope(.25f,-2,9,15,0,0,10,eval)!=0)throw std::runtime_error("clamp failed");
 auto nan=std::numeric_limits<float>::quiet_NaN();if(evaluateRecoveredCurveEnvelope(0,nan,0,15,0,0,1,eval)==evaluateRecoveredCurveEnvelope(0,nan,0,15,0,0,1,eval))throw std::runtime_error("NaN changed");
 std::cout<<"curve_kind_pairs=65536 envelope_checks=7\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
