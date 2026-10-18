#include "controller_registration_hash_comparison_candidate.h"
#include <cassert>
#include <iostream>
int main(){std::uint32_t code=0;for(unsigned i=0;i<1000000;++i){code=code*1664525u+1013904223u;auto folded=code^((((code>>12)^code)>>12)&0xfffu);assert(controllerRegistrationHashCandidate(code)==(folded^((folded%19u)*13u)));}
 assert(controllerRegistrationCompareCandidate(0,0xffffffff)==1);assert(controllerRegistrationCompareCandidate(0x80000000,0)==(-2147483647-1));assert(controllerRegistrationCompareCandidate(7,7)==0);
 std::cout<<"x64 registration callbacks: 1000000 independent modulo checks and wrapped comparison boundaries passed\n";
}
