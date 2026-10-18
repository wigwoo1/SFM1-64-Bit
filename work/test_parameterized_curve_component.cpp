#include "parameterized_curve_component.h"
#include "catmull_curve_component.h"
#include <iostream>
#include <cmath>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");auto close=[](float a,float b){if(std::abs(a-b)>2e-6f)throw 1;};for(float f:{0.f,.25f,.5f,.75f,1.f})close(evaluateRecoveredParameterizedComponent(0,0,1,0,f,{0,0,0}),evaluateRecoveredCatmullComponent(0,0,1,0,f));for(auto code:{7u,8u,9u}){auto p=recoveredParameterizedCurve(code);close(evaluateRecoveredParameterizedComponent(-2,3,7,9,0,p),3);close(evaluateRecoveredParameterizedComponent(-2,3,7,9,1,p),7);close(evaluateRecoveredParameterizedComponent(4,4,4,4,.37f,p),4);}std::cout<<"parameterized_component_checks=14\n";return 0;}catch(...){std::cerr<<"parameterized arithmetic mismatch\n";return 1;}}
