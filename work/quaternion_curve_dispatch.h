#pragma once
#include "quaternion_interpolation.h"
#include "two_point_curve_evaluation.h"
#include "curve_evaluation_envelope.h"
inline std::array<float,4> evaluateRecoveredQuaternionDispatch(uint32_t code,const std::array<std::array<float,4>,4>&points,float fraction){
 if(code==14)return {};if(code==15)return points[1];
 float weight=fraction;if(code==2||code==3||code==4)weight=evaluateRecoveredTwoPointCurve(code,fraction,0,1);
 return interpolateRecoveredQuaternion(points[1],points[2],weight,false);
}
inline std::array<float,4> evaluateRecoveredQuaternionEnvelope(const std::array<std::array<float,4>,4>&points,float fraction,int32_t firstKind,int32_t nextKind){
 auto left=decodeRecoveredCurveKinds(firstKind).outgoing,right=decodeRecoveredCurveKinds(nextKind).incoming;
 if(left==15)return points[1];if(right==15)return points[2];
 auto a=evaluateRecoveredQuaternionDispatch(left,points,fraction);if(left==right)return a;
 return interpolateRecoveredQuaternion(a,evaluateRecoveredQuaternionDispatch(right,points,fraction),fraction,false);
}
