#include <iostream>
#include <limits>
#include "studio_shader_cloak_gate_candidate.h"
struct Ops {
 int32_t& index;float factor;uint32_t definition;unsigned queries=0,reads=0;
 uint32_t defined(int32_t i){if(i!=index)throw 1;++queries;index=37;return definition;}
 float value(int32_t i){if(i!=37)throw 2;++reads;return factor;}
};
int main(){unsigned cases=0;
 for(int32_t initial:{-17,-1,0,19})for(uint32_t defined:{0u,1u,256u,257u})for(unsigned n=0;n<=1024;++n){
  // n/512 covers0..2 exactly; crossing occurs between227/512 and228/512.
  float factor=float(n)/512.f;int32_t index=initial;Ops ops{index,factor,defined};
  auto result=studioShaderCloakGateCandidate(index,ops);bool read=initial>=0&&uint8_t(defined)!=0;
  if(result!=(read&&n>=228)||ops.queries!=unsigned(initial>=0)||ops.reads!=unsigned(read))return 1;++cases;
 }
 for(float f:{-1.f,-0.f,0.f,std::numeric_limits<float>::quiet_NaN(),-std::numeric_limits<float>::infinity()})if(studioShaderCloakOpaqueCandidate(f))return 2;
 if(!studioShaderCloakOpaqueCandidate(std::numeric_limits<float>::infinity()))return 3;
 std::cout<<"cloak_gate_cases="<<cases<<" low_byte_defined=1 live_index_reload=1 scalar_sse=1 exceptional_values=6\n";
}
