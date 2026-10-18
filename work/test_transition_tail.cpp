#include "studio_transition_tail_candidate.h"
#include <cstdio>
struct Ops{
 unsigned input=0,selected=0,byteCalls=0,auxCalls=0,converts=0,pixels=0,renders=0;
 bool disabled=false;uint32_t packed=0;std::array<float,3> normalized{};
 void transitionByte(unsigned value){if(value!=(input&&selected==0?1u:0u))throw 1;++byteCalls;}
 unsigned currentState(){return 1;}unsigned auxiliaryIndex(){return 17;}
 void auxiliary(unsigned index,unsigned enable){if(index!=17||enable!=input)throw 2;++auxCalls;}
 std::array<uint8_t,3> liveColor(){if(auxCalls!=1)throw 3;return {64,96,192};}
 void publishConversionDisabled(bool value){disabled=value;}
 uint32_t convert(std::array<uint8_t,3>,unsigned flag){if(disabled||flag!=0x101)throw 4;++converts;return 0xdeadbeef;}
 void publishNormalized(std::array<float,3> value){normalized=value;}
 void pixelHelper(){++pixels;}uint32_t packedCache(){return packed;}
 void publishPacked(uint32_t value){packed=value;}
 void renderState(unsigned state,unsigned value,unsigned force){if(state!=34||value!=packed||force)throw 5;++renders;}
};
int main(){unsigned cases=0;
 for(unsigned input=0;input<7;++input)for(unsigned selected=0;selected<3;++selected)for(unsigned bypass=0;bypass<2;++bypass){
  Ops ops;ops.input=input;ops.selected=selected;
  studioTransitionTailCandidate(input,selected,0x101,bypass,ops);
  if(ops.byteCalls!=1||ops.auxCalls!=1)return 1;
  if(!input){if(ops.converts||ops.pixels||ops.renders)return 2;}
  else{
   const auto color=studioShaderColorSelectCandidate(input,{64,96,192},bypass);
   if(ops.converts!=unsigned(color.convert)||ops.disabled==color.convert||ops.pixels!=unsigned(selected!=0)||ops.renders!=unsigned(selected==0))return 3;
   if(!selected&&ops.packed!=(color.convert?0xdeadbeefu:color.packed))return 4;
  }
  ++cases;
 }
 std::printf("transition_tail_cases=%u helper_order_and_routing=1\n",cases);
}
