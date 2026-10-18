#include "studio_shader_color_conversion_candidate.h"
#include <cstdio>
#include <limits>
struct Ops {
 uint8_t a,b;uint32_t first,second;unsigned calls=0,lookups=0;
 uint8_t flag5aa(){return a;}uint8_t flag5a9(){return b;}
 uint32_t mode(){return calls++?second:first;}
 float lookup(float value){++lookups;return value;}
 float convertedScale(){return 1.f;}float rawScale(){return 1.f;}
};
struct BoundaryOps {
 float factor;bool converted;unsigned lookups=0,scaleReads=0;
 uint8_t flag5aa(){return converted?1:0;}uint8_t flag5a9(){return 0;}
 uint32_t mode(){return 1;}
 float lookup(float value){++lookups;return value;}
 float convertedScale(){++scaleReads;return factor;}
 float rawScale(){++scaleReads;return factor;}
};
struct MutationOps {
 unsigned modes=0,lookups=0;float factor=1;
 uint8_t flag5aa(){return 0;}uint8_t flag5a9(){return 0;}
 uint32_t mode(){return modes++?1:2;}
 float lookup(float value){
  const float expected[]{1.f,0.f,1.f};
  if(value!=expected[lookups])throw 1;
  ++lookups;if(lookups==3)factor=0;return value;
 }
 float convertedScale(){if(lookups!=3)throw 2;return factor;}
 float rawScale(){throw 3;}
};
int main(){unsigned cases=0;
 for(unsigned a=0;a<2;++a)for(unsigned b=0;b<2;++b)for(uint32_t flag:{0u,1u,256u,257u})for(uint32_t first:{0u,1u,2u})for(uint32_t second:{0u,1u,2u}){
  Ops ops{uint8_t(a),uint8_t(b?128:0),first,second};
  const bool conditional=!a&&uint8_t(flag)&&!b;
  const bool converted=a||(uint8_t(flag)&&(b||first==2));
  const auto packed=studioShaderColorConversionCandidate({255,0,255},flag,ops);
  if(packed!=0xffff00ffu||ops.calls!=(conditional?2u:1u)||ops.lookups!=(converted?3u:0u))return 1;
  ++cases;
 }
 unsigned boundaries=0;
 const float factors[]{0.f,1.f,2.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()};
 const uint32_t expected[]{0xff000000u,0xffff00ffu,0xffff00ffu,0xff010001u,0xffffffffu,0xffffffffu};
 for(bool converted:{false,true})for(unsigned i=0;i<6;++i){
  BoundaryOps ops{factors[i],converted};
  const auto value=studioShaderColorConversionCandidate({255,0,255},0,ops);
  if(value!=expected[i]||ops.lookups!=(converted?3u:0u)||ops.scaleReads!=1)return 2;
  ++boundaries;
 }
 MutationOps mutation;
 if(studioShaderColorConversionCandidate({255,0,255},1,mutation)!=0xff000000u||mutation.modes!=2||mutation.lookups!=3)return 3;
 std::printf("shader_color_conversion_cases=%u boundary_cases=%u callback_mutation_cases=1 configuration_gates=1 live_mode_calls=1 lookup_count=1\n",cases,boundaries);return 0;
}
