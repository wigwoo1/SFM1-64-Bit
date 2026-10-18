#include "recovered_tick_seconds_candidate.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream file(argv[1],std::ios::binary);std::vector<char>bytes((std::istreambuf_iterator<char>(file)),{});if(bytes.size()!=18*8)throw std::runtime_error("oracle size");for(size_t offset=0;offset<bytes.size();offset+=8){int32_t tick;float expected;std::memcpy(&tick,bytes.data()+offset,4);std::memcpy(&expected,bytes.data()+offset+4,4);auto actual=recoveredTickSecondsCandidate(tick);if(std::memcmp(&actual,&expected,4))throw std::runtime_error("native seconds mismatch");}if(recoveredTickSecondsCandidate(10000)!=1.0f||recoveredTickSecondsCandidate(-10000)!=-1.0f)throw std::runtime_error("seconds unit");std::cout<<"x64 tick seconds passed native_cases=18 bitwise_equal=1 nominal_ticks_per_second=10000 constructor_and_frame_rounding=unverified\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
