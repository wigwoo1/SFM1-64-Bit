#include "float_mutation.h"
#include <limits>
#include <iostream>
#include <stdexcept>
int main(){try{float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
 if(floatMutationNeeded(1,1)||floatMutationNeeded(0,-0.0f)||floatMutationNeeded(inf,inf)||!floatMutationNeeded(1,2)||!floatMutationNeeded(nan,nan)||!floatMutationNeeded(1,nan)||!floatMutationNeeded(nan,1))throw std::runtime_error("mutation predicate mismatch");
 std::cout<<"7 float mutation cases passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
