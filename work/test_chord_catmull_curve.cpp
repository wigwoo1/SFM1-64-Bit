#include "float_curve_evaluation.h"
#include <iostream>
#include <cmath>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");auto close=[](float a,float b){if(std::abs(a-b)>2e-6f)throw 1;};std::array<float,4>t{0,1,3,7},v{0,1,3,7};for(float f:{0.f,.25f,.5f,.75f,1.f})close(evaluateRecoveredChordCatmull(t,v,f),1+2*f);close(evaluateRecoveredChordCatmull({1,1,3,3},{0,0,1,1},.5f),.5f);close(evaluateRecoveredChordCatmull({0,1,1,2},{0,4,4,9},.5f),4);close(evaluateRecoveredFloatCurve(t,v,{0,0xC0C,0xC0C,0},.5f,0,10),2);std::cout<<"chord_curve_checks=8 mode=12\n";return 0;}catch(...){std::cerr<<"chord curve mismatch\n";return 1;}}
