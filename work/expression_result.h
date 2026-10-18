#pragma once
#include <vector>
#include <xmmintrin.h>
#include <cstdint>
#include <cstring>
enum class ExpressionUnary { Plus,Minus,Not };
// Native sign() uses COMISS/JB, so unordered also selects -1.
inline float expressionSign(float value) {
 return value>=0.0f?1.0f:-1.0f;
}
inline float expressionMin(float left,float right){return right>left?left:right;}
inline float expressionMax(float left,float right){return left>right?left:right;}
// Exact SSE sequence at 0x10A5EC52; depends on current MXCSR rounding mode.
inline float expressionFloor(float value){
 auto input=_mm_set_ss(value);
 auto sign=_mm_and_ps(_mm_set_ss(-0.0f),input);
 auto magnitude=_mm_xor_ps(input,sign);
 auto large=_mm_set_ss(8388608.0f);
 auto adjustment=_mm_or_ps(_mm_and_ps(large,_mm_cmplt_ss(magnitude,large)),sign);
 auto rounded=_mm_sub_ss(_mm_add_ss(input,adjustment),adjustment);
 auto correction=_mm_and_ps(_mm_cmpnle_ss(_mm_sub_ss(rounded,input),sign),_mm_set_ss(1.0f));
 return _mm_cvtss_f32(_mm_sub_ss(rounded,correction));
}
inline float expressionRound(float value){return expressionFloor(_mm_cvtss_f32(_mm_add_ss(_mm_set_ss(value),_mm_set_ss(0.5f))));}
inline float expressionUnary(ExpressionUnary op,float value) {
 if(op==ExpressionUnary::Not)return value==0.0f?1.0f:0.0f;
 if(op==ExpressionUnary::Minus){uint32_t bits;std::memcpy(&bits,&value,4);bits^=0x80000000u;std::memcpy(&value,&bits,4);}
 return value;
}
// Recovered from ifm.dll 0x10A601E0. Parser not implemented here.
inline bool finishExpression(bool hasText, bool parsed,
                             const std::vector<float>& stack, float& result) {
 if (!hasText || !parsed || stack.size()!=1) { result=0.0f; return false; }
 result=stack.back(); return true;
}
// 0x10A60191: ordered zero is false; unordered (NaN) is true.
// Both branch values have already been evaluated by the parser.
inline float expressionConditional(float condition,float yes,float no) {
 return condition!=0.0f ? yes : no;
}
// Operands are evaluated before these helpers; no short circuit.
inline float expressionAnd(float left,float right) {
 return (left!=0.0f && right!=0.0f) ? 1.0f : 0.0f;
}
inline float expressionOr(float left,float right) {
 return (left!=0.0f || right!=0.0f) ? 1.0f : 0.0f;
}
enum class ExpressionComparison { Less,Greater,LessEqual,GreaterEqual,Equal,NotEqual };
enum class ExpressionArithmetic { Add,Subtract,Multiply,Divide };
inline float expressionArithmetic(ExpressionArithmetic op,float left,float right) {
 const auto a=_mm_set_ss(left),b=_mm_set_ss(right);
 switch(op){
 case ExpressionArithmetic::Add:return _mm_cvtss_f32(_mm_add_ss(a,b));
 case ExpressionArithmetic::Subtract:return _mm_cvtss_f32(_mm_sub_ss(a,b));
 case ExpressionArithmetic::Multiply:return _mm_cvtss_f32(_mm_mul_ss(a,b));
 case ExpressionArithmetic::Divide:return _mm_cvtss_f32(_mm_div_ss(a,b));
 }
 return 0.0f;
}
inline float expressionInRange(float value,float low,float high){return value>=low&&high>=value?1.0f:0.0f;}
inline float expressionClamp(float value,float low,float high){if(value>high)return high;return low>value?low:value;}
inline float expressionRamp(float value,float low,float high){
 auto numerator=expressionArithmetic(ExpressionArithmetic::Subtract,value,low);
 auto denominator=expressionArithmetic(ExpressionArithmetic::Subtract,high,low);
 return expressionArithmetic(ExpressionArithmetic::Divide,numerator,denominator);
}
inline float expressionLerp(float fraction,float low,float high){
 auto delta=expressionArithmetic(ExpressionArithmetic::Subtract,high,low);
 auto scaled=expressionArithmetic(ExpressionArithmetic::Multiply,delta,fraction);
 return expressionArithmetic(ExpressionArithmetic::Add,scaled,low);
}
inline float expressionClampedRamp(float value,float low,float high){return expressionClamp(expressionRamp(value,low,high),0,1);}
inline float expressionClampedLerp(float fraction,float low,float high){return expressionClamp(expressionLerp(fraction,low,high),low,high);}
inline float expressionRescale(float value,float inputLow,float inputHigh,float outputLow,float outputHigh){
 return expressionLerp(expressionRamp(value,inputLow,inputHigh),outputLow,outputHigh);
}
inline float expressionClampedRescale(float value,float inputLow,float inputHigh,float outputLow,float outputHigh){
 return expressionClamp(expressionRescale(value,inputLow,inputHigh,outputLow,outputHigh),outputLow,outputHigh);
}
inline float expressionEasedLerp(float t,float low,float high){
 using A=ExpressionArithmetic;
 auto cubic=expressionArithmetic(A::Multiply,t,2);
 cubic=expressionArithmetic(A::Multiply,cubic,t);
 cubic=expressionArithmetic(A::Multiply,cubic,t);
 auto quadratic=expressionArithmetic(A::Multiply,t,3);
 quadratic=expressionArithmetic(A::Multiply,quadratic,t);
 auto eased=expressionArithmetic(A::Subtract,quadratic,cubic);
 return expressionLerp(eased,low,high);
}
inline float expressionCompare(ExpressionComparison op,float left,float right) {
 switch(op){
 case ExpressionComparison::Less:return left<right?1.0f:0.0f;
 case ExpressionComparison::Greater:return left>right?1.0f:0.0f;
 case ExpressionComparison::LessEqual:return left<=right?1.0f:0.0f;
 case ExpressionComparison::GreaterEqual:return left>=right?1.0f:0.0f;
 case ExpressionComparison::Equal:return left==right?1.0f:0.0f;
 case ExpressionComparison::NotEqual:return left!=right?1.0f:0.0f;
 }
 return 0.0f;
}
