#include "parameterized_curve_cubic_weights.h"
#include <iostream>
#include <cmath>
int main(){static_assert(sizeof(void*)==8,"x64 required");auto base=recoveredParameterizedCubicWeights({0,0,0});if(base!=std::array<float,4>{-1,3,-3,1})return 1;auto zero=recoveredParameterizedCubicWeights({1,0,0});if(zero!=std::array<float,4>{0,4,-4,0})return 1;for(auto code:{7u,8u,9u}){auto w=recoveredParameterizedCubicWeights(recoveredParameterizedCurve(code));float sum=0;for(auto x:w)sum+=x;if(std::abs(sum)>1e-6f)return 1;}auto a=recoveredParameterizedCubicWeights(recoveredParameterizedCurve(8)),b=recoveredParameterizedCubicWeights(recoveredParameterizedCurve(9));if(a[0]!=0||b[3]!=0||a[3]<=0||b[0]>=0)return 1;std::cout<<"parameterized_cubic_checks=6\n";}
