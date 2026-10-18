#include "float_curve_evaluation.h"
#include <iostream>
#include <cmath>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");std::array<float,4>t{0,1,3,7},v{0,0,1,0};float expected=evaluateRecoveredNormalizedCatmull(t,v,.25f);for(uint32_t code=16;code<256;++code){auto packed=static_cast<int32_t>((code<<8)|code);if(evaluateRecoveredFloatCurve(t,v,{0,packed,packed,0},.25f,-10,10)!=expected)throw 1;}auto mix=evaluateRecoveredFloatCurve(t,v,{0,16,6<<8,0},.25f,-10,10);float linear=.25f;if(std::abs(mix-(expected+(linear-expected)*.25f))>1e-7f)throw 1;std::cout<<"invalid_curve_codes=240 mixed_fallback=verified\n";return 0;}catch(...){std::cerr<<"fallback mismatch\n";return 1;}}
