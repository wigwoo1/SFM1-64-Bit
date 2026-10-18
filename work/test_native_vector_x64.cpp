#include <fstream>
#include <iostream>
#include <cstring>
#include "vector_key_sampler.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;
 std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");
 unsigned checks=0,mismatches=0;
 for(;;){int32_t tick;std::array<int32_t,4> times;std::array<std::array<float,3>,4> values;std::array<float,3> expected;
  f.read(reinterpret_cast<char*>(&tick),4);if(f.gcount()==0&&f.eof())break;if(!f)throw std::runtime_error("truncated tick");
  f.read(reinterpret_cast<char*>(times.data()),16);f.read(reinterpret_cast<char*>(values.data()),48);f.read(reinterpret_cast<char*>(expected.data()),12);if(!f)throw std::runtime_error("truncated oracle");
  VectorKeySampler sampler({times.begin(),times.end()},{values.begin(),values.end()});auto actual=sampler.sample(tick);
  if(std::memcmp(actual.data(),expected.data(),12))++mismatches;++checks;
 }
 if(checks!=3303)throw std::runtime_error("unexpected corpus count");
 std::cout<<"native_vector_x86_vs_x64_checks="<<checks<<" mismatches="<<mismatches<<'\n';return mismatches?2:0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
