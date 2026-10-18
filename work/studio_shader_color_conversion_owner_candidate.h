#pragma once
#include "studio_shader_color_conversion_candidate.h"
// Normalized live owner fields; service and lookup callbacks can change scales.
// No original object ABI or lookup table ownership is implied.
struct StudioShaderColorConversionOwnerCandidate {
 uint8_t flag5aa=0,flag5a9=0;
 float convertedScale=1,rawScale=1;
};
template<class Services>struct StudioShaderColorConversionOwnerAdapterCandidate {
 StudioShaderColorConversionOwnerCandidate& owner;Services& services;
 uint8_t flag5aa(){return owner.flag5aa;}
 uint8_t flag5a9(){return owner.flag5a9;}
 unsigned mode(){return services.mode();}
 float convertedScale(){return owner.convertedScale;}
 float rawScale(){return owner.rawScale;}
 float lookup(float value){return services.lookup(value);}
};
template<class Services>uint32_t studioShaderColorConversionOwnerCandidate(
 StudioShaderColorConversionOwnerCandidate& owner,std::array<uint8_t,3> rgb,uint32_t flag,Services& services){
 StudioShaderColorConversionOwnerAdapterCandidate<Services> adapter{owner,services};
 return studioShaderColorConversionCandidate(rgb,flag,adapter);
}
