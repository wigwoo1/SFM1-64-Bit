#include "extended_rounding.h"
#include <iostream>
int main(){for(uint32_t retained=0;retained<2;++retained)for(uint32_t guard=0;guard<=0xffffu;++guard){
 bool expected=guard>32768u||(guard==32768u&&retained==1);
 if(extendedScaleRoundUp((retained<<16)|guard)!=expected)return 1;
 }std::cout<<"131072 extended rounding predicate cases passed\n";
}
