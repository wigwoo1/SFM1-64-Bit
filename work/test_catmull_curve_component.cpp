#include "catmull_curve_component.h"
#include <iostream>
#include <stdexcept>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");if(evaluateRecoveredCatmullComponent(0,2,4,6,0)!=2||evaluateRecoveredCatmullComponent(0,2,4,6,1)!=4||evaluateRecoveredCatmullComponent(0,2,4,6,.25f)!=2.5f||evaluateRecoveredCatmullComponent(0,2,4,6,.5f)!=3)throw std::runtime_error("linear track failed");if(evaluateRecoveredCatmullComponent(0,0,1,0,.5f)!=.5625f||evaluateRecoveredCatmullComponent(0,0,1,0,.25f)!=.2265625f)throw std::runtime_error("curvature failed");std::cout<<"catmull_component_checks=6\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
