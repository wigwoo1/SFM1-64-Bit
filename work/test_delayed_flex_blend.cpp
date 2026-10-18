#include "delayed_flex_blend_candidate.h"
#include <fstream>
#include <vector>
#include <limits>
#include <iostream>
int main(){
 std::ifstream input("work/delayed-flex-blend-input.bin",std::ios::binary);
 std::ofstream output("outputs/sfm64-delayed-flex-blend-output.bin",std::ios::binary);
 if(!input||!output)return 1;
 float tuple[3]; std::size_t cases=0;
 while(input.read(reinterpret_cast<char*>(tuple),sizeof(tuple))){
  delayedFlexBlendCandidate(tuple,tuple+1,1,tuple[2]);
  output.write(reinterpret_cast<char*>(tuple+1),sizeof(float));++cases;
 }
 if(!input.eof()||!output||cases!=65536)return 2;
 float history[]={0.5f,0.25f};float invalid[]={0.8f,std::numeric_limits<float>::quiet_NaN()};
 try{delayedFlexBlendCandidate(invalid,history,2,0.8f);return 3;}catch(const std::runtime_error&){}
 if(history[0]!=0.5f||history[1]!=0.25f)return 4;
 delayedFlexBlendCandidate(nullptr,nullptr,0,1.0f);
 std::cout<<"delayed blend arithmetic cases="<<cases<<" invalid input preserves history\n";
}
