#include "overlay_duration_division_candidate.h"
#include <iostream>
#include <limits>
int main(){
 static_assert(sizeof(void*)==8,"x64 required");
 struct Case{int32_t input;float divisor;int32_t expected;};
 const Case cases[]={{1,2,1},{-1,2,0},{5,2,3},{-5,2,-2},{10,3,3},{10001,3,3334},{0,1,0},{16777217,1,16777216},{1,0,INT32_MIN},{0,0,INT32_MIN}};
 unsigned checked=0;
 for(auto c:cases){auto value=c.input;auto result=overlayDurationDivisionCandidate(value,c.divisor);
  if(result!=c.expected||value!=result){std::cerr<<"case failed "<<c.input<<'/'<<c.divisor<<" got "<<result<<'\n';return 1;}++checked;
 }
 std::cout<<"{\"pointer_bits\":64,\"cases_passed\":"<<checked<<",\"scope\":\"SSE_translation_edge_cases_default_MXCSR\",\"original_runtime_parity\":false}\n";
}
