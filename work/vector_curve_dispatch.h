#pragma once
#include "two_point_curve_evaluation.h"
#include "catmull_curve_component.h"
#include "bspline_curve_component.h"
#include "parameterized_curve_component.h"
#include "exponential_curve_evaluation.h"
// Native 10066D40. This differs from scalar dispatch and the code-1 wrapper.
inline std::array<float,3> evaluateRecoveredVectorDispatch(uint32_t code,const std::array<std::array<float,3>,4>&points,float fraction){
 std::array<float,3> result{};
 if(code==14){std::array<float,4> times{},values{};for(unsigned i=0;i<4;++i){times[i]=points[i][0];values[i]=points[i][1];}result[1]=evaluateRecoveredExponentialCurve(times,values,fraction);return result;}
 if(code==15){result[1]=points[1][1];return result;}
 for(unsigned c=0;c<3;++c){float p0=points[0][c],p1=points[1][c],p2=points[2][c],p3=points[3][c];
  if(code==2||code==3||code==4||code==6||code==10)result[c]=evaluateRecoveredTwoPointCurve(code,fraction,p1,p2);
  else if(code==5)result[c]=evaluateRecoveredBSplineComponent(p0,p1,p2,p3,fraction);
  else if(code>=7&&code<=9)result[c]=evaluateRecoveredParameterizedComponent(p0,p1,p2,p3,fraction,recoveredParameterizedCurve(code));
  else result[c]=evaluateRecoveredCatmullComponent(p0,p1,p2,p3,fraction);
 }
 return result;
}
