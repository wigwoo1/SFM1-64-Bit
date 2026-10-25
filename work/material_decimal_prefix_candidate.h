#pragma once
#include "material_constructor_decimal_candidate.h"
struct MaterialDecimalPrefixCandidate{double value=0;size_t consumed=0;};
// Immediate C-locale strtod-style input, bounded to the private fixture domain.
inline MaterialDecimalPrefixCandidate materialDecimalPrefixCandidate(const std::string&s){
    size_t i=0;while(i<s.size()&&materialStringSpaceCandidate(static_cast<unsigned char>(s[i])))++i;
    if(i<s.size()&&(s[i]=='+'||s[i]=='-'))++i;
    auto digit=[](char c){return c>='0'&&c<='9';};bool seen=false;
    while(i<s.size()&&digit(s[i])){seen=true;++i;}
    if(i<s.size()&&s[i]=='.'){++i;while(i<s.size()&&digit(s[i])){seen=true;++i;}}
    if(!seen)return {};
    const size_t mantissaEnd=i;
    if(i<s.size()&&(s[i]=='e'||s[i]=='E'||s[i]=='d'||s[i]=='D')){
        size_t saved=i++;if(i<s.size()&&(s[i]=='+'||s[i]=='-'))++i;
        const size_t first=i;while(i<s.size()&&digit(s[i]))++i;if(i==first)i=saved;
    }
    double value=materialConstructorDoubleCandidate(s);
    // Original strtod wrapper's underflow-zero branch returns its FLDZ result;
    // a literal -0 retains its sign. The atof/constructor contract differs.
    if(value==0){for(size_t j=0;j<mantissaEnd;++j)if(s[j]>='1'&&s[j]<='9'){value=0.0;break;}}
    return {value,i};
}
