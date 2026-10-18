#pragma once
#include "normalized_catmull_curve.h"
#include "bspline_curve_component.h"
#include "exponential_curve_evaluation.h"
#include "catmull_derivative_curve.h"
#include "chord_catmull_curve.h"
#include "parameterized_curve_component.h"
#include "two_point_curve_evaluation.h"
#include "curve_evaluation_envelope.h"
inline float evaluateRecoveredFloatCurve(const std::array<float,4>&times,const std::array<float,4>&values,const std::array<int32_t,4>&kinds,float fraction,float minimum,float maximum){
 auto evaluate=[&](uint32_t code,float f){switch(code){
 case 0:case 1:return evaluateRecoveredNormalizedCatmull(times,values,f);
 case 2:case 3:case 4:case 6:case 10:return evaluateRecoveredTwoPointCurve(code,f,values[1],values[2]);
 case 5:return evaluateRecoveredBSplineComponent(values[0],values[1],values[2],values[3],f);
 case 7:case 8:case 9:{auto v=normalizeRecoveredCurveValues(times,values);return evaluateRecoveredParameterizedComponent(v[0],v[1],v[2],v[3],f,recoveredParameterizedCurve(code));}
 case 12:return evaluateRecoveredChordCatmull(times,values,f);
 case 13:return evaluateRecoveredCatmullDerivative(values[0],values[1],values[2],values[3],f);
 case 14:return evaluateRecoveredExponentialCurve(times,values,f);
 case 11:return evaluateRecoveredCatmullComponent(values[0],values[1],values[2],values[3],f);
 //10066CC0 emits diagnostic, then falls through to1049AC20.
 default:return evaluateRecoveredNormalizedCatmull(times,values,f);
 }};
 return evaluateRecoveredCurveEnvelope(fraction,values[1],values[2],kinds[1],kinds[2],minimum,maximum,evaluate);
}
