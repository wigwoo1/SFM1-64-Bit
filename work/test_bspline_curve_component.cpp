#include "float_curve_evaluation.h"
#include <iostream>
#include <cmath>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");auto close=[](float a,float b){if(std::abs(a-b)>2e-6f)throw std::runtime_error("B-spline mismatch");};close(evaluateRecoveredBSplineComponent(0,0,1,0,0),1.f/6);close(evaluateRecoveredBSplineComponent(0,0,1,0,.5f),23.f/48);close(evaluateRecoveredBSplineComponent(0,0,1,0,1),2.f/3);for(float f:{0.f,.25f,.5f,.75f,1.f})close(evaluateRecoveredBSplineComponent(0,1,2,3,f),1+f);close(evaluateRecoveredFloatCurve({0,1,2,3},{0,0,1,0},{0,0x505,0x505,0},.5f,0,1),23.f/48);std::cout<<"bspline_checks=9 mode=5\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
