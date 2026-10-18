#include "reference_mark_policy.h"
#include <iostream>
int main(){unsigned checks=0;for(unsigned flags=0;flags<65536;++flags)for(unsigned mode=0;mode<5;++mode){bool expected=((flags&31)==1||(flags&31)==15);if(mode>=3)expected=false;else if(mode==1)expected=expected&&!(flags&4096);else if(mode==2)expected=expected&&(flags&2048)&&!(flags&4096);if(referenceMarkTraverses(uint16_t(flags),mode)!=expected)return 1;++checks;}std::cout<<"reference_mark_policy_checks="<<checks<<'\n';}
