#include "material_selfillum_composition_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
 const std::array<float,3>diffuse{.125f,.25f,.5f},albedo{.5f,.25f,.125f},tint{5,4,3};
 auto test=[&](float alpha,const std::array<float,3>*mask,bool enabled,std::array<float,3>expected){if(composeMaterialSelfIllumCandidate(diffuse,albedo,tint,alpha,mask,enabled)!=expected)throw std::runtime_error("composition differs");};
 test(0,nullptr,true,diffuse);test(1,nullptr,true,{2.5f,1,.375f});test(.5f,nullptr,true,{1.3125f,.625f,.4375f});
 std::array<float,3>rgbMask{0,.5f,1};test(0,&rgbMask,true,{.125f,.625f,.375f});test(1,&rgbMask,true,{.125f,.625f,.375f});
 test(1,&rgbMask,false,diffuse);
 unsigned rejected=0;for(float bad:{-1.f,2.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}){try{composeMaterialSelfIllumCandidate(diffuse,albedo,tint,bad,nullptr,true);}catch(const std::runtime_error&){++rejected;}}
 if(rejected!=4)throw std::runtime_error("invalid mask admitted");
 std::cout<<"PASS linear-space composition: disabled, alpha mask endpoints/interpolation, RGB explicit-mask precedence, unclamped HDR output, four malformed rejections; installed SFM/GPU parity unverified\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
