#include "decimal_scaling_plan.h"
#include <iostream>
int main(){for(int exponent=-5200;exponent<=5200;++exponent){auto plan=decimalScalingPlan(exponent);unsigned decoded=0,prior=0;bool first=true;
 for(auto address:plan){uint32_t offset=address-(exponent<0?0x10F229E8u:0x10F22888u);unsigned group=offset/84,digit=(offset%84)/12+1;
  if(offset%12||(!first&&group<=prior)||digit>7||group>4)return 1;
  decoded+=digit*(1u<<(3*group));prior=group;first=false;
 }if(decoded!=static_cast<unsigned>(exponent<0?-exponent:exponent))return 2;}
 for(int e:{-5201,5201}){bool rejected=false;try{decimalScalingPlan(e);}catch(const std::runtime_error&){rejected=true;}if(!rejected)return 3;}
 std::cout<<"10401 exponent selection cases and 2 range checks passed\n";
}
