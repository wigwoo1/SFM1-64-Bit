#pragma once
#include "material_decimal_prefix_candidate.h"
#include "material_decimal_float_candidate.h"
#include <string_view>
inline constexpr const char*materialMatrixFormatCandidate=" [ %f %f %f %f  %f %f %f %f  %f %f %f %f  %f %f %f %f ]";
inline constexpr const char*materialTransformFormatCandidate=" center %f %f scale %f %f rotate %f translate %f %f";
// Formatted numeric assignment only. Matrix construction is a separate stage.
inline int materialMatrixScanCandidate(std::string_view input,bool transform,std::array<uint32_t,16>&words){
    std::string s(input);const auto nul=s.find('\0');if(nul!=std::string::npos)s.resize(nul);
    const std::string_view format=transform?materialTransformFormatCandidate:materialMatrixFormatCandidate;
    size_t at=0;int assigned=0;
    auto skip=[&](){while(at<s.size()&&materialStringSpaceCandidate(static_cast<unsigned char>(s[at])))++at;};
    for(size_t i=0;i<format.size();++i){
        if(format[i]==' '){skip();continue;}
        if(format[i]=='%'){
            ++i;skip();if(at==s.size())return assigned?assigned:-1;
            const size_t start=at;auto digit=[](char c){return c>='0'&&c<='9';};bool seen=false;
            if(at<s.size()&&(s[at]=='+'||s[at]=='-'))++at;
            while(at<s.size()&&digit(s[at])){++at;seen=true;}
            if(at<s.size()&&s[at]=='.'){++at;while(at<s.size()&&digit(s[at])){++at;seen=true;}}
            if(!seen)return assigned;
            if(at<s.size()&&(s[at]=='e'||s[at]=='E')){
                ++at;if(at<s.size()&&(s[at]=='+'||s[at]=='-'))++at;while(at<s.size()&&digit(s[at]))++at;
            }
            const std::string token=s.substr(start,at-start);
            // Installed scanf starts with350 bytes, copies into700 on first
            // growth, then its recalloc wrapper clears the existing token on
            // growth to1400. Model that value result without unsafe storage.
            // The scanner does not append positive sign characters to scratch.
            size_t stored=0;for(char c:token)if(c!='+')++stored;
            std::array<uint8_t,12>wide{};if(stored<700)materialConstructorDoubleCandidate(token,&wide);
            words[size_t(assigned)]=materialDecimalFloatCandidate(wide);++assigned;
        }else{
            if(at==s.size())return assigned?assigned:-1;
            if(s[at++]!=format[i])return assigned;
        }
    }
    return assigned;
}
