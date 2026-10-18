#include "float_curve_evaluation.h"
#include <iostream>
#include <cmath>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");auto close=[](float a,float b){if(std::abs(a-b)>1e-6f)throw 1;};for(float f:{0.f,.25f,.5f,.75f,1.f})close(evaluateRecoveredCatmullDerivative(0,2,4,6,f),2);close(evaluateRecoveredCatmullDerivative(0,0,1,0,0),.5f);close(evaluateRecoveredCatmullDerivative(0,0,1,0,.5f),1.375f);close(evaluateRecoveredCatmullDerivative(0,0,1,0,1),0);close(evaluateRecoveredFloatCurve({0,1,2,3},{0,0,1,0},{0,0xD0D,0xD0D,0},.5f,0,1),1);std::cout<<"catmull_derivative_checks=9 mode=13\n";return 0;}catch(...){std::cerr<<"derivative mismatch\n";return 1;}}
