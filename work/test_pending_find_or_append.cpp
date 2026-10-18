#include "pending_reference_find_or_append.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)throw std::runtime_error("oracle path required");std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");unsigned cases=0;
 for(uint32_t bucket=0;bucket<4;++bucket)for(uint32_t index=0;index<8;++index)for(uint32_t result=0;result<8;++result)for(unsigned hit=0;hit<2;++hit){uint32_t finds=0,appends=0,seen=0xffffffffu;
 auto actual=recoveredPendingFindOrAppendCandidate(17,[&](int key,uint32_t&b,uint32_t&i){if(key!=17)throw std::runtime_error("wrong lookup key");++finds;b=bucket;if(hit)i=index;return hit!=0;},[](uint32_t b){return b+9;},[&](uint32_t b,uint32_t i,int key){if(b!=bucket||key!=17)throw std::runtime_error("wrong append inputs");++appends;seen=i;return result;});
 std::array<uint32_t,4> expected{},got{actual,finds,appends,seen};f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||expected!=got)throw std::runtime_error("native wrapper mismatch");++cases;
 }if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra oracle data");std::cout<<"pending_find_or_append_native_comparisons="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
