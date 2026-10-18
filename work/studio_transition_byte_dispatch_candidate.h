#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
// State/operation storage is captured by the owner before dispatch and remains
// alive through callbacks. Each next opcode is read after the preceding call.
template<class State,std::size_t Ordinary,std::size_t Packed>
void studioTransitionByteDispatchCandidate(State* state,const std::uint8_t* operations,
 std::int32_t count,const std::array<void(*)(State*,unsigned),Ordinary>& ordinary,
 const std::array<void(*)(State*,unsigned),Packed>& packed){
 if(count<=0)return;
 if(!state||!operations)throw std::invalid_argument("transition dispatch storage");
 for(std::int32_t index=0;index<count;++index){const auto opcode=operations[index];
  const bool compressed=(opcode&0x80)!=0;
  const unsigned handler=compressed?((opcode&0x7f)>>4):(opcode&0x7f);
  const unsigned argument=compressed?(opcode&0xf):0;
  if(compressed){if(handler>=Packed||!packed[handler])throw std::out_of_range("packed transition handler");packed[handler](state,argument);}
  else{if(handler>=Ordinary||!ordinary[handler])throw std::out_of_range("ordinary transition handler");ordinary[handler](state,argument);}
 }
}
