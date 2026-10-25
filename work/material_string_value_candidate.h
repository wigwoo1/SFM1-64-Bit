#pragma once
#include "material_numeric_value_candidate.h"
#include <cstdlib>
#include <cstring>
#include <locale.h>
#include <string>
#include <string_view>

// Owned immediate/ownerless string and numeric values. Resource types other
// than string are not admitted. The allocator/notification ABI is not adopted.
struct MaterialStringValueCandidate{
    MaterialNumericValueCandidate numeric;
    std::string text;
};
inline bool materialStringDomainCandidate(uint32_t type){
    return materialNumericResourceFreeCandidate(type)||(type&15)==1;
}
inline bool materialStringSpaceCandidate(unsigned char c){
    return c==' '||(c>=9&&c<=13);
}
inline uint32_t materialStringIntegerCandidate(std::string_view s){
    size_t i=0;while(i<s.size()&&materialStringSpaceCandidate(static_cast<unsigned char>(s[i])))++i;
    bool negative=false;if(i<s.size()&&(s[i]=='-'||s[i]=='+'))negative=s[i++]=='-';
    const uint32_t limit=negative?0x80000000u:0x7fffffffu;uint32_t value=0;
    for(;i<s.size()&&s[i]>='0'&&s[i]<='9';++i){
        const auto digit=uint32_t(s[i]-'0');
        if(value>(limit-digit)/10)return limit;
        value=value*10+digit;
    }
    return negative?0u-value:value;
}
inline uint32_t materialStringFloatCandidate(const std::string&s){
    // Bound parsing to the installed legacy decimal prefix grammar. Modern
    // CRTs also accept hex, inf and nan spellings that this parser did not.
    size_t start=0;while(start<s.size()&&materialStringSpaceCandidate(static_cast<unsigned char>(s[start])))++start;
    size_t i=start;if(i<s.size()&&(s[i]=='+'||s[i]=='-'))++i;
    bool digit=false;while(i<s.size()&&s[i]>='0'&&s[i]<='9'){digit=true;++i;}
    if(i<s.size()&&s[i]=='.'){++i;while(i<s.size()&&s[i]>='0'&&s[i]<='9'){digit=true;++i;}}
    if(!digit)return 0;
    if(i<s.size()&&(s[i]=='e'||s[i]=='E')){
        const auto exponent=i++;if(i<s.size()&&(s[i]=='+'||s[i]=='-'))++i;
        const auto first=i;while(i<s.size()&&s[i]>='0'&&s[i]<='9')++i;
        if(first==i)i=exponent;
    }
    const auto prefix=s.substr(start,i-start);
    struct CLocale{_locale_t value=_create_locale(LC_ALL,"C");~CLocale(){if(value)_free_locale(value);}};
    static const CLocale locale;
    if(!locale.value)throw std::bad_alloc();
    const float value=static_cast<float>(_strtod_l(prefix.c_str(),nullptr,locale.value));
    uint32_t bits;std::memcpy(&bits,&value,4);return bits;
}
inline bool materialStringSetCandidate(MaterialStringValueCandidate&value,uint32_t operation,
        const std::array<uint32_t,4>&arguments={},uint32_t count=0,std::string_view input={}){
    if(!materialStringDomainCandidate(value.numeric.typeAndMetadata)||operation>7)return false;
    if(operation!=7){
        if(!input.empty())return false;
        auto numeric=value.numeric;
        if((numeric.typeAndMetadata&15)==1)numeric.typeAndMetadata=(numeric.typeAndMetadata&~15u)|6;
        if(!materialNumericSetCandidate(numeric,MaterialNumericOpCandidate(operation),arguments,count))return false;
        value.numeric=numeric;value.text.clear();return true;
    }
    if(count||input.size()>4095)return false;
    // Prepare owned storage before mutation, including if input aliases value.text.
    // RAII/SSO allocation timing deliberately differs from original pointer code.
    const auto end=input.find('\0');std::string text(input.substr(0,end));
    auto numeric=value.numeric;numeric.typeAndMetadata=(numeric.typeAndMetadata&~15u)|1;
    numeric.integer=materialStringIntegerCandidate(text);
    numeric.vector.fill(materialStringFloatCandidate(text));
    value.numeric=numeric;value.text=std::move(text);return true;
}
