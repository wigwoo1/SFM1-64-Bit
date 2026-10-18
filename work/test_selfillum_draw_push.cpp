#include "material_selfillum_draw_push_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
 std::array<float,24>prefix;for(size_t n=0;n<prefix.size();++n)prefix[n]=float(n);
 MaterialSelfIllumCandidate settings;settings.enabled=true;settings.mask="model/mask";settings.tint={5,5,5};
 auto push=materialSelfIllumDrawPushCandidate(prefix,.5f,.25f,settings,true,{2,3,4});
 if(push.existingVertexConstants!=prefix||push.alpha!=.5f||push.alphaCutoff!=.25f||push.enabled!=1||push.separateMask!=1||push.linearTint!=std::array<float,3>{2,3,4})throw std::runtime_error("draw constants lost");
 unsigned rejected=0;
 for(int mode=0;mode<3;++mode){try{auto tint=std::array<float,3>{2,3,4};if(mode==2)tint[1]=std::numeric_limits<float>::infinity();materialSelfIllumDrawPushCandidate(prefix,mode==1?2.f:.5f,0,settings,mode!=0,tint);}catch(const std::runtime_error&){++rejected;}}
 if(rejected!=3)throw std::runtime_error("invalid draw admitted");
 settings.enabled=false;auto disabled=materialSelfIllumDrawPushCandidate(prefix,1,0,settings,false,{1,1,1});if(disabled.enabled||disabled.separateMask)throw std::runtime_error("disabled draw requests mask");
 settings.enabled=true;settings.mask.clear();auto fallback=materialSelfIllumDrawPushCandidate(prefix,1,0,settings,false,{1,1,1});if(fallback.enabled!=1||fallback.separateMask)throw std::runtime_error("alpha fallback lost");
 std::cout<<"PASS native selfillum draw push: 124bytes, compiled shader offsets, prefix preserved, explicit linear tint, mask availability, disabled and alpha fallback, three invalid rejections; GPU binding pending\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
