#pragma once
#include <cstdint>
// Secondary slot267: compare exact low byte, submit state28, then cache byte.
// Publication is AFTER the callback, unlike several other recovered states.
template<class Ops>bool studioSlot267Candidate(uint32_t input,uint8_t& cache,Ops& ops){
 const auto value=static_cast<uint8_t>(input);
 if(value==cache)return false;
 ops.renderState(28,value,0);
 cache=value;
 return true;
}
