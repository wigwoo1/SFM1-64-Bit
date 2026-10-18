#include "studio_shader_color_select_candidate.h"
#include <cstdio>
int main(){unsigned cases=0;
 for(uint32_t mode=1;mode<=6;++mode)for(uint32_t flag=0;flag<512;++flag){
  const std::array<uint8_t,3> live{17,93,241};auto value=studioShaderColorSelectCandidate(mode,live,flag);
  auto expected=mode==1||mode==3?std::array<uint8_t,3>{128,128,128}:mode==2?std::array<uint8_t,3>{0,0,0}:mode==5?std::array<uint8_t,3>{255,255,255}:live;
  if(value.rgb!=expected||value.convert!=((mode!=2&&mode!=5)&&!uint8_t(flag)))return 1;
  const uint32_t packed=mode==1||mode==3?0xff808080u:mode==2?0xff000000u:mode==5?0xffffffffu:0xff115df1u;
  if(value.packed!=packed)return 2;++cases;
 }
 for(uint32_t mode:{0u,7u,0xffffffffu}){bool rejected=false;try{studioShaderColorSelectCandidate(mode,{0,0,0},0);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)return 3;}
 std::printf("shader_color_selection_cases=%u unknown_mode_rejections=3\n",cases);return 0;
}
