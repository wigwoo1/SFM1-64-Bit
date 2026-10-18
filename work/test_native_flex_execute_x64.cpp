#include "global_flex_execute.h"
#include "flex_execute_fixture.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream oracle(argv[1],std::ios::binary);
 for(unsigned seed=0;seed<32768;++seed){auto f=flexExecuteFixture(seed);struct Destination{uint16_t flags;}dest{f.destinationFlags};std::array<uint32_t,8>actual{},expected{};unsigned cursor=0,reads=0;
 auto result=recoveredFlexExecuteCandidate(RecoveredFlexBindingCache{f.handle,f.index},[&](uint32_t handle)->Destination*{actual[cursor++]=1;actual[cursor++]=handle;return f.present?&dest:nullptr;},[&]{++reads;return RecoveredFlexSourceValueCandidate{f.sourceFlags,f.bits};},[&](Destination*p,uint32_t index,uint32_t type,uint32_t bits){if(p!=&dest)throw std::runtime_error("destination");actual[cursor++]=2;actual[cursor++]=index;actual[cursor++]=type;actual[cursor++]=bits;},[&](Destination*p,uint32_t type,uint32_t bits){if(p!=&dest)throw std::runtime_error("destination");actual[cursor++]=3;actual[cursor++]=type;actual[cursor++]=bits;});actual[7]=cursor;
 if(reads!=(f.present?1u:0u)||(result==RecoveredFlexExecuteDispatchCandidate::MissingDestination)!=!f.present)throw std::runtime_error("lazy source read");if(!oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected))||actual!=expected)throw std::runtime_error("native mismatch seed="+std::to_string(seed));}
 if(oracle.peek()!=EOF)throw std::runtime_error("trailing oracle");std::cout<<"x64 flex execute: 32768 native dispatch comparisons passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
