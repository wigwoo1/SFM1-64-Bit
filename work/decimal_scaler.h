#pragma once
#include "extended_multiply.h"
#include "decimal_scaling_plan.h"
inline NormalizedExtended scaleDecimalExtended(NormalizedExtended value,int exponent){
 struct Constant{uint32_t address;NormalizedExtended value;};
 static const Constant constants[]={
#include "decimal_scaling_constants.inc"
 };
 auto plan=decimalScalingPlan(exponent);
 if(!plan.empty())value.significand[0]=0; // native wrapper's precision argument0
 for(auto address:plan){bool found=false;
  for(auto&entry:constants)if(entry.address==address){auto multiplier=entry.value;
   if(multiplier.significand[0]>=0x8000){uint32_t middle=uint32_t(multiplier.significand[1])|(uint32_t(multiplier.significand[2])<<16);--middle;multiplier.significand[1]=uint16_t(middle);multiplier.significand[2]=uint16_t(middle>>16);}
   value=multiplyExtended(value,multiplier);found=true;break;
  }if(!found)throw std::runtime_error("missing recovered decimal constant");
 }
 return value;
}
