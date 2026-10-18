#include "array_reference_range_assignment.h"
#include "array_reference_range_fixture.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle path required");std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("oracle unavailable");
 for(unsigned seed=0;seed<5376;++seed){auto f=arrayRangeFixture(seed);std::vector<uint32_t>values(f.old.begin(),f.old.end()),incoming(f.incoming.begin(),f.incoming.begin()+f.count);std::array<uint32_t,128>out{},expected{};unsigned cursor=0;auto emit=[&](uint32_t c,uint32_t a){out[cursor++]=c;out[cursor++]=a;out[cursor++]=arrayRangeHash(values.data());};uint32_t flags=0;
 assignRecoveredReferenceArrayRangeCandidate(values,f.start,incoming,42,[&](){emit(1,0);return f.allow;},[](size_t,size_t){},[&](uint32_t h){emit(2,h);},[&](uint32_t h){emit(4,h);return h>=1&&h<=4?h:0u;},[&](uint32_t h,uint32_t cls){if(cls!=42)throw std::runtime_error("class");emit(5,h);return !(f.reject&&(h==2||h==4));},[](size_t,size_t){},[&](uint32_t h){emit(3,h);},[&](){emit(6,1);flags|=0x1000;});
 for(unsigned i=0;i<5;++i)out[120+i]=values[i];out[125]=cursor;out[126]=flags;input.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!input||out!=expected)throw std::runtime_error("native mismatch seed "+std::to_string(seed));
 }if(input.peek()!=EOF)throw std::runtime_error("trailing oracle");std::cout<<"x64_array_range_cases=5376 native_payload_event_order_flags=matched\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
