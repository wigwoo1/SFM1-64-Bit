#include "clock_correction_gate_candidate.h"
#include <cstring>
#include <cmath>
#include <iostream>
#include <stdexcept>
int main(){
 const uint32_t boundary[]={0,0x80000000u,1,0x80000001u,0x7f800000u,0xff800000u,0x7fc01234u,0xffc01234u,0x7f801234u,0xff801234u};
 uint32_t seed=0x1743990;unsigned checked=0;
 for(unsigned i=0;i<65536;++i){
  seed=seed*1664525u+1013904223u;uint32_t word=i<10?boundary[i]:seed;
  float value;std::memcpy(&value,&word,4);
  for(int state:{-1,0,1,2,3})for(int enabled:{0,1,-1}){
   bool expected=enabled!=0&&(state<2||std::isnan(value)||value>0);
   if(clockCorrectionGateCandidate(state,word,enabled)!=expected)throw std::runtime_error("clock gate mismatch");
   ++checked;
  }
 }
 std::cout<<"clock_gate_reference_cases="<<checked<<"\n";
}
