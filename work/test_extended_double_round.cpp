#include "extended_double_round.h"
#include <iostream>
ExtendedLimbs input(uint64_t retained,uint32_t guard){uint64_t upper=(retained<<11)|(guard>>16);return {uint16_t(guard),uint16_t(upper),uint16_t(upper>>16),uint16_t(upper>>32),uint16_t(upper>>48)};}
int main(){size_t count=0;
 for(auto retained:{uint64_t(1)<<52,(uint64_t(1)<<52)+1,(uint64_t(1)<<53)-1})
 for(auto guard:{0u,0x3ffffffu,0x4000000u,0x4000001u,0x7ffffffu}){
  auto actual=roundExtendedTo53(input(retained,guard));uint64_t expected=retained+(guard>0x4000000u?1:0);bool carry=(expected>>53)!=0;if(carry)expected=uint64_t(1)<<52;
  if(actual.bits!=expected||actual.exponentCarry!=carry)return 1;++count;
 }
 std::cout<<count<<" extended-to-double rounding boundary cases passed\n";
}
