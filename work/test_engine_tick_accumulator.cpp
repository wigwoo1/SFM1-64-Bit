#include "engine_tick_accumulator_candidate.h"
#include <iostream>
#include <stdexcept>
static uint32_t bits(float v){uint32_t w;std::memcpy(&w,&v,4);return w;}
int main(){try{
 unsigned checks=0;
 // Exact rational workloads: interval1/4, elapsed n/64. Independent integer oracle.
 for(int n=0;n<4096;++n)for(int parity=0;parity<2;++parity)for(bool alternate:{false,true}){
  auto r=engineTickAccumulatorCandidate(0,bits(float(n)/64),bits(.25f),false,alternate,parity);
  int expected=n/16;
  if(alternate && n>=16 && ((parity+expected)&1))++expected;
  float remainder=float(n-expected*16)/64;
  if(r.ticks!=expected||r.remainderWord!=bits(remainder))throw std::runtime_error("rational workload mismatch");
  ++checks;
 }
 auto frozen=engineTickAccumulatorCandidate(bits(.125f),bits(100),bits(.25f),true,false,0);
 if(frozen.ticks||frozen.remainderWord!=bits(.125f))throw std::runtime_error("retained accumulator changed");
 auto negative=engineTickAccumulatorCandidate(bits(-.125f),0,bits(.25f),true,false,0);
 if(negative.ticks||negative.remainderWord!=bits(-.125f))throw std::runtime_error("negative remainder altered");
 auto nan=engineTickAccumulatorCandidate(0x7fc01234,0,bits(.25f),true,false,0);
 if(nan.ticks||nan.remainderWord!=0x7fc01234)throw std::runtime_error("unordered branch altered");
 std::cout<<"tick_accumulator_rational_checks="<<checks<<" boundary_checks=3 original_execution_parity=unverified\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
