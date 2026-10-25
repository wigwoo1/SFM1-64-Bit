#pragma once
#include "material_decimal_prefix_candidate.h"
struct MaterialVectorLiteralCandidate{
    uint32_t count=0,warnings=0;
    std::array<uint32_t,4> words{};
};
inline bool materialVectorLiteralStartCandidate(std::string_view s){
    size_t i=0;while(i<s.size()&&(s[i]==' '||s[i]=='\t'))++i;
    return i<s.size()&&(s[i]=='['||s[i]=='{');
}
// Caller admits bracket/brace literals. Preserve all four scratch words even
// on malformed input: native parsing writes a failed numeric slot before return.
inline bool materialVectorLiteralCandidate(std::string_view input,MaterialVectorLiteralCandidate&out){
    if(input.size()>4095||!materialVectorLiteralStartCandidate(input))return false;
    std::string s(input);const auto nul=s.find('\0');if(nul!=std::string::npos)s.resize(nul);
    size_t i=0;while(i<s.size()&&(s[i]==' '||s[i]=='\t'))++i;const bool bytes=s[i++]=='{';
    auto value=out;value.count=value.warnings=0;
    while(value.count<4){
        while(i<s.size()&&(s[i]==' '||s[i]=='\t'))++i;
        const char ch=i<s.size()?s[i]:'\0';
        if(ch=='\n'||ch=='\0'||ch==']'||ch=='}'){
            if(ch!=']'&&ch!='}')++value.warnings;
            value.words[value.count]=0;break;
        }
        const auto number=materialDecimalPrefixCandidate(s.substr(i));
        const float scalar=static_cast<float>(number.value);
        std::memcpy(&value.words[value.count],&scalar,4);
        if(!number.consumed){value.count=0;++value.warnings;out=value;return true;}
        ++value.count;i+=number.consumed;
    }
    if(bytes){
        // Installed constant is verified as binary32 1/255 by the verifier.
        constexpr float reciprocal=1.0f/255.0f;
        for(auto&w:value.words){float x=0;std::memcpy(&x,&w,4);x*=reciprocal;std::memcpy(&w,&x,4);}
    }
    out=value;return true;
}
