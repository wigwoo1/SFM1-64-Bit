#include "float_curve_evaluation.h"
#include <iostream>
#include <limits>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");auto close=[](float a,float b){if(std::abs(a-b)>2e-6f)throw 1;};std::array<float,4>t{0,1,3,7},v{0,2,10,0};close(evaluateRecoveredExponentialCurve(t,v,0),2);close(evaluateRecoveredExponentialCurve(t,v,1),9.992f);close(evaluateRecoveredExponentialCurve(t,v,.5f),static_cast<float>(2+8*(1-std::sqrt(double(.001f)))));close(evaluateRecoveredExponentialCurve({0,1,1,2},v,.5f),2);close(evaluateRecoveredExponentialCurve({0,3,1,2},v,.5f),2);close(evaluateRecoveredFloatCurve(t,v,{0,0xE0E,0xE0E,0},.5f,0,5),5);std::cout<<"exponential_curve_checks=6 mode=14\n";return 0;}catch(...){std::cerr<<"exponential mismatch\n";return 1;}}
