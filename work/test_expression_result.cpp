#include "expression_result.h"
#include <limits>
#include <stdexcept>
#include <iostream>
#include <cmath>
int main(){
 float r=42;
 if(finishExpression(false,true,{7},r)||r!=0) return 1;
 if(finishExpression(true,false,{7},r)||r!=0) return 2;
 if(finishExpression(true,true,{},r)||r!=0) return 3;
 if(finishExpression(true,true,{1,2},r)||r!=0) return 4;
 if(!finishExpression(true,true,{7},r)||r!=7) return 5;
 if(expressionConditional(0,3,4)!=4||expressionConditional(-0.0f,3,4)!=4
 ||expressionConditional(-1,3,4)!=3
 ||expressionConditional(std::numeric_limits<float>::quiet_NaN(),3,4)!=3) return 6;
 float values[]={0.0f,-0.0f,-2.0f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()};
 bool truth[]={false,false,true,true,true};
 for(int i=0;i<5;++i)for(int j=0;j<5;++j){
  if(expressionAnd(values[i],values[j])!=float(truth[i]&&truth[j]))return 7;
  if(expressionOr(values[i],values[j])!=float(truth[i]||truth[j]))return 8;
 }
 float ordered[]={-std::numeric_limits<float>::infinity(),-2,0,-0.0f,3,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()};
 int rank[]={0,1,2,2,3,4,5};
 for(int i=0;i<7;++i)for(int j=0;j<7;++j){
  bool unordered=i==6||j==6;
  bool expected[]={!unordered&&rank[i]<rank[j],!unordered&&rank[i]>rank[j],
   !unordered&&rank[i]<=rank[j],!unordered&&rank[i]>=rank[j],
   !unordered&&rank[i]==rank[j],unordered||rank[i]!=rank[j]};
  for(int k=0;k<6;++k)if(expressionCompare(static_cast<ExpressionComparison>(k),ordered[i],ordered[j])!=float(expected[k]))return 9;
 }
 using A=ExpressionArithmetic;
 if(expressionArithmetic(A::Add,2,3)!=5||expressionArithmetic(A::Subtract,2,3)!=-1
 ||expressionArithmetic(A::Multiply,2,3)!=6||expressionArithmetic(A::Divide,6,3)!=2)return 10;
 if(expressionArithmetic(A::Subtract,10,expressionArithmetic(A::Subtract,3,2))!=9
 ||expressionArithmetic(A::Divide,24,expressionArithmetic(A::Divide,6,2))!=8)return 11;
 float zero=0,one=1,negzero=-0.0f;
 if(!std::isinf(expressionArithmetic(A::Divide,one,zero))
 ||!std::signbit(expressionArithmetic(A::Divide,one,negzero))
 ||!std::isnan(expressionArithmetic(A::Divide,zero,zero))
 ||!std::signbit(expressionArithmetic(A::Multiply,negzero,one)))return 12;
 uint32_t patterns[]={0,0x80000000u,0x3f800000u,0xbf800000u,0x7f800000u,0xff800000u,0x7fc01234u,0xffc05678u,1};
 for(auto bits:patterns){float v;std::memcpy(&v,&bits,4);
  float plus=expressionUnary(ExpressionUnary::Plus,v),minus=expressionUnary(ExpressionUnary::Minus,v);
  uint32_t p,m;std::memcpy(&p,&plus,4);std::memcpy(&m,&minus,4);
  if(p!=bits||m!=(bits^0x80000000u))return 13;
  bool isZero=(bits&0x7fffffffu)==0;
  if(expressionUnary(ExpressionUnary::Not,v)!=float(isZero))return 14;
 }
 float signValues[]={0,-0.0f,1,-1,std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()};
 float signs[]={1,1,1,-1,1,-1,-1};
 for(int i=0;i<7;++i)if(expressionSign(signValues[i])!=signs[i])return 15;
 uint32_t extremaBits[]={0xbf800000u,0,0x80000000u,0x3f800000u,0x7fc01234u};
 int extremaRank[]={0,1,1,2,3};
 for(int i=0;i<5;++i)for(int j=0;j<5;++j){
  float l,h;std::memcpy(&l,&extremaBits[i],4);std::memcpy(&h,&extremaBits[j],4);
  float mn=expressionMin(l,h),mx=expressionMax(l,h);uint32_t n,x;
  std::memcpy(&n,&mn,4);std::memcpy(&x,&mx,4);
  bool comparable=i!=4&&j!=4;
  if(n!=extremaBits[comparable&&extremaRank[i]<extremaRank[j]?i:j]
  ||x!=extremaBits[comparable&&extremaRank[i]>extremaRank[j]?i:j])return 16;
 }
 float qnan=std::numeric_limits<float>::quiet_NaN();
 if(expressionInRange(0,0,1)!=1||expressionInRange(1,0,1)!=1||expressionInRange(2,0,1)!=0||expressionInRange(qnan,0,1)!=0)return 17;
 if(expressionClamp(-1,0,1)!=0||expressionClamp(2,0,1)!=1||!std::isnan(expressionClamp(qnan,0,1))||expressionClamp(2,5,1)!=1)return 18;
 if(expressionRamp(3,2,6)!=0.25f||expressionLerp(0.25f,2,6)!=3||expressionClampedRamp(7,2,6)!=1||expressionClampedLerp(-1,2,6)!=2)return 19;
 if(!std::isnan(expressionClampedRamp(2,2,2))||expressionClampedLerp(0,6,2)!=2)return 20;
 if(expressionRescale(3,2,6,10,18)!=12||expressionRescale(7,2,6,10,18)!=20
 ||expressionClampedRescale(7,2,6,10,18)!=18||expressionClampedRescale(1,2,6,10,18)!=10
 ||expressionRescale(3,6,2,10,18)!=16||expressionClampedRescale(3,2,6,18,10)!=10
 ||!std::isnan(expressionRescale(2,2,2,10,18))||!std::isnan(expressionClampedRescale(qnan,2,6,10,18)))return 21;
 if(expressionEasedLerp(0,2,6)!=2||expressionEasedLerp(1,2,6)!=6
 ||expressionEasedLerp(0.5f,2,6)!=4||expressionEasedLerp(0.25f,0,1)!=0.15625f
 ||expressionEasedLerp(2,0,1)!=-4||!std::isnan(expressionEasedLerp(qnan,0,1)))return 22;
 for(int i=-40;i<=40;++i){float v=float(i)/8;
  if(expressionFloor(v)!=std::floor(v)||expressionRound(v)!=std::floor(v+0.5f))return 23;
 }
 float roundingEdges[]={8388607.0f,8388608.0f,-8388608.0f,std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()};
 for(float v:roundingEdges)if(expressionFloor(v)!=v)return 24;
 if(!std::isnan(expressionFloor(qnan)))return 25;
 std::cout<<"475 prior expression cases and 168 floor/round cases passed\n";
}
