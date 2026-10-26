#pragma once
#include "material_constructor_decimal_candidate.h"
struct MaterialDecimalPrefixCandidate{double value=0;size_t consumed=0;bool rangeError=false;};
// Immediate C-locale strtod-style input, bounded to the private fixture domain.
inline MaterialDecimalPrefixCandidate materialDecimalPrefixCandidate(const std::string&s){
    size_t i=0;while(i<s.size()&&materialStringSpaceCandidate(static_cast<unsigned char>(s[i])))++i;
    if(i<s.size()&&(s[i]=='+'||s[i]=='-'))++i;
    auto digit=[](char c){return c>='0'&&c<='9';};bool seen=false;
    while(i<s.size()&&digit(s[i])){seen=true;++i;}
    if(i<s.size()&&s[i]=='.'){++i;while(i<s.size()&&digit(s[i])){seen=true;++i;}}
    if(!seen)return {};
    if(i<s.size()&&(s[i]=='e'||s[i]=='E'||s[i]=='d'||s[i]=='D')){
        size_t saved=i++;if(i<s.size()&&(s[i]=='+'||s[i]=='-'))++i;
        const size_t first=i;while(i<s.size()&&digit(s[i]))++i;if(i==first)i=saved;
    }
    std::array<uint8_t,12>wide{};int parserStatus=0;
    double value=materialConstructorDoubleCandidate(s,&wide,&parserStatus);
    const auto converted=materialDecimalDoubleCandidate(wide);
    // Original strtod wrapper's underflow-zero branch returns its FLDZ result;
    // a literal -0 retains its sign. The atof/constructor contract differs.
    // The original wrapper consults parser/converter status. A nonzero
    // mantissa alone is insufficient: in-range wide scaling may yield exact
    // signed zero with neither stage reporting underflow (e.g. -1e-5200).
    const bool underflow=parserStatus==1||converted.status==2;
    const bool rangeError=parserStatus==2||converted.status==1||(underflow&&value==0);
    if(underflow&&value==0)value=0.0;
    return {value,i,rangeError};
}
