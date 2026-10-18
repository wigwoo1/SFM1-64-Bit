#pragma once
#include "expression_tokens.h"
#include "expression_result.h"
#include "decimal_crt.h"
#include "expression_ceiling.h"
#include "expression_modulo.h"
// Recovered operator grammar and verified builtin arithmetic. Remaining math
// callees and variable collection mode are pending integrations.
class ExpressionParser {
 std::string_view text;const ExpressionVariables&variables;size_t position=0;int numericError=0;
 bool function(float&value){
  struct Builtin{const char*name;unsigned arity;};
  static const Builtin builtins[]={
   {"abs",1},{"sqr",1},{"sqrt",1},{"dtor",1},{"rtod",1},{"floor",1},{"round",1},{"sign",1},
   {"min",2},{"max",2},{"inrange",3},{"clamp",3},{"ramp",3},{"lerp",3},{"cramp",3},{"clerp",3},{"elerp",3},{"rescale",5},{"crescale",5},{"ceiling",1}
  };
  const size_t start=position;
  for(unsigned index=0;index<sizeof builtins/sizeof builtins[0];++index){
   position=start;const auto&builtin=builtins[index];float a[5]{};
   if(!expressionToken(text,position,builtin.name)||!expressionToken(text,position,"("))continue;
   bool valid=true;for(unsigned i=0;i<builtin.arity;++i){if((i&&!expressionToken(text,position,","))||!conditional(a[i])){valid=false;break;}}
   if(!valid||!expressionToken(text,position,")"))continue;
   using A=ExpressionArithmetic;
   switch(index){
    case 0:{uint32_t bits;std::memcpy(&bits,a,4);bits&=0x7fffffffu;std::memcpy(&value,&bits,4);break;}
    case 1:value=expressionArithmetic(A::Multiply,a[0],a[0]);break;
    case 2:value=_mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(a[0])));break;
    case 3:value=expressionArithmetic(A::Multiply,a[0],0.01745329238474369f);break;
    case 4:value=expressionArithmetic(A::Multiply,a[0],57.295780181884766f);break;
    case 5:value=expressionFloor(a[0]);break;
    case 6:value=expressionRound(a[0]);break;
    case 7:value=expressionSign(a[0]);break;
    case 8:value=expressionMin(a[0],a[1]);break;
    case 9:value=expressionMax(a[0],a[1]);break;
    case 10:value=expressionInRange(a[0],a[1],a[2]);break;
    case 11:value=expressionClamp(a[0],a[1],a[2]);break;
    case 12:value=expressionRamp(a[0],a[1],a[2]);break;
    case 13:value=expressionLerp(a[0],a[1],a[2]);break;
    case 14:value=expressionClampedRamp(a[0],a[1],a[2]);break;
    case 15:value=expressionClampedLerp(a[0],a[1],a[2]);break;
    case 16:value=expressionEasedLerp(a[0],a[1],a[2]);break;
    case 17:value=expressionRescale(a[0],a[1],a[2],a[3],a[4]);break;
    case 18:value=expressionClampedRescale(a[0],a[1],a[2],a[3],a[4]);break;
    case 19:{auto ceiling=expressionCeiling(a[0]);if(!ceiling)continue;value=*ceiling;break;}
   }
   return true;
  }
  position=start;return false;
 }
 bool primary(float&value){
  const size_t start=position;
  if(expressionNumber(text,position,value,numericError))return true;
  if(variables.match(text,position,value))return true;
  if(expressionToken(text,position,"(")&&conditional(value)&&expressionToken(text,position,")"))return true;
  position=start;return function(value);
 }
 bool unary(float&value){
  const size_t start=position;
  for(auto token:{"+","-","!"}){
   if(expressionToken(text,position,token)){
    if(unary(value)){value=expressionUnary(token[0]=='+'?ExpressionUnary::Plus:token[0]=='-'?ExpressionUnary::Minus:ExpressionUnary::Not,value);return true;}
    position=start;return false;
   }
  }
  return primary(value);
 }
 bool binary(unsigned level,float&value){
  if(level==6)return unary(value);
  const size_t start=position;
  if(!binary(level+1,value)){position=start;return false;}
  const float left=value;const size_t afterLeft=position;
  const char*token=nullptr;
  auto match=[&](const char*candidate){if(expressionToken(text,position,candidate)){token=candidate;return true;}return false;};
  switch(level){
   case 0:match("||");break;
   case 1:match("&&");break;
   case 2:if(!match("=="))match("!=");break;
   case 3:if(!match("<=" )&&!match(">=")&&!match("<"))match(">");break;
   case 4:if(!match("+"))match("-");break;
   case 5:if(!match("*")&&!match("/"))match("%");break;
  }
  if(!token)return true;
  float right;
  if(!binary(level,right)){position=afterLeft;value=left;return true;}
  if(level==0)value=expressionOr(left,right);
  else if(level==1)value=expressionAnd(left,right);
  else if(level==2)value=expressionCompare(token[0]=='='?ExpressionComparison::Equal:ExpressionComparison::NotEqual,left,right);
  else if(level==3)value=expressionCompare(token[0]=='<'?(token[1]=='='?ExpressionComparison::LessEqual:ExpressionComparison::Less):(token[1]=='='?ExpressionComparison::GreaterEqual:ExpressionComparison::Greater),left,right);
  else if(token[0]=='%')value=*expressionModulo(left,right);
  else value=expressionArithmetic(token[0]=='+'?ExpressionArithmetic::Add:token[0]=='-'?ExpressionArithmetic::Subtract:token[0]=='*'?ExpressionArithmetic::Multiply:ExpressionArithmetic::Divide,left,right);
  return true;
 }
 bool conditional(float&value){
  const size_t start=position;if(!binary(0,value)){position=start;return false;}
  const size_t afterLeft=position;const float condition=value;
  if(!expressionToken(text,position,"?"))return true;
  float yes,no;
  if(conditional(yes)&&expressionToken(text,position,":")&&conditional(no)){value=expressionConditional(condition,yes,no);return true;}
  position=afterLeft;value=condition;return true;
 }
public:
 ExpressionParser(std::string_view source,const ExpressionVariables&bindings):text(source),variables(bindings){}
 bool evaluate(float&result){position=0;numericError=0;float value=0;bool parsed=conditional(value);return finishExpression(true,parsed,parsed?std::vector<float>{value}:std::vector<float>{},result);}
 size_t consumed()const{return position;}
 int error()const{return numericError;}
};
