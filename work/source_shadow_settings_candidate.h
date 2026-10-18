#pragma once
#include <charconv>
#include <string_view>
// Prototype renderer setting; official SFM default/menu persistence remain unverified.
struct SourceShadowSettingsCandidate {
 uint32_t resolution=64;
 void setResolution(uint32_t value){if(!value)throw std::runtime_error("shadow resolution must be positive");resolution=value;}
};
inline uint32_t parseSourceShadowResolutionCandidate(std::string_view text){
 if(text.empty())throw std::runtime_error("invalid shadow resolution");
 uint32_t value=0;auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
 if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||!value)throw std::runtime_error("invalid shadow resolution");return value;
}
inline void testSourceShadowSettingsCandidate(){
 SourceShadowSettingsCandidate settings;settings.setResolution(128);if(settings.resolution!=128)throw std::runtime_error("shadow resolution change");settings.setResolution(64);if(settings.resolution!=64)throw std::runtime_error("shadow resolution restore");
 unsigned rejected=0;for(auto text:{"","0","-1","+64","64x"," 64","4294967296"}){try{parseSourceShadowResolutionCandidate(text);}catch(const std::runtime_error&){++rejected;}}if(rejected!=7||parseSourceShadowResolutionCandidate("128")!=128)throw std::runtime_error("shadow resolution parser coverage");
 bool zero=false;try{settings.setResolution(0);}catch(const std::runtime_error&){zero=true;}if(!zero||settings.resolution!=64)throw std::runtime_error("shadow resolution rejected change altered state");std::cout<<"shadow_settings_validation=passed invalid_cases=8 restore=exact\n";
}
