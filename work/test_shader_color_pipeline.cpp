#include "studio_shader_color_pipeline_candidate.h"
#include "studio_color_lookup_candidate.h"
#include <cstdio>
struct Ops {
 std::array<float,256> table;StudioColorLookupRoundingCandidate rounding;unsigned lookups=0;
 uint8_t flag5aa(){return 0;}uint8_t flag5a9(){return 128;}
 uint32_t mode(){return 0;}
 float rawScale(){return 1;}float convertedScale(){return 1;}
 float lookup(float value){++lookups;return studioColorLookupCandidate(value,table,rounding);}
};
int main(){unsigned cases=0;
 using R=StudioColorLookupRoundingCandidate;
 for(uint32_t mode=1;mode<=6;++mode)for(uint32_t conversion:{0u,256u,1u,257u})for(uint32_t bypass:{0u,256u,1u,257u})for(R rounding:{R::NearestEven,R::Down,R::Up,R::TowardZero}){
  Ops ops{{},rounding};ops.table.fill(0.25f);
  const bool helper=mode!=2&&mode!=5&&!uint8_t(bypass);
  const bool lookup=helper&&uint8_t(conversion);
  const uint32_t selected=mode==1||mode==3?0xff808080u:mode==2?0xff000000u:mode==5?0xffffffffu:0xff405060u;
  const uint32_t expected=lookup?0xff3f3f3fu:selected;
  const auto actual=studioShaderColorPipelineCandidate(mode,{64,80,96},conversion,bypass,ops);
  if(actual!=expected||ops.lookups!=(lookup?3u:0u))return 1;
  ++cases;
 }
 std::printf("shader_color_pipeline_cases=%u distinct_argument_flags=1 synthetic_table=1\n",cases);return 0;
}
