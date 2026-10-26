#pragma once
#include <cstdint>
#include <string_view>

// Parameter-name prefixes only (e.g. hdr?$color), not KeyValues bracket
// conditions, shader fallbacks or live device discovery. Supply explicit state.
struct MaterialParameterConditionEnvironmentCandidate {
    int32_t lowFill=0,hdr=0,gpu=0;
    uint32_t srgbReturn=0;
};
struct MaterialParameterConditionCandidate {
    bool skip=false,conditional=false,unrecognized=false;
};
inline bool materialParameterConditionCandidate(MaterialParameterConditionCandidate&out,
        std::string_view name,const MaterialParameterConditionEnvironmentCandidate&state) {
    if(name.size()>1023)return false;
    name=name.substr(0,name.find('\0'));
    const auto question=name.find('?');
    MaterialParameterConditionCandidate value;
    if(question==std::string_view::npos||question==0){out=value;return true;}
    // Original local prefix storage is 256 bytes including its terminator:
    // ebp-0x104 through ebp-5, immediately before the cookie at ebp-4.
    // Reject longer prefixes; never reproduce its unchecked stack overwrite.
    if(question>255)return false;
    value.conditional=true;auto prefix=name.substr(0,question);
    bool negate=false;if(prefix.front()=='!'){negate=true;prefix.remove_prefix(1);}
    auto equal=[&](std::string_view token){
        if(prefix.size()!=token.size())return false;
        for(size_t i=0;i<prefix.size();++i){
            auto lower=[](char c){return c>='A'&&c<='Z'?char(c-'A'+'a'):c;};
            if(lower(prefix[i])!=lower(token[i]))return false;
        }
        return true;
    };
    bool skip=true;
    if(equal("lowfill"))skip=state.lowFill==0;
    else if(equal("hdr"))skip=state.hdr==0;
    else if(equal("srgb"))skip=(state.srgbReturn&255u)==0;
    else if(equal("ldr"))skip=state.hdr!=0;
    else if(equal("GPU>=3"))skip=state.gpu<3;
    else if(equal("GPU>=2"))skip=state.gpu<2;
    else if(equal("GPU>=1"))skip=state.gpu<1;
    else if(equal("GPU<2"))skip=state.gpu>=2;
    else if(equal("GPU<3"))skip=state.gpu>=3;
    else if(equal("GPU<1"))skip=state.gpu>=1;
    else if(!equal("360"))value.unrecognized=true;
    value.skip=skip!=negate;out=value;return true;
}
